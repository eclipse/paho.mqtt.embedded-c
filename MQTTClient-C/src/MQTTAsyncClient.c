/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *   Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *   Ian Craggs - fix for #96 - check rem_len in readPacket
 *   Ian Craggs - add ability to set message handler separately #6
 *******************************************************************************/
#include "MQTTAsyncClient.h"

#include <stdio.h>
#include <string.h>


static void NewMessageData(MQTTAsyncMessageData* md, MQTTString* aTopicName, MQTTAsyncMessage* aMessage) 
{
    md->topicName = aTopicName;
    md->message = aMessage;
}


static int getNextPacketId(MQTTAsyncClient *c) 
{
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}


static int sendPacket(MQTTAsyncClient* c, unsigned char *head_ptr, unsigned short length, unsigned char *payload_ptr, int payloadlen, MQTTAsyncTimer timer)
{
    int rc = MQTTAsync_FAILURE,
        sent = 0;

    while (sent < length && !c->plat_ptr->TimerIsExpired(timer))
    {
        rc = c->plat_ptr->MqttWrite(c->plat_ptr, &head_ptr[sent], length - sent, c->plat_ptr->TimerLeftMS(timer));
        if (rc < 0)
            break;
        sent += rc;
    }

    if (sent == length)
    {
        /* Send extra paylaod data when payload valid. */
        if((payload_ptr != NULL) && (payloadlen != 0))
        {
            sent = 0;
            while (sent < payloadlen && !c->plat_ptr->TimerIsExpired(timer))
            {
                rc = c->plat_ptr->MqttWrite(c->plat_ptr, &payload_ptr[sent], payloadlen - sent, c->plat_ptr->TimerLeftMS(timer));
                if (rc < 0)
                    break;
                sent += rc;
            } 
            if (sent == payloadlen)
            {
                c->plat_ptr->TimerCountdown(c->last_sent, c->initcfg_ptr->keepAliveInterval); // record the fact that we have successfully sent the packet
                rc = MQTTAsync_SUCCESS;
            }
            else
            {
                rc = MQTTAsync_FAILURE;
            }
        }
        else
        {
            c->plat_ptr->TimerCountdown(c->last_sent, c->initcfg_ptr->keepAliveInterval); // record the fact that we have successfully sent the packet
            rc = MQTTAsync_SUCCESS;
        }
    }
    else
    {
        rc = MQTTAsync_FAILURE;
    }
    
    return rc;
}


static int decodePacket(MQTTAsyncClient* c, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->plat_ptr->MqttRead(c->plat_ptr, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
exit:
    return len;
}


static int readPacket(MQTTAsyncClient* c, MQTTAsyncTimer timer, unsigned char waitWhenNodata, unsigned char **packet_pptr, int *packetlen_ptr)
{
    int                   rc = 0;
    MQTTHeader        header = {0};
    int                  len = 0;
    int              rem_len = 0;
    
    unsigned char first_byte = 0;
    unsigned char * buff_ptr = NULL;
    int            buff_size = 0;


    /* 1. read the header byte(packet type). */
    if(waitWhenNodata != 0)
    {
        /* Waiting for specific timer. */
        rc = c->plat_ptr->MqttRead(c->plat_ptr, &first_byte, 1, c->plat_ptr->TimerLeftMS(timer));
    }
    else
    {
        /* Only wait 10ms, jump out when no data. 
        Caution: must not less then 10ms, because some chip's task schedule unit bigger than 10ms. */
        rc = c->plat_ptr->MqttRead(c->plat_ptr, &first_byte, 1, 10);
    }
    if (rc != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length. This is variable in itself */
    decodePacket(c, &rem_len, c->plat_ptr->TimerLeftMS(timer));

    /* Build new buffer */
    if((buff_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, rem_len + 10)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }

    /* put the original remaining length back into the buffer */
    buff_ptr[0]= first_byte;
    len += MQTTPacket_encode(buff_ptr + 1, rem_len);

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0) 
    {
        rc = c->plat_ptr->MqttRead(c->plat_ptr, buff_ptr + len, rem_len, c->plat_ptr->TimerLeftMS(timer));
        if(rc != rem_len)
        {
            rc = 0;
            goto exit;
        }
    }

    /* Return packet type. */
    header.byte = buff_ptr[0];
    rc = header.bits.type;

    // record the fact that we have successfully received a packet
    if (c->initcfg_ptr->keepAliveInterval > 0)
        c->plat_ptr->TimerCountdown(c->last_received, c->initcfg_ptr->keepAliveInterval);

    /* Return packet data and length. */
    * packet_pptr = buff_ptr;
    * packetlen_ptr = buff_size;
    
    return rc;
    
exit:

    if(buff_ptr != NULL)
    {
        c->plat_ptr->MemFree(buff_ptr);
        buff_ptr = NULL;
    }
    
    return rc;
}


// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}


static int deliverMessage(MQTTAsyncClient* c, MQTTString* topicName, MQTTAsyncMessage* message)
{
    int i;
    int rc = MQTTAsync_FAILURE;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < c->max_message_handlers; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MQTTAsyncMessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(c->messageHandlers[i].context_ptr, &md);
                rc = MQTTAsync_SUCCESS;
            }
        }
    }

    if (rc == MQTTAsync_FAILURE && c->initcfg_ptr->defaultMessageHandler != NULL)
    {
        MQTTAsyncMessageData md;
        NewMessageData(&md, topicName, message);
        c->initcfg_ptr->defaultMessageHandler(c->initcfg_ptr->defaultMessageCtx_ptr, &md);
        rc = MQTTAsync_SUCCESS;
    }

    return rc;
}


