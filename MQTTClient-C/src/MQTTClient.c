/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
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
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/
#include "MQTTClient.h"

static void NewMessageData(MessageData *md, MQTTString *aTopicName, MQTTMessage *aMessage)
{
   md->topicName = aTopicName;
   md->message = aMessage;
}

static int getNextPacketId(MQTTClient *c)
{
   return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}

static int sendPacket(MQTTClient *c, int length, Timer *timer)
{
   int rc = FAILURE,
       sent = 0;

#if defined(MQTT_TASK)
   MutexLock(&c->write_mutex);
#endif
   while (sent < length && !TimerIsExpired(timer))
   {
      rc = c->ipstack->mqttwrite(c->ipstack, &c->buf[sent], length, TimerLeftMS(timer));
      if (rc < 0)
         break;
      sent += rc;
   }
#if defined(MQTT_TASK)
   MutexUnlock(&c->write_mutex);
#endif

   if (sent == length)
   {
      TimerCountdown(&c->ping_timer, c->keepAliveInterval); // record the fact that we have successfully sent the packet
      rc = SUCCESS;
   }
   else
      rc = FAILURE;

   return rc;
}

void MQTTClientInit(MQTTClient *c, Network *network, unsigned int command_timeout_ms,
                    unsigned char *sendbuf, size_t sendbuf_size, unsigned char *readbuf, size_t readbuf_size)
{
   int i;
   c->ipstack = network;

   for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
      c->messageHandlers[i].topicFilter = 0;
   c->command_timeout_ms = command_timeout_ms;
   c->buf = sendbuf;
   c->buf_size = sendbuf_size;
   c->readbuf = readbuf;
   c->readbuf_size = readbuf_size;
   c->isconnected = 0;
   c->ping_outstanding = 0;
   c->defaultMessageHandler = NULL;
   c->next_packetid = 1;
   TimerInit(&c->ping_timer);
#if defined(MQTT_TASK)
   MutexInit(&c->reply_mutex);
   ConditionInit(&c->has_reply);
   MutexInit(&c->write_mutex);
#endif
}

void MQTTClientDestroy(MQTTClient *c)
{
   ThreadJoin(&c->read_thread);
   MutexDestroy(&c->write_mutex);
   ConditionDestroy(&c->has_reply);
   MutexDestroy(&c->reply_mutex);
}

static int decodePacket(MQTTClient *c, int *value, int timeout)
{
   unsigned char i;
   int multiplier = 1;
   int len = 0;
   int rc = SUCCESS;
   const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

   *value = 0;
   do
   {
      if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
      {
         rc = FAILURE;
         goto exit;
      }
      if (c->ipstack->mqttread(c->ipstack, &i, 1, timeout) != 1)
         goto exit;
      *value += (i & 127) * multiplier;
      multiplier *= 128;
   } while ((i & 128) != 0);

exit:
   return rc;
}

static int readPacket(MQTTClient *c, Timer *timer)
{
   int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1, TimerLeftMS(timer));
   if (rc != 1)
      goto exit;

   int rem_len = 0;

   rc = decodePacket(c, &rem_len, TimerLeftMS(timer));
   if (rc != SUCCESS)
      goto exit;

   int len = 1 + MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

   if (rem_len > 0 && (rc = c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len))
      goto exit;

   MQTTHeader header = {0};
   header.byte = c->readbuf[0];
   rc = header.bits.type;

exit:
   return rc;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char *topicFilter, MQTTString *topicName)
{
   char *curf = topicFilter;
   char *curn = topicName->lenstring.data;
   char *curn_end = curn + topicName->lenstring.len;

   while (*curf && curn < curn_end)
   {
      if (*curn == '/' && *curf != '/')
         break;
      if (*curf != '+' && *curf != '#' && *curf != *curn)
         break;
      if (*curf == '+')
      {
         char *nextpos = curn + 1;
         while (nextpos < curn_end && *nextpos != '/')
            nextpos = ++curn + 1;
      }
      else if (*curf == '#')
         curn = curn_end - 1;
      curf++;
      curn++;
   };

   return (curn == curn_end) && (*curf == '\0');
}

int deliverMessage(MQTTClient *c, MQTTString *topicName, MQTTMessage *message)
{
   int i;
   int rc = FAILURE;

   // we have to find the right message handler - indexed by topic
   for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
   {
      if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char *)c->messageHandlers[i].topicFilter) ||
                                                     isTopicMatched((char *)c->messageHandlers[i].topicFilter, topicName)))
      {
         if (c->messageHandlers[i].fp != NULL)
         {
            MessageData md;
            NewMessageData(&md, topicName, message);
            c->messageHandlers[i].fp(&md, c->messageHandlers[i].arg);
            rc = SUCCESS;
         }
      }
   }

   if (rc == FAILURE && c->defaultMessageHandler != NULL)
   {
      MessageData md;
      NewMessageData(&md, topicName, message);
      c->defaultMessageHandler(&md);
      rc = SUCCESS;
   }

   return rc;
}

