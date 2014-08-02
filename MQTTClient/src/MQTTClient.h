/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/
 
 /*
 
 TODO: 
 
 ensure publish packets are retried on reconnect
 
 */

#if !defined(MQTTCLIENT_H)
#define MQTTCLIENT_H

#include "FP.h"
#include "MQTTPacket.h"
#include "stdio.h"

namespace MQTT
{


enum QoS { QOS0, QOS1, QOS2 };

// all failure return codes must be negative
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };


struct Message
{
    enum QoS qos;
    bool retained;
    bool dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
};


struct MessageData
{
    MessageData(MQTTString &aTopicName, struct Message &aMessage)  : message(aMessage), topicName(aTopicName)
    { }
    
    struct Message &message;
    MQTTString &topicName;
};


class PacketId
{
public:
    PacketId()
    {
        next = 0;
    }
    
    int getNext()
    {
        return next = (next == MAX_PACKET_ID) ? 1 : ++next;
    }
   
private:
    static const int MAX_PACKET_ID = 65535;
    int next;
};


class QoS2
{
public:

    
private:


};
  
  
/**
 * @class Client
 * @brief blocking, non-threaded MQTT client API
 * 
 * This version of the API blocks on all method calls, until they are complete.  This means that only one
 * MQTT request can be in process at any one time.  
 * @param Network a network class which supports send, receive
 * @param Timer a timer class with the methods: 
 */ 
template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE = 100, int MAX_MESSAGE_HANDLERS = 5>
class Client
{
    
public:
   
    typedef void (*messageHandler)(MessageData&);

    /** Construct the client
     *  @param network - pointer to an instance of the Network class - must be connected to the endpoint
     *      before calling MQTT connect
     *  @param limits an instance of the Limit class - to alter limits as required
     */
    Client(Network& network, unsigned int command_timeout_ms = 30000); 
    
    /** Set the default message handling callback - used for any message which does not match a subscription message handler
     *  @param mh - pointer to the callback function
     */
    void setDefaultMessageHandler(messageHandler mh)
    {
        defaultMessageHandler.attach(mh);
    }
    
    /** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
     *  The nework object must be connected to the network endpoint before calling this 
     *  @param options - connect options
     *  @return success code -  
     */       
    int connect(MQTTPacket_connectData* options = 0);
      
    /** MQTT Publish - send an MQTT publish packet and wait for all acks to complete for all QoSs
     *  @param topic - the topic to publish to
     *  @param message - the message to send
     *  @return success code -  
     */      
    int publish(const char* topicName, Message* message);
   
    /** MQTT Subscribe - send an MQTT subscribe packet and wait for the suback
     *  @param topicFilter - a topic pattern which can include wildcards
     *  @param qos - the MQTT QoS to subscribe at
     *  @param mh - the callback function to be invoked when a message is received for this subscription
     *  @return success code -  
     */   
    int subscribe(const char* topicFilter, enum QoS qos, messageHandler mh);
    
    /** MQTT Unsubscribe - send an MQTT unsubscribe packet and wait for the unsuback
     *  @param topicFilter - a topic pattern which can include wildcards
     *  @return success code -  
     */   
    int unsubscribe(const char* topicFilter);
    
    /** MQTT Disconnect - send an MQTT disconnect packet, and clean up any state
     *  @return success code -  
     */
    int disconnect();
    
    /** A call to this API must be made within the keepAlive interval to keep the MQTT connection alive
     *  yield can be called if no other MQTT operation is needed.  This will also allow messages to be 
     *  received.
     *  @param timeout_ms the time to wait, in milliseconds
     *  @return success code - on failure, this means the client has disconnected
     */
    int yield(int timeout_ms = 1000);
    
private:

    int cycle(Timer& timer);
    int waitfor(int packet_type, Timer& timer);
    int keepalive();