static int keepalive(MQTTAsyncClient* c)
{
#define KEEPALIVE_TRYCNT  4

    int                 rc = MQTTAsync_SUCCESS;
    int                len = 0;
    unsigned char head[10] = { 0 };


    if (c->initcfg_ptr->keepAliveInterval == 0)
        goto exit;

    if(c->plat_ptr->TimerIsExpired(c->last_sent))
    {
        MQTTAsyncTimer timer = NULL;

        c->plat_ptr->TimerInit(&timer);
        c->plat_ptr->TimerCountdownMS(timer, 1000);
        
        len = MQTTSerialize_pingreq(head, sizeof(head));
        rc = sendPacket(c, head, len, NULL, 0, timer);

        /* Set ping flag. */
        c->ping_outstanding ++;
          
        if(KEEPALIVE_TRYCNT <= c->ping_outstanding)
        {
            c->ping_outstanding = 0;
            rc = MQTTAsync_FAILURE; /* PINGRESP not received in keepalive interval */
        }
        else
        {
            /* Set last_sent to small slice. */
            c->plat_ptr->TimerCountdown(c->last_sent, (30 < c->initcfg_ptr->keepAliveInterval)? 4 : 2);
            rc = MQTTAsync_SUCCESS;     
        }

		if(timer != NULL)
		{
			c->plat_ptr->TimerDeinit(&timer);
			timer = NULL;
		}

		/* MQTTAsync will not wait server's pong frame when nopong state active. */
		if(c->nopong_stat != 0)
		{
			/* Set last_sent to keep alive data. */
            c->ping_outstanding = 0;
            c->plat_ptr->TimerCountdown(c->last_sent, c->initcfg_ptr->keepAliveInterval);
		}
    }

exit:

    return rc;
    
#undef KEEPALIVE_TRYCNT
}