int keepalive(MQTTClient *c)
{
   int rc = FAILURE;

   if (c->keepAliveInterval == 0)
   {
      rc = SUCCESS;
      goto exit;
   }

   if (TimerIsExpired(&c->ping_timer))
   {
      if (!c->ping_outstanding)
      {
         Timer timer;
         TimerInit(&timer);
         TimerCountdownMS(&timer, c->command_timeout_ms);
         int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
         if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
            c->ping_outstanding = 1;
      }
   }

exit:
   return rc;
}

int cycle(MQTTClient *c, Timer *timer)
{
   int rc = readPacket(c, timer);
   if (rc < 0)
      return rc;
   if (rc == 0)
      return FAILURE; // 0 is no more data to read, treat as general failure

   unsigned short packet_type = (unsigned short)rc;
   rc = SUCCESS;

   int len = 0;
   switch (packet_type)
   {
   case CONNACK:
   case PUBACK:
   case SUBACK:
      break;

   case PUBLISH:
   {
      MQTTString topicName;
      MQTTMessage msg;
      int intQoS;
      if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                                  (unsigned char **)&msg.payload, (int *)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
         goto exit;
      msg.qos = (enum QoS)intQoS;
      deliverMessage(c, &topicName, &msg);
      if (msg.qos != QOS0)
      {
         if (msg.qos == QOS1)
            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
         else if (msg.qos == QOS2)
            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
         if (len <= 0)
            rc = FAILURE;
         else
         {
            TimerCountdownMS(timer, c->command_timeout_ms);
            rc = sendPacket(c, len, timer);
         }
         if (rc == FAILURE)
            goto exit;
      }
      break;
   }

   case PUBREC:
   {
      unsigned short mypacketid;
      unsigned char dup, type;
      if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
         rc = FAILURE;
      else if ((len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREL, 0, mypacketid)) <= 0)
         rc = FAILURE;
      else
      {
         TimerCountdownMS(timer, c->command_timeout_ms);
         rc = sendPacket(c, len, timer);
      }
      if (rc == FAILURE)
         goto exit;
      break;
   }

   case PUBCOMP:
      break;

   case PINGRESP:
      c->ping_outstanding = 0;
      break;
   }

exit:
   if (rc == SUCCESS)
      rc = packet_type;
   return rc;
}

#if !defined(MQTT_TASK)
int MQTTYield(MQTTClient *c, int timeout_ms)
{
   int rc = SUCCESS;

   Timer timer;
   TimerInit(&timer);
   TimerCountdownMS(&timer, timeout_ms);

   do
   {
      if (cycle(c, &timer) == FAILURE)
      {
         rc = FAILURE;
         break;
      }
      keepalive(c);
   } while (!TimerIsExpired(&timer));

   return rc;
}
#endif

#if defined(MQTT_TASK)
void MQTTRead(void *arg)
{
   MQTTClient *c = (MQTTClient *)arg;

   Timer timer;
   TimerInit(&timer);

   while (c->isconnected)
   {
      TimerCountdownMS(&timer, 1000);
      int rc = cycle(c, &timer);
      switch (rc)
      {
      case TIMEOUT:
         break;

      case FAILURE:
         goto exit;

      default:
      {
         unsigned short packet_type = (unsigned short)rc;
         switch (packet_type)
         {
         case CONNACK:
         case PUBACK:
         case SUBACK:
         case PUBCOMP:
            MutexLock(&c->reply_mutex);
            while (c->reply != 0)
               ConditionWait(&c->has_reply, &c->reply_mutex);
            c->reply = packet_type;
            MutexUnlock(&c->reply_mutex);
            ConditionSignal(&c->has_reply);
            break;
         }
         break;
      }
      }

      keepalive(c);
   }

exit:
   printf("Read thread exiting\n");
   c->isconnected = 0;
   ThreadExit();
}
#endif

int waitfor(MQTTClient *c, unsigned short packet_type, Timer *timer)
{
   int rc = FAILURE;

   do
   {
      if (TimerIsExpired(timer))
      {
         rc = FAILURE;
         break;
      }
#if defined(MQTT_TASK)
      MutexLock(&c->reply_mutex);
      while (c->reply == 0)
         ConditionWait(&c->has_reply, &c->reply_mutex);
      rc = c->reply;
      c->reply = 0;
      MutexUnlock(&c->reply_mutex);
      ConditionSignal(&c->has_reply);
   } while (rc != packet_type);
#elif
   } while ((rc = cycle(c, timer)) != packet_type);
#endif

   return rc;
}