    int decodePacket(int* value, int timeout);
    int readPacket(Timer& timer);
    int sendPacket(int length, Timer& timer);
    int deliverMessage(MQTTString& topicName, Message& message);
    bool isTopicMatched(char* topicFilter, MQTTString& topicName);
    
    Network& ipstack;
    unsigned int command_timeout_ms;
    
    unsigned char buf[MAX_MQTT_PACKET_SIZE];  
    unsigned char readbuf[MAX_MQTT_PACKET_SIZE];  

    Timer ping_timer;
    unsigned int keepAliveInterval;
    bool ping_outstanding;
    
    PacketId packetid;
    
    struct MessageHandlers
    {
        const char* topicFilter;
        FP<void, MessageData&> fp;
    } messageHandlers[MAX_MESSAGE_HANDLERS];      // Message handlers are indexed by subscription topic
    
    FP<void, MessageData&> defaultMessageHandler;
     
    bool isconnected;
    
#if 0
    struct
    {
      bool used;
      int id;  
    } QoS2messages[MAX_QOS2_MESSAGES];
    
#endif

};

}


template<class Network, class Timer, int a, int MAX_MESSAGE_HANDLERS> 
MQTT::Client<Network, Timer, a, MAX_MESSAGE_HANDLERS>::Client(Network& network, unsigned int command_timeout_ms)  : ipstack(network), packetid()
{
    ping_timer = Timer();
    ping_outstanding = 0;
    for (int i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        messageHandlers[i].topicFilter = 0;
    this->command_timeout_ms = command_timeout_ms; 
    isconnected = false;
}


template<class Network, class Timer, int a, int b> 
int MQTT::Client<Network, Timer, a, b>::sendPacket(int length, Timer& timer)
{
    int rc = FAILURE, 
        sent = 0;
    
    while (sent < length && !timer.expired())
    {
        rc = ipstack.write(&buf[sent], length, timer.left_ms());
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }
    if (sent == length)
    {
        ping_timer.countdown(this->keepAliveInterval); // record the fact that we have successfully sent the packet    
        rc = SUCCESS;
    }
    else
        rc = FAILURE;
    return rc;
}


template<class Network, class Timer, int a, int b> 
int MQTT::Client<Network, Timer, a, b>::decodePacket(int* value, int timeout)
{
    unsigned char c;
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
        rc = ipstack.read(&c, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);
exit:
    return len;
}


/**
 * If any read fails in this method, then we should disconnect from the network, as on reconnect
 * the packets can be retried. 
 * @param timeout the max time to wait for the packet read to complete, in milliseconds
 * @return the MQTT packet type, or -1 if none
 */
template<class Network, class Timer, int a, int b> 
int MQTT::Client<Network, Timer, a, b>::readPacket(Timer& timer) 
{
    int rc = FAILURE;
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    if (ipstack.read(readbuf, 1, timer.left_ms()) != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    decodePacket(&rem_len, timer.left_ms());
    len += MQTTPacket_encode(readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (ipstack.read(readbuf + len, rem_len, timer.left_ms()) != rem_len)
        goto exit;

    header.byte = readbuf[0];
    rc = header.bits.type;
exit:
    return rc;
}


// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
template<class Network, class Timer, int a, int b> 
bool MQTT::Client<Network, Timer, a, b>::isTopicMatched(char* topicFilter, MQTTString& topicName)
{
    char* curf = topicFilter;
    char* curn = topicName.lenstring.data;
    char* curn_end = curn + topicName.lenstring.len;
    
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



template<class Network, class Timer, int a, int MAX_MESSAGE_HANDLERS> 
int MQTT::Client<Network, Timer, a, MAX_MESSAGE_HANDLERS>::deliverMessage(MQTTString& topicName, Message& message)
{
    int rc = FAILURE;

    // we have to find the right message handler - indexed by topic
    for (int i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(&topicName, (char*)messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)messageHandlers[i].topicFilter, topicName)))
        {
            if (messageHandlers[i].fp.attached())
            {
                MessageData md(topicName, message);
                messageHandlers[i].fp(md);
                rc = SUCCESS;
            }
        }
    }
    
    if (rc == FAILURE && defaultMessageHandler.attached()) 
    {
        MessageData md(topicName, message);
        defaultMessageHandler(md);
        rc = SUCCESS;
    }   
    
    return rc;
}



template<class Network, class Timer, int a, int b> 
int MQTT::Client<Network, Timer, a, b>::yield(int timeout_ms)
{
    int rc = SUCCESS;
    Timer timer = Timer();
    
    timer.countdown_ms(timeout_ms);
    while (!timer.expired())
    {
        if (cycle(timer) == FAILURE)
        {
            rc = FAILURE;
            break;
        }
    }
        
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::cycle(Timer& timer)
{
    /* get one piece of work off the wire and one pass through */

    // read the socket, see what work is due
    unsigned short packet_type = readPacket(timer);
    
    int len = 0,
        rc = SUCCESS;

    switch (packet_type)
    {
        case CONNACK:
        case PUBACK:
        case SUBACK:
            break;
        case PUBLISH:
            MQTTString topicName;
            Message msg;
            if (MQTTDeserialize_publish((unsigned char*)&msg.dup, (int*)&msg.qos, (unsigned char*)&msg.retained, (unsigned short*)&msg.id, &topicName,
                                 (unsigned char**)&msg.payload, (int*)&msg.payloadlen, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                goto exit;
//          if (msg.qos != QOS2) 
                deliverMessage(topicName, msg);
#if 0
            else if (isQoS2msgidFree(msg.id))
            {
                UseQoS2msgid(msg.id);
                deliverMessage(topicName, msg);
            }
#endif
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(buf, MAX_MQTT_PACKET_SIZE, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(buf, MAX_MQTT_PACKET_SIZE, PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = FAILURE;
                else
                    rc = sendPacket(len, timer);
                if (rc == FAILURE)
                    goto exit; // there was a problem
            }
            break;
        case PUBREC:
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(buf, MAX_MQTT_PACKET_SIZE, PUBREL, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = sendPacket(len, timer)) != SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            break;
        case PUBCOMP:
            break;
        case PINGRESP:
            ping_outstanding = false;
            break;
    }
    keepalive();
exit:
    if (rc == SUCCESS)
        rc = packet_type;
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b>
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::keepalive()
{
    int rc = FAILURE;

    if (keepAliveInterval == 0)
    {
        rc = SUCCESS;
        goto exit;
    }

    if (ping_timer.expired())
    {
        if (!ping_outstanding)
        {
            Timer timer = Timer(1000);
            int len = MQTTSerialize_pingreq(buf, MAX_MQTT_PACKET_SIZE);
            if (len > 0 && (rc = sendPacket(len, timer)) == SUCCESS) // send the ping packet
                ping_outstanding = true;
        }
    }

exit:
    return rc;
}


// only used in single-threaded mode where one command at a time is in process
template<class Network, class Timer, int a, int b> 
int MQTT::Client<Network, Timer, a, b>::waitfor(int packet_type, Timer& timer)
{
    int rc = FAILURE;
    
    do
    {
        if (timer.expired()) 
            break; // we timed out
    }
    while ((rc = cycle(timer)) != packet_type);  
    
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::connect(MQTTPacket_connectData* options)
{
    Timer connect_timer = Timer(command_timeout_ms);
    int rc = FAILURE;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;
    
    if (isconnected) // don't send connect packet again if we are already connected
        goto exit;

    if (options == 0)
        options = &default_options; // set default options if none were supplied
    
    this->keepAliveInterval = options->keepAliveInterval;
    ping_timer.countdown(this->keepAliveInterval);
    if ((len = MQTTSerialize_connect(buf, MAX_MQTT_PACKET_SIZE, options)) <= 0)
        goto exit;
    if ((rc = sendPacket(len, connect_timer)) != SUCCESS)  // send the connect packet
        goto exit; // there was a problem
    
    // this will be a blocking call, wait for the connack
    if (waitfor(CONNACK, connect_timer) == CONNACK)
    {
        unsigned char connack_rc = 255;
        bool sessionPresent = false;
        if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, readbuf, MAX_MQTT_PACKET_SIZE) == 1)
            rc = connack_rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;
    
exit:
    if (rc == SUCCESS)
        isconnected = true;
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int MAX_MESSAGE_HANDLERS> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, MAX_MESSAGE_HANDLERS>::subscribe(const char* topicFilter, enum QoS qos, messageHandler messageHandler)
{ 
    int rc = FAILURE;  
    Timer timer = Timer(command_timeout_ms);
    int len = 0;
    MQTTString topic = {(char*)topicFilter, 0, 0};
    
    if (!isconnected)
        goto exit;
    
    len = MQTTSerialize_subscribe(buf, MAX_MQTT_PACKET_SIZE, 0, packetid.getNext(), 1, &topic, (int*)&qos);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(len, timer)) != SUCCESS) // send the subscribe packet
        goto exit;             // there was a problem
    
    if (waitfor(SUBACK, timer) == SUBACK)      // wait for suback 
    {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, readbuf, MAX_MQTT_PACKET_SIZE) == 1)
            rc = grantedQoS; // 0, 1, 2 or 0x80 
        if (rc != 0x80)
        {
            for (int i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
            {
                if (messageHandlers[i].topicFilter == 0)
                {
                    messageHandlers[i].topicFilter = topicFilter;
                    messageHandlers[i].fp.attach(messageHandler);
                    rc = 0;
                    break;
                }
            }
        }
    }
    else 
        rc = FAILURE;
        
exit:
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int MAX_MESSAGE_HANDLERS> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, MAX_MESSAGE_HANDLERS>::unsubscribe(const char* topicFilter)
{   
    int rc = FAILURE;
    Timer timer = Timer(command_timeout_ms);    
    MQTTString topic = {(char*)topicFilter, 0, 0};
    int len = 0;
    
    if (!isconnected)
        goto exit;
    
    if ((len = MQTTSerialize_unsubscribe(buf, MAX_MQTT_PACKET_SIZE, 0, packetid.getNext(), 1, &topic)) <= 0)
        goto exit;
    if ((rc = sendPacket(len, timer)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem
    
    if (waitfor(UNSUBACK, timer) == UNSUBACK)
    {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, readbuf, MAX_MQTT_PACKET_SIZE) == 1)
            rc = 0; 
    }
    else
        rc = FAILURE;
    
exit:
    return rc;
}


   
template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::publish(const char* topicName, Message* message)
{
    int rc = FAILURE;
    Timer timer = Timer(command_timeout_ms);   
    MQTTString topicString = {(char*)topicName, 0, 0};
    int len = 0;
    
    if (!isconnected)
        goto exit;

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = packetid.getNext();
    
    len = MQTTSerialize_publish(buf, MAX_MQTT_PACKET_SIZE, 0, message->qos, message->retained, message->id, 
              topicString, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(len, timer)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem
    
    if (message->qos == QOS1)
    {
        if (waitfor(PUBACK, timer) == PUBACK)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }
    else if (message->qos == QOS2)
    {
        if (waitfor(PUBCOMP, timer) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }
    
exit:
    return rc;
}


template<class Network, class Timer, int MAX_MQTT_PACKET_SIZE, int b> 
int MQTT::Client<Network, Timer, MAX_MQTT_PACKET_SIZE, b>::disconnect()
{  
    int rc = FAILURE;
    Timer timer = Timer(command_timeout_ms);     // we might wait for incomplete incoming publishes to complete
    int len = MQTTSerialize_disconnect(buf, MAX_MQTT_PACKET_SIZE);
    if (len > 0)
        rc = sendPacket(len, timer);            // send the disconnect packet
        
    isconnected = false;
    return rc;
}


#endif