void MQTTAsyncCleanSession(MQTTAsyncClient* c)
{
    int i = 0;

    for (i = 0; i < c->max_message_handlers; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}


void MQTTAsyncCloseSession(MQTTAsyncClient* c)
{
    c->ping_outstanding = 0;
    c->isconnected = 0;
    
    if (c->cleansession)
        MQTTAsyncCleanSession(c);
}


static int MQTTAsyncCycle(MQTTAsyncClient* c, MQTTAsyncTimer timer, unsigned char waitWhenNodata, int outpacket_type, unsigned char **packet_pptr, int *packetlen_ptr, unsigned char async_ackpacket)
{
    int                   len = 0;
    int                    rc = MQTTAsync_SUCCESS;
    unsigned int     waittime = c->plat_ptr->TimerLeftMS(timer);

    unsigned char *packet_ptr = NULL;
    int             packetlen = 0;
    int           packet_type = 0;


	/* read the socket, see what work is due */
    packet_type = readPacket(c, timer, waitWhenNodata, &packet_ptr, &packetlen);
    switch (packet_type)
    {
        default:
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            rc = packet_type;
            goto exit;
            
        case 0: /* timed out reading packet */
            break;
            
        case CONNACK:
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
            break;
        
        case PUBLISH:
        {
            MQTTString   topicName = { 0 };
            MQTTAsyncMessage   msg = { 0 };
            int             intQoS = 0;
            unsigned char head[10] = { 0 };
    
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName, (unsigned char**)&msg.payload, (int*)&msg.payloadlen, packet_ptr, packetlen) != 1)
                goto exit;
            
            msg.qos = (enum QoS)intQoS;
            deliverMessage(c, &topicName, &msg);
            
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(head, sizeof(head), PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(head, sizeof(head), PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = MQTTAsync_FAILURE;
                else
                {
                    /* Reset the waittime before sending packet. because routine 'deliverMessage' would use a lot. */
                    c->plat_ptr->TimerCountdownMS(timer, waittime);
                    rc = sendPacket(c, head, len, NULL, 0, timer);
                }
                if (rc == MQTTAsync_FAILURE)
                    goto exit;
            }
            
            break;
        }
        case PUBREC:
        case PUBREL:
        {
            unsigned short mypacketid = 0;
            unsigned char         dup = 0;
            unsigned char        type = 0;
            unsigned char    head[10] = { 0 };
            
            /* Reset the waittime before sendint packet. only in case. */
            c->plat_ptr->TimerCountdownMS(timer, waittime);

            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, packet_ptr, packetlen) != 1)
                rc = MQTTAsync_FAILURE;
            else if ((len = MQTTSerialize_ack(head, sizeof(head), (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = MQTTAsync_FAILURE;
            else if ((rc = sendPacket(c, head, len, NULL, 0, timer)) != MQTTAsync_SUCCESS) // send the PUBREL packet
                rc = MQTTAsync_FAILURE;
                
            if (rc == MQTTAsync_FAILURE)
                goto exit;
                
            break;
        }

        case PUBCOMP:
            break;
            
        case PINGRESP:
            /* Set last_sent to keep alive data. */
            c->ping_outstanding = 0;
            c->plat_ptr->TimerCountdown(c->last_sent, c->initcfg_ptr->keepAliveInterval);
        
            break;
    }

    if (keepalive(c) != MQTTAsync_SUCCESS) 
    {
        //check only keepalive MQTTAsync_FAILURE status so that previous MQTTAsync_FAILURE status can be considered as FAULT
        rc = MQTTAsync_FAILURE;
    }

exit:

    if (rc == MQTTAsync_SUCCESS)
        rc = packet_type;
    else if (c->isconnected)
        MQTTAsyncCloseSession(c);

    if((outpacket_type != 0) && (outpacket_type == packet_type))
    {
        /* Give packet data out when packet type be we wanted. */
        * packet_pptr = packet_ptr;
        * packetlen_ptr = packetlen;
    }
    else
    {
        /* Store async ack packet for other thread to check. */
        if((async_ackpacket != 0) && (packet_ptr != NULL)
        && ((packet_type == CONNACK) || (packet_type == PUBACK) || (packet_type == SUBACK) || (packet_type == UNSUBACK) || (packet_type == PUBCOMP)))
        {
            int     i = 0;

            for(i = 0; i < (sizeof(c->async_ackpacket)/sizeof(c->async_ackpacket[0])); i++)
            {
                /* Store to empty room. */
                if(c->async_ackpacket[i].packet_type == 0)
                {      
                    if(c->async_ackpacket[i].packet_ptr != NULL)
                    {
                        c->plat_ptr->MemFree(c->async_ackpacket[i].packet_ptr);
                        c->async_ackpacket[i].packet_ptr = NULL;
                    }

                    c->async_ackpacket[i].packet_ptr = packet_ptr;
                    c->async_ackpacket[i].packetlen = packetlen;
                    c->async_ackpacket[i].packet_type = packet_type;
                    break;
                }
            }

            if(i == (sizeof(c->async_ackpacket)/sizeof(c->async_ackpacket[0])))
            {
                printf("Error: Async ack packet room overflow, we assume will not be here.\n");

                c->plat_ptr->MemFree(packet_ptr);
                packet_ptr = NULL;
            }
        }
        else
        {
            /* Free packet buffer when: 1. no need to give packet data out; 2. not wanted packet type. */
            if(packet_ptr != NULL)
            {
                c->plat_ptr->MemFree(packet_ptr);
                packet_ptr = NULL;
            }
        }
    }
    
    return rc;
}


int MQTTAsyncYield(MQTTAsyncClient* c, int timeout_ms, unsigned char waitWhenNodata, unsigned char isAsyncAck)
{
    int               rc = MQTTAsync_SUCCESS;
    MQTTAsyncTimer timer = NULL;


    c->plat_ptr->TimerInit(&timer);
    c->plat_ptr->TimerCountdownMS(timer, timeout_ms);

    rc = MQTTAsyncCycle(c, timer, waitWhenNodata, 0, NULL, NULL, isAsyncAck);
    
	if(timer != NULL)
	{
		c->plat_ptr->TimerDeinit(&timer);
		timer = NULL;
	}

    return rc;
}

int MQTTAsyncIsConnected(MQTTAsyncClient* client)
{
    return client->isconnected;
}


static int MQTTAsyncWaitfor(MQTTAsyncClient* c, int packet_type, MQTTAsyncTimer timer, unsigned char **packet_pptr, int *packetlen_ptr)
{
    int                     rc = MQTTAsync_FAILURE;
    unsigned char * packet_ptr = NULL;
    int              packetlen = 0;


    do
    {
        /* Read once no matter whatever for timer. Caution: Waiting data when no data in receive buffer. */
        rc = MQTTAsyncCycle(c, timer, 1, packet_type, packet_pptr, packetlen_ptr, 0);
        if(c->plat_ptr->TimerIsExpired(timer))
        {
            if(rc <= 0)
            {
                /* Stop loop when no data received or read error. */
                break;
            }
            else
            {
                /* Give more 500ms for next reading when reading other valid type packet. */
                c->plat_ptr->TimerCountdownMS(timer, 500);
            }
        }
    }
    while (rc != packet_type && rc >= 0);

    return rc;
}


/* Waitting packet from mqtt buffer in asychrous mode. Caution: Caller routine must lock the mutex because this routine will unlock mutex. */
static int MQTTAsyncWaitFromBuffer(MQTTAsyncClient* c, int packet_type, unsigned short id, MQTTAsyncTimer timer, unsigned char **packet_pptr, int *packetlen_ptr)
{
    int            rc = MQTTAsync_FAILURE;
    int             i = 0;


    do
    {
        c->plat_ptr->MutexUnlock(&(c->inner_mutex));
        
        /* Sleep 50ms every loop. */
        c->plat_ptr->DelayMs(50);
        
        c->plat_ptr->MutexLock(&(c->inner_mutex));

        /* Traverse array and get specific packet. */
        for(i = 0; i < (sizeof(c->async_ackpacket)/sizeof(c->async_ackpacket[0])); i++)
        {
            if((c->async_ackpacket[i].packet_type != 0) && (c->async_ackpacket[i].packet_type == packet_type))
            {
                if(c->async_ackpacket[i].packet_ptr != NULL)
                {
                    unsigned short mypacketid = 0;
                    unsigned char         dup = 0;
                    unsigned char        type = 0;
                    
                    MQTTDeserialize_ack(&type, &dup, &mypacketid, c->async_ackpacket[i].packet_ptr, c->async_ackpacket[i].packetlen);

                    /* Find success and fetch packet data. Caution: set id=0 means donot care msgid. */
                    if(((mypacketid == id) && (id != 0)) || (id == 0))
                    {
                        *packet_pptr = c->async_ackpacket[i].packet_ptr;
                        *packetlen_ptr = c->async_ackpacket[i].packetlen;

                        c->async_ackpacket[i].packet_type = 0;
                        c->async_ackpacket[i].packet_ptr = NULL;
                        c->async_ackpacket[i].packetlen = 0;

                        rc = packet_type;
                        goto exit;
                    }
                }
                else
                {
                    printf("Error: Async ack packet buffer NULL, we assume will not be here.\n");
                }
            }
        }

        /* Out when time exipired. */
        if(c->plat_ptr->TimerIsExpired(timer))
        {
            break;
        }
    }
    while (1);

exit:

    return rc;
}


static int MQTTAsyncConnectWithResults(MQTTAsyncClient* c, MQTTPacket_connectData* options, MQTTAsyncConnackData* data, unsigned char isAsyncAck)
{
    MQTTAsyncTimer connect_timer = NULL;
    int                   rc = MQTTAsync_FAILURE;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int                  len = 0;
    int                  cnt = 0;
    unsigned char * head_ptr = NULL;
    int            head_size = 0;
    

    c->plat_ptr->MutexLock(&(c->inner_mutex));

	if (c->isconnected) /* don't send connect packet again if we are already connected */
		  goto exit;

    c->plat_ptr->TimerInit(&connect_timer);

    /* Create new buffer. */
    head_size = MQTTPacket_len(MQTTSerialize_connectLength(options)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }

SEND_START:

    c->plat_ptr->TimerCountdownMS(connect_timer, c->initcfg_ptr->cmdTimeoutMs);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    c->initcfg_ptr->keepAliveInterval = options->keepAliveInterval;
    c->cleansession = options->cleansession;
    c->plat_ptr->TimerCountdown(c->last_received, c->initcfg_ptr->keepAliveInterval);

    if ((len = MQTTSerialize_connect(head_ptr, head_size, options)) <= 0)
        goto exit;
    
    if ((rc = sendPacket(c, head_ptr, len, NULL, 0, &connect_timer)) != MQTTAsync_SUCCESS)
        goto exit;
    
    if(isAsyncAck == 0)
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        // this will be a blocking call, wait for the connack
        if (MQTTAsyncWaitfor(c, CONNACK, &connect_timer, &packet_ptr, &packetlen) == CONNACK)
        {
            data->rc = 0;
            data->sessionPresent = 0;
            
            if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, packet_ptr, packetlen) == 1)
                rc = data->rc;
            else
                rc = MQTTAsync_FAILURE;
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }
    else
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        // this will be a blocking call, wait for the connack
        if (MQTTAsyncWaitFromBuffer(c, CONNACK, 0, &connect_timer, &packet_ptr, &packetlen) == CONNACK)
        {
            data->rc = 0;
            data->sessionPresent = 0;
            
            if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, packet_ptr, packetlen) == 1)
                rc = data->rc;
            else
                rc = MQTTAsync_FAILURE;
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }

exit:

    if(rc == MQTTAsync_SUCCESS) { c->isconnected = 1; c->ping_outstanding = 0; }
	if(connect_timer != NULL)   { c->plat_ptr->TimerDeinit(&connect_timer); connect_timer = NULL; }
	if(head_ptr != NULL)        { c->plat_ptr->MemFree(head_ptr); head_ptr = NULL; }
    
    c->plat_ptr->MutexUnlock(&(c->inner_mutex));
    return rc;
}


void MQTTAsyncClientInit(MQTTAsyncClient* c, MQTTAsyncPlatform *platform, MQTTAsyncClientCfg* initcfg)
{
    /* Reset model memory and init model strucuture. */
    memset(c, 0, sizeof(*c));
    
    c->plat_ptr = platform;
    c->initcfg_ptr = initcfg;

    c->isconnected = 0;
    c->cleansession = 0;
    c->ping_outstanding = 0;
    c->next_packetid = 1;


    c->max_message_handlers = 0;
    c->messageHandlers = NULL;

    c->plat_ptr->TimerInit(&(c->last_sent));
    c->plat_ptr->TimerInit(&(c->last_received));
    c->plat_ptr->MutexInit(&(c->read_mutex));
    c->plat_ptr->MutexInit(&(c->write_mutex));

    c->plat_ptr->MutexInit(&(c->inner_mutex));
}


int MQTTAsyncClientDeinit(MQTTAsyncClient* c, int isSendPacket)
{
    int               rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer timer = NULL;     // we might wait for incomplete incoming publishes to complete
    int              len = 0;
    unsigned char head[10] = { 0 };
    int                i = 0;


    c->plat_ptr->TimerInit(&timer);
    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

	/* Send MQTTAsync disconnect package when tcp link ok. Caution: Some platform will crash when call send routine */
	if(isSendPacket != 0)
	{
        len = MQTTSerialize_disconnect(head, sizeof(head));
        if (len > 0)
            rc = sendPacket(c, head, len, NULL, 0, timer);
	}

    MQTTAsyncCloseSession(c);

    if(c->messageHandlers != NULL) { c->plat_ptr->MemFree(c->messageHandlers); c->messageHandlers = NULL; }
    c->max_message_handlers = 0;

	if(c->last_sent != NULL)     { c->plat_ptr->TimerDeinit(&(c->last_sent)); c->last_sent = NULL; }
	if(c->last_received != NULL) { c->plat_ptr->TimerDeinit(&(c->last_received)); c->last_received = NULL; }
    
	if(c->read_mutex != NULL)    { c->plat_ptr->MutexDeinit(&(c->read_mutex)); c->read_mutex = NULL; }
	if(c->write_mutex != NULL)   { c->plat_ptr->MutexDeinit(&(c->write_mutex)); c->write_mutex = NULL; }

	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer); timer = NULL; }

    c->plat_ptr->MutexLock(&(c->inner_mutex));
    for(i = 0; i < (sizeof(c->async_ackpacket)/sizeof(c->async_ackpacket[0])); i++)
    {
        c->async_ackpacket[i].packet_type = 0;
        c->async_ackpacket[i].packetlen = 0;
        
        if(c->async_ackpacket[i].packet_ptr != NULL)
        {
            c->plat_ptr->MemFree(c->async_ackpacket[i].packet_ptr);
            c->async_ackpacket[i].packet_ptr = NULL;
        }
    }
    c->plat_ptr->MutexUnlock(&(c->inner_mutex));

    if(c->inner_mutex != NULL)   { c->plat_ptr->MutexDeinit(&(c->inner_mutex)); c->inner_mutex = NULL; }
    return rc;
}

int MQTTAsyncConnect(MQTTAsyncClient* c, MQTTPacket_connectData* options, unsigned char isAsyncAck)
{
    MQTTAsyncConnackData data = { 0 };
    return MQTTAsyncConnectWithResults(c, options, &data, isAsyncAck);
}