int MQTTConnect(MQTTClient *c, MQTTPacket_connectData *options)
{
   int rc = FAILURE;

   if (c->isconnected)
      goto exit;

   Timer connect_timer;
   TimerInit(&connect_timer);
   TimerCountdownMS(&connect_timer, c->command_timeout_ms);

   MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
   if (options == NULL)
      options = &default_options;

   c->keepAliveInterval = options->keepAliveInterval;
   TimerCountdown(&c->ping_timer, c->keepAliveInterval);
   int len;
   if ((len = MQTTSerialize_connect(c->buf, c->buf_size, options)) <= 0)
      goto exit;
   if ((rc = sendPacket(c, len, &connect_timer)) != SUCCESS)
      goto exit;

   c->isconnected = 1;
   ThreadStart(&c->read_thread, &MQTTRead, c);

   if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
   {
      unsigned char connack_rc = 255;
      unsigned char sessionPresent = 0;
      if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, c->readbuf, c->readbuf_size) == 1)
         rc = connack_rc;
      else
         rc = FAILURE;
   }
   else
      rc = FAILURE;

exit:
   if (rc == FAILURE)
   {
      c->isconnected = 0;
      ThreadJoin(&c->read_thread);
   }

   return rc;
}

int MQTTSubscribe(MQTTClient *c, const char *topicFilter, enum QoS qos, messageHandler messageHandler, void *arg)
{
   int rc = FAILURE;
   Timer timer;
   int len = 0;
   MQTTString topic = MQTTString_initializer;
   topic.cstring = (char *)topicFilter;

   if (!c->isconnected)
      goto exit;

   TimerInit(&timer);
   TimerCountdownMS(&timer, c->command_timeout_ms);

   len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic, (int *)&qos);
   if (len <= 0)
      goto exit;
   if ((rc = sendPacket(c, len, &timer)) != SUCCESS)
      goto exit;

   if (waitfor(c, SUBACK, &timer) == SUBACK) // wait for suback
   {
      int count = 0, grantedQoS = -1;
      unsigned short mypacketid;
      if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size) == 1)
         rc = grantedQoS; // 0, 1, 2 or 0x80
      if (rc != 0x80)
      {
         int i;
         for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
         {
            if (c->messageHandlers[i].topicFilter == NULL)
            {
               c->messageHandlers[i].arg = arg;
               c->messageHandlers[i].topicFilter = topicFilter;
               c->messageHandlers[i].fp = messageHandler;
               rc = SUCCESS;
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

int MQTTUnsubscribe(MQTTClient *c, const char *topicFilter)
{
   int rc = FAILURE;
   Timer timer;
   MQTTString topic = MQTTString_initializer;
   topic.cstring = (char *)topicFilter;
   int len = 0;

   if (!c->isconnected)
      goto exit;

   TimerInit(&timer);
   TimerCountdownMS(&timer, c->command_timeout_ms);

   if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
      goto exit;
   if ((rc = sendPacket(c, len, &timer)) != SUCCESS)
      goto exit;

   if (waitfor(c, UNSUBACK, &timer) == UNSUBACK)
   {
      unsigned short mypacketid;
      if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1)
         rc = 0;
   }
   else
      rc = FAILURE;

exit:
   return rc;
}

int MQTTPublish(MQTTClient *c, const char *topicName, MQTTMessage *message)
{
   int rc = FAILURE;
   Timer timer;
   MQTTString topic = MQTTString_initializer;
   topic.cstring = (char *)topicName;
   int len = 0;

   if (!c->isconnected)
      goto exit;

   TimerInit(&timer);
   TimerCountdownMS(&timer, c->command_timeout_ms);

   if (message->qos == QOS1 || message->qos == QOS2)
      message->id = getNextPacketId(c);

   len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
                               topic, (unsigned char *)message->payload, message->payloadlen);
   if (len <= 0)
      goto exit;
   if ((rc = sendPacket(c, len, &timer)) != SUCCESS)
      goto exit;

   if (message->qos == QOS1)
   {
      if (waitfor(c, PUBACK, &timer) == PUBACK)
      {
         unsigned short mypacketid;
         unsigned char dup, type;
         if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
            rc = FAILURE;
      }
      else
         rc = FAILURE;
   }
   else if (message->qos == QOS2)
   {
      if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
      {
         unsigned short mypacketid;
         unsigned char dup, type;
         if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
            rc = FAILURE;
      }
      else
         rc = FAILURE;
   }

exit:
   return rc;
}

int MQTTDisconnect(MQTTClient *c)
{
   int rc = FAILURE;
   Timer timer; // we might wait for incomplete incoming publishes to complete
   int len = 0;

   TimerInit(&timer);
   TimerCountdownMS(&timer, c->command_timeout_ms);

   len = MQTTSerialize_disconnect(c->buf, c->buf_size);
   if (len > 0)
      rc = sendPacket(c, len, &timer); // send the disconnect packet

   c->isconnected = 0;

   return rc;
}