int MQTTAsyncDisconnect(MQTTAsyncClient* c, int isSendPacket)
{
    int               rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer timer = NULL;     // we might wait for incomplete incoming publishes to complete
    int              len = 0;
    unsigned char head[10] = { 0 };
    int                i = 0;


    c->plat_ptr->TimerInit(&timer);
    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

	/* Send MQTTAsync disconnect package when tcp link ok. Caution: Some platform will crash when call send routine */
	if(isSendPacket != 0)
	{
        len = MQTTSerialize_disconnect(head, sizeof(head));
        if (len > 0)
            rc = sendPacket(c, head, len, NULL, 0, timer);
	}

    MQTTAsyncCloseSession(c);

    if(c->messageHandlers != NULL) { c->plat_ptr->MemFree(c->messageHandlers); c->messageHandlers = NULL; }
    c->max_message_handlers = 0;

	if(c->last_sent != NULL)     { c->plat_ptr->TimerDeinit(&(c->last_sent)); c->last_sent = NULL; }
	if(c->last_received != NULL) { c->plat_ptr->TimerDeinit(&(c->last_received)); c->last_received = NULL; }
    
	if(c->read_mutex != NULL)    { c->plat_ptr->MutexDeinit(&(c->read_mutex)); c->read_mutex = NULL; }
	if(c->write_mutex != NULL)   { c->plat_ptr->MutexDeinit(&(c->write_mutex)); c->write_mutex = NULL; }

	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer); timer = NULL; }

    c->plat_ptr->MutexLock(&(c->inner_mutex));
    for(i = 0; i < (sizeof(c->async_ackpacket)/sizeof(c->async_ackpacket[0])); i++)
    {
        c->async_ackpacket[i].packet_type = 0;
        c->async_ackpacket[i].packetlen = 0;
        
        if(c->async_ackpacket[i].packet_ptr != NULL)
        {
            c->plat_ptr->MemFree(c->async_ackpacket[i].packet_ptr);
            c->async_ackpacket[i].packet_ptr = NULL;
        }
    }
    c->plat_ptr->MutexUnlock(&(c->inner_mutex));

    if(c->inner_mutex != NULL)   { c->plat_ptr->MutexDeinit(&(c->inner_mutex)); c->inner_mutex = NULL; }
    return rc;
}


static int MQTTAsyncSetMessageHandler(MQTTAsyncClient* c, const char* topicFilter, messageHandler msgHandler, void *context_ptr)
{
    int                             rc = MQTTAsync_FAILURE;
    int                              i = -1;
    MQTTAsyncMessageHandlers * new_msghdler_ptr = NULL;
    int                   new_maxhdler = 0;


SETMSG_START:
    
    /* first check for an existing matching slot */
    for (i = 0; i < c->max_message_handlers; ++i)
    {
        if (c->messageHandlers[i].topicFilter != NULL && strcmp(c->messageHandlers[i].topicFilter, topicFilter) == 0)
        {
            if (msgHandler == NULL) /* remove existing */
            {
                c->messageHandlers[i].topicFilter = NULL;
                c->messageHandlers[i].fp = NULL;
            }
            rc = MQTTAsync_SUCCESS; /* return i when adding new subscription */
            break;
        }
    }
    /* if no existing, look for empty slot (unless we are removing) */
    if (msgHandler != NULL) {
        if (rc == MQTTAsync_FAILURE)
        {
            for (i = 0; i < c->max_message_handlers; ++i)
            {
                if (c->messageHandlers[i].topicFilter == NULL)
                {
                    rc = MQTTAsync_SUCCESS;
                    break;
                }
            }
        }

        if (i < c->max_message_handlers)
        {
            c->messageHandlers[i].topicFilter = topicFilter;
            c->messageHandlers[i].fp = msgHandler;
            c->messageHandlers[i].context_ptr = context_ptr;
        }
        else
        {
            new_maxhdler = c->max_message_handlers + 30;
            if((new_msghdler_ptr = (MQTTAsyncMessageHandlers*)c->plat_ptr->MemCalloc(new_maxhdler, sizeof(*new_msghdler_ptr))) != NULL)
            {
                if(c->messageHandlers != NULL)
                {
                    memcpy(new_msghdler_ptr, c->messageHandlers, c->max_message_handlers * sizeof(*c->messageHandlers));
                    c->plat_ptr->MemFree(c->messageHandlers);
                }
                c->messageHandlers = new_msghdler_ptr;
                c->max_message_handlers = new_maxhdler;

                goto SETMSG_START;
            }
            else
            {
                rc = MQTTAsync_FAILURE;
            } 
        }
    }
    return rc;
}


static int MQTTAsyncSubscribeWithResults(MQTTAsyncClient* c, const char* topicFilter, int qos, messageHandler msgHandler, void *context_ptr, MQTTAsyncSubackData* data, unsigned char isAsyncAck)
{
    int                   rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer     timer = NULL;
    int                  len = 0;
    MQTTString         topic = MQTTString_initializer;
    int                  cnt = 0;
    unsigned char * head_ptr = NULL;
    int            head_size = 0;
    unsigned short  packetid = 0;


    c->plat_ptr->MutexLock(&(c->inner_mutex));
    
	if (!c->isconnected)
		goto exit;

    c->plat_ptr->TimerInit(&timer);
    topic.cstring = (char *)topicFilter;

    /* Create new buffer. */
    head_size = MQTTPacket_len(MQTTSerialize_subscribeLength(1, &topic)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }

SEND_START: 

    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

    packetid = getNextPacketId(c);
    len = MQTTSerialize_subscribe(head_ptr, head_size, 0, packetid, 1, &topic, (int*)&qos);
    if (len <= 0)
        goto exit;

    if ((rc = sendPacket(c, head_ptr, len, NULL, 0, timer)) != MQTTAsync_SUCCESS)
        goto exit;

    if(isAsyncAck == 0)
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;
        
        if (MQTTAsyncWaitfor(c, SUBACK, timer, &packet_ptr, &packetlen) == SUBACK)
        {
            int count = 0;
            unsigned short mypacketid;
            data->grantedQoS = QOS0;
        
            if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, packet_ptr, packetlen) == 1)
            {
                if (data->grantedQoS != 0x80)
                {
                    rc = MQTTAsyncSetMessageHandler(c, topicFilter, msgHandler, context_ptr);
                }
                else
                {
                    rc = MQTTAsync_FAILURE;
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }
    else
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;
        
        if (MQTTAsyncWaitFromBuffer(c, SUBACK, packetid, timer, &packet_ptr, &packetlen) == SUBACK)
        {
            int count = 0;
            unsigned short mypacketid;
            data->grantedQoS = QOS0;
        
            if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, packet_ptr, packetlen) == 1)
            {
                if (data->grantedQoS != 0x80)
                {
                    rc = MQTTAsyncSetMessageHandler(c, topicFilter, msgHandler, context_ptr);
                }
                else
                {
                    rc = MQTTAsync_FAILURE;
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }

exit:

    if (rc == MQTTAsync_FAILURE) { MQTTAsyncCloseSession(c); }
	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer); timer = NULL; }
	if(head_ptr != NULL)         { c->plat_ptr->MemFree(head_ptr); head_ptr = NULL; }

    c->plat_ptr->MutexUnlock(&(c->inner_mutex)); 
    return rc;
}


int MQTTAsyncSubscribe(MQTTAsyncClient* c, const char* topicFilter, int qos, messageHandler msgHandler, void *context_ptr, unsigned char isAsyncAck)
{
    MQTTAsyncSubackData data;
    return MQTTAsyncSubscribeWithResults(c, topicFilter, qos, msgHandler, context_ptr, &data, isAsyncAck);
}


static int MQTTAsyncSubscribeManyWithResults(MQTTAsyncClient* c, int count, MQTTAsyncMessageHandlers msgHandlers[], int qos[], int grantedQoS[], unsigned char isAsyncAck)
{
    int                 rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer        timer = NULL;
    int                  i = 0;
    int                len = 0;
    MQTTString * topic_ptr = NULL;
    int                cnt = 0;
    unsigned char * head_ptr = NULL;
    int            head_size = 0;
    unsigned short  packetid = 0;


    c->plat_ptr->MutexLock(&(c->inner_mutex));
    
	if (!c->isconnected)
		goto exit;

    c->plat_ptr->TimerInit(&timer);

    /* Give room for topic sub. */
    if((topic_ptr = c->plat_ptr->MemCalloc(count, sizeof(MQTTString))) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }
    for(i = 0; i < count; i++)
    {
        topic_ptr[i].cstring = (char*)msgHandlers[i].topicFilter;
    }

    /* Create new buffer. */
    head_size = MQTTPacket_len(MQTTSerialize_subscribeLength(count, topic_ptr)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }

SEND_START: 

    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

    packetid = getNextPacketId(c);
    len = MQTTSerialize_subscribe(head_ptr, head_size, 0, packetid, count, topic_ptr, (int*)qos);
    if (len <= 0)
        goto exit;

    if ((rc = sendPacket(c, head_ptr, len, NULL, 0, timer)) != MQTTAsync_SUCCESS) // send the subscribe packet
        goto exit;

    if(isAsyncAck == 0)
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitfor(c, SUBACK, timer, &packet_ptr, &packetlen) == SUBACK)      // wait for suback
        {
            int             ret_count = 0;
            unsigned short mypacketid = 0;

            if (MQTTDeserialize_suback(&mypacketid, count, &ret_count, grantedQoS, packet_ptr, packetlen) == 1)
            {
                /* Error out when return count not equal to requst count. */
                if(ret_count != count)
                {
                    if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                    
                    rc = MQTTAsync_FAILURE;
                    goto exit;
                }

                /* Store msg handler. */
                for(i = 0; i < count; i++)
                {
                    /* Store all successd handler routine. */
                    if (grantedQoS[i] != SUBFAIL)
                    {
                        rc = MQTTAsyncSetMessageHandler(c, msgHandlers[i].topicFilter, msgHandlers[i].fp, msgHandlers[i].context_ptr);
                    }
                    else
                    {
                        rc = MQTTAsync_FAILURE;
                    }
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; } 
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }
    else
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitFromBuffer(c, SUBACK, packetid, timer, &packet_ptr, &packetlen) == SUBACK)      // wait for suback
        {
            int             ret_count = 0;
            unsigned short mypacketid = 0;

            if (MQTTDeserialize_suback(&mypacketid, count, &ret_count, grantedQoS, packet_ptr, packetlen) == 1)
            {
                /* Error out when return count not equal to requst count. */
                if(ret_count != count)
                {
                    if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                    
                    rc = MQTTAsync_FAILURE;
                    goto exit;
                }

                /* Store msg handler. */
                for(i = 0; i < count; i++)
                {
                    /* Store all successd handler routine. */
                    if (grantedQoS[i] != SUBFAIL)
                    {
                        rc = MQTTAsyncSetMessageHandler(c, msgHandlers[i].topicFilter, msgHandlers[i].fp, msgHandlers[i].context_ptr);
                    }
                    else
                    {
                        rc = MQTTAsync_FAILURE;
                    }
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; } 
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }        
    }

exit:

    if (rc == MQTTAsync_FAILURE) { MQTTAsyncCloseSession(c); }
	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer);    timer = NULL; }
    if(topic_ptr != NULL)        { c->plat_ptr->MemFree(topic_ptr); topic_ptr = NULL; }
    if(head_ptr != NULL)         { c->plat_ptr->MemFree(head_ptr);   head_ptr = NULL; }
    
    c->plat_ptr->MutexUnlock(&(c->inner_mutex)); 
    return rc;
}


int MQTTAsyncSubscribeMany(MQTTAsyncClient* c, int count, MQTTAsyncMessageHandlers msgHandlers[], int qos[], int grantedQoS[], unsigned char isAsyncAck)
{
    return MQTTAsyncSubscribeManyWithResults(c, count, msgHandlers, qos, grantedQoS, isAsyncAck);
}


int MQTTAsyncUnsubscribe(MQTTAsyncClient* c, const char* topicFilter, unsigned char isAsyncAck)
{
    int rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer timer = NULL;
    MQTTString topic = MQTTString_initializer;

    int                  len = 0;
    int                  cnt = 0;
    unsigned char * head_ptr = NULL;
    int            head_size = 0;
    unsigned short  packetid = 0;


    c->plat_ptr->MutexLock(&(c->inner_mutex));

	if (!c->isconnected)
		goto exit;

    c->plat_ptr->TimerInit(&timer);
    topic.cstring = (char *)topicFilter;

    /* Create new buffer. */
    head_size = MQTTPacket_len(MQTTSerialize_unsubscribeLength(1, &topic)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }
    
SEND_START:
    
    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

    packetid = getNextPacketId(c);
    if ((len = MQTTSerialize_unsubscribe(head_ptr, head_size, 0, packetid, 1, &topic)) <= 0)
        goto exit;
    
    if ((rc = sendPacket(c, head_ptr, len, NULL, 0, timer)) != MQTTAsync_SUCCESS)
        goto exit; // there was a problem

    if(isAsyncAck == 0)
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitfor(c, UNSUBACK, timer, &packet_ptr, &packetlen) == UNSUBACK)
        {
            unsigned short mypacketid;  // should be the same as the packetid above
            if (MQTTDeserialize_unsuback(&mypacketid, packet_ptr, packetlen) == 1)
            {
                /* remove the subscription message handler associated with this topic, if there is one */
                MQTTAsyncSetMessageHandler(c, topicFilter, NULL, NULL);
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }
    else
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitFromBuffer(c, UNSUBACK, packetid, timer, &packet_ptr, &packetlen) == UNSUBACK)
        {
            unsigned short mypacketid;  // should be the same as the packetid above
            if (MQTTDeserialize_unsuback(&mypacketid, packet_ptr, packetlen) == 1)
            {
                /* remove the subscription message handler associated with this topic, if there is one */
                MQTTAsyncSetMessageHandler(c, topicFilter, NULL, NULL);
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }        
    }

exit:

    if (rc == MQTTAsync_FAILURE) { MQTTAsyncCloseSession(c); }
	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer); timer = NULL; }
    if(head_ptr != NULL)         { c->plat_ptr->MemFree(head_ptr); head_ptr = NULL; }

    c->plat_ptr->MutexUnlock(&(c->inner_mutex));
    return rc;
}


int MQTTAsyncUnsubscribeMany(MQTTAsyncClient* c, int count, const char* topicFilter[], unsigned char isAsyncAck)
{
    int                 rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer   timer = NULL;
    MQTTString * topic_ptr = NULL;
    int                  i = 0;

    int                  len = 0;
    int                  cnt = 0;
    unsigned char * head_ptr = NULL;
    int            head_size = 0;
    unsigned short  packetid = 0;


    c->plat_ptr->MutexLock(&(c->inner_mutex));

	if (!c->isconnected)
		goto exit;

    c->plat_ptr->TimerInit(&timer);

    /* Give room for topic sub. */
    if((topic_ptr = c->plat_ptr->MemCalloc(count, sizeof(MQTTString))) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }
    for(i = 0; i < count; i++)
    {
        topic_ptr[i].cstring = (char*)topicFilter[i];
    }

    /* Create new buffer. */
    head_size = MQTTPacket_len(MQTTSerialize_unsubscribeLength(count, topic_ptr)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }
    
SEND_START:

    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

    packetid = getNextPacketId(c);
    if ((len = MQTTSerialize_unsubscribe(head_ptr, head_size, 0, packetid, 1, topic_ptr)) <= 0)
        goto exit;
    
    if ((rc = sendPacket(c, head_ptr, len, NULL, 0, timer)) != MQTTAsync_SUCCESS) // send the subscribe packet
        goto exit;

    if(isAsyncAck == 0)
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitfor(c, UNSUBACK, timer, &packet_ptr, &packetlen) == UNSUBACK)
        {
            unsigned short mypacketid;  // should be the same as the packetid above
            
            if (MQTTDeserialize_unsuback(&mypacketid, packet_ptr, packetlen) == 1)
            {
                /* Store msg handler. */
                for(i = 0; i < count; i++)
                {
                    /* remove the subscription message handler associated with this topic, if there is one */
                    MQTTAsyncSetMessageHandler(c, topicFilter[i], NULL, NULL);
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }
    else
    {
        unsigned char *packet_ptr = NULL;
        int             packetlen = 0;

        if (MQTTAsyncWaitFromBuffer(c, UNSUBACK, packetid, timer, &packet_ptr, &packetlen) == UNSUBACK)
        {
            unsigned short mypacketid;  // should be the same as the packetid above
            
            if (MQTTDeserialize_unsuback(&mypacketid, packet_ptr, packetlen) == 1)
            {
                /* Store msg handler. */
                for(i = 0; i < count; i++)
                {
                    /* remove the subscription message handler associated with this topic, if there is one */
                    MQTTAsyncSetMessageHandler(c, topicFilter[i], NULL, NULL);
                }
            }
        }
        else
        {
            if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                        goto SEND_START;       }
            else                      { rc = MQTTAsync_FAILURE; }
        }

        if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
    }

exit:

    if (rc == MQTTAsync_FAILURE) { MQTTAsyncCloseSession(c); }
	
	if(timer != NULL)            { c->plat_ptr->TimerDeinit(&timer);    timer = NULL; }
    if(topic_ptr != NULL)        { c->plat_ptr->MemFree(topic_ptr); topic_ptr = NULL; }	
    if(head_ptr != NULL)         { c->plat_ptr->MemFree(head_ptr);   head_ptr = NULL; }

    c->plat_ptr->MutexUnlock(&(c->inner_mutex));
    return rc;
}


int MQTTAsyncPublish(MQTTAsyncClient* c, const char* topicName, MQTTAsyncMessage* message, unsigned char isAsyncAck)
{
    int                rc = MQTTAsync_FAILURE;
    MQTTAsyncTimer  timer = NULL;
    MQTTString      topic = MQTTString_initializer;
    int               len = 0;
    int               cnt = 0;

    unsigned char * head_ptr = NULL;
    int            head_size = 0;


    c->plat_ptr->MutexLock(&(c->inner_mutex));
    
	if (!c->isconnected)
		goto exit;

    c->plat_ptr->TimerInit(&timer);
    topic.cstring = (char *)topicName;

    /* Reset previous read buffer and create new one. Caution: Only make room for msg header. */
    head_size = MQTTPacket_len(MQTTSerialize_publishLength(message->qos, topic, 0)) + 10;
    if((head_ptr = (unsigned char*)c->plat_ptr->MemCalloc(1, head_size)) == NULL)
    {
        rc = MQTTAsync_NOMEM;
        goto exit;
    }
    
SEND_START:
    
    c->plat_ptr->TimerCountdownMS(timer, c->initcfg_ptr->cmdTimeoutMs);

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerializeHeader_publish(head_ptr, head_size, 0, message->qos, message->retained, message->id, topic, message->payloadlen);
    if (len <= 0)
        goto exit;

    if ((rc = sendPacket(c, head_ptr, len, (unsigned char*)message->payload, message->payloadlen, timer)) != MQTTAsync_SUCCESS)
        goto exit;

    if (message->qos == QOS1)
    {
        if(isAsyncAck == 0)
        {
            unsigned char *packet_ptr = NULL;
            int             packetlen = 0;

            /* Waitting for specific packet and get packet data. */
            if (MQTTAsyncWaitfor(c, PUBACK, timer, &packet_ptr, &packetlen) == PUBACK)
            {
                unsigned short mypacketid = 0;
                unsigned char         dup = 0;
                unsigned char        type = 0;
                
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, packet_ptr, packetlen) != 1)
                    rc = MQTTAsync_FAILURE;
            }
            else
            {
                if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                            goto SEND_START;       }
                else                      { rc = MQTTAsync_FAILURE; }
            }
            
            if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
        }
        else
        {
            unsigned char *packet_ptr = NULL;
            int             packetlen = 0;

            /* Waitting for specific packet from stored buffer and get packet data. */
            if (MQTTAsyncWaitFromBuffer(c, PUBACK, message->id, timer, &packet_ptr, &packetlen) == PUBACK)
            {
                unsigned short mypacketid = 0;
                unsigned char         dup = 0;
                unsigned char        type = 0;
                
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, packet_ptr, packetlen) != 1)
                    rc = MQTTAsync_FAILURE;
            }
            else
            {
                if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                            goto SEND_START;       }
                else                      { rc = MQTTAsync_FAILURE; }
            }
            
            if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
        }
    }
    else if (message->qos == QOS2)
    {
        if(isAsyncAck == 0)
        {
            unsigned char *packet_ptr = NULL;
            int             packetlen = 0;
        
            if (MQTTAsyncWaitfor(c, PUBCOMP, timer, &packet_ptr, &packetlen) == PUBCOMP)
            {
                unsigned short mypacketid = 0;
                unsigned char         dup = 0;
                unsigned char        type = 0;
                
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, packet_ptr, packetlen) != 1)
                    rc = MQTTAsync_FAILURE;
            }
            else
            {
                if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                            goto SEND_START;       }
                else                      { rc = MQTTAsync_FAILURE; }
            }

            if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
        }
        else
        {
            unsigned char *packet_ptr = NULL;
            int             packetlen = 0;
        
            if (MQTTAsyncWaitFromBuffer(c, PUBCOMP, message->id, timer, &packet_ptr, &packetlen) == PUBCOMP)
            {
                unsigned short mypacketid = 0;
                unsigned char         dup = 0;
                unsigned char        type = 0;
                
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, packet_ptr, packetlen) != 1)
                    rc = MQTTAsync_FAILURE;
            }
            else
            {
                if((cnt++) <= c->initcfg_ptr->tryCnt) { if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
                                            goto SEND_START;       }
                else                      { rc = MQTTAsync_FAILURE; }
            }

            if(packet_ptr != NULL) { c->plat_ptr->MemFree(packet_ptr); packet_ptr = NULL; }
        }
    }

exit:

    if(rc == MQTTAsync_FAILURE) { MQTTAsyncCloseSession(c); }
	if(timer != NULL)           { c->plat_ptr->TimerDeinit(&timer);  timer = NULL; }
	if(head_ptr != NULL)        { c->plat_ptr->MemFree(head_ptr); head_ptr = NULL; }
    
    c->plat_ptr->MutexUnlock(&(c->inner_mutex));
    return rc;
}


/* Get keepalive left time in ms. */
int MQTTAsyncKeppaliveLeftMS(MQTTAsyncClient *c)
{
	return c->plat_ptr->TimerLeftMS(c->last_sent);
}


/* Set mqtt's no pong state. */
void MQTTAsyncSetNopongStat(MQTTAsyncClient *c, int isNopong)
{
	c->nopong_stat = (isNopong != 0)? 1 : 0;
}


int MQTTAsyncSendHeartBeat(MQTTAsyncClient* c, unsigned int timeout_ms)
{
#define HEARTBEAT_DELAYMS   200
    int                rc = MQTTAsync_SUCCESS;
    int               len = 0;
    MQTTAsyncTimer  timer = NULL;
    unsigned int time_tmp = 0;

    unsigned char head[10] = { 0 };


    /* Set ping flag. */
    c->ping_outstanding ++;
    c->plat_ptr->TimerInit(&timer);

    while(1)
    {
        c->plat_ptr->TimerCountdownMS(timer, 1000);
        len = MQTTSerialize_pingreq(head, sizeof(head));

        /* Out when sending failure. */
        if((rc = sendPacket(c, head, len, NULL, 0, timer)) != MQTTAsync_SUCCESS)
        {
            goto exit;
        }

        /* Out when no need to wait pong frame. */
        if(timeout_ms == 0)
        {
            goto exit;
        }
        else
        {
            c->plat_ptr->DelayMs(HEARTBEAT_DELAYMS);
            time_tmp += HEARTBEAT_DELAYMS;

            /* Out success when have received pong frame. */
            if(c->ping_outstanding == 0)
            {
                rc = MQTTAsync_SUCCESS;
                goto exit;
            }

            /* Out failure when waitting for pong frame overtime. */
            if(timeout_ms <= time_tmp)
            {
                rc = MQTTAsync_FAILURE;
                goto exit;
            } 
        }
    }

exit:

    if(timer != NULL) { c->plat_ptr->TimerDeinit(&timer); timer = NULL; }
    return rc;
    
#undef HEARTBEAT_DELAYMS
}

