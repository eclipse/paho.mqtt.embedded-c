/*******************************************************************************
 * Copyright (c) 2023 Microsoft Corporation. All rights reserved.
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
 *******************************************************************************/

#if !defined(MQTTV5_CLIENT_H)
#define MQTTV5_CLIENT_H

#if defined(__cplusplus)
 extern "C" {
#endif

#include "../MQTTClient.h"
#include "V5/MQTTV5Packet.h"


typedef struct MQTTV5UnsubackData
{
    enum MQTTReasonCodes reasonCode;
    MQTTProperties* properties;
} MQTTV5UnsubackData;

/**
 * @brief Create an MQTT client object
 * 
 * @param client 
 * @param network 
 * @param command_timeout_ms 
 * @param sendbuf 
 * @param sendbuf_size 
 * @param readbuf 
 * @param readbuf_size 
 * @return DLLExport 
 */
DLLExport void MQTTV5ClientInit(MQTTClient* client, Network* network, unsigned int command_timeout_ms,
		unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size, 
    MQTTProperties* recvProperties);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The network object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @return success code
 */
DLLExport int MQTTV5ConnectWithResults(MQTTClient* client, MQTTPacket_connectData* options,
    MQTTProperties* connectProperties, MQTTProperties* willProperties, MQTTConnackData* data);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The nework object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @return success code
 */
DLLExport int MQTTV5Connect(MQTTClient* client, MQTTPacket_connectData* options, 
  MQTTProperties* connectProperties, MQTTProperties* willProperties);

/** MQTT Publish - send an MQTT publish packet and wait for all acks to complete for all QoSs
 *  @param client - the client object to use
 *  @param topic - the topic to publish to
 *  @param message - the message to send
 *  @return success code
 */
DLLExport int MQTTV5Publish(MQTTClient* client, const char* topic, MQTTMessage* message, 
  MQTTProperties* properties);


DLLExport int MQTTV5PublishWithResults(MQTTClient* client, const char* topic, MQTTMessage* message, 
  MQTTProperties* properties, MQTTPubDoneData* ack);

/** MQTT SetMessageHandler - set or remove a per topic message handler
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter set the message handler for
 *  @param messageHandler - pointer to the message handler function or NULL to remove
 *  @return success code
 */
DLLExport int MQTTV5SetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler);

/**
 * @brief MQTT Auth - send an MQTT AUTH packet
 * 
 * @param client - the client object to use
 * @param reasonCode - the reason code to send
 * @param properties - the properties to send
 * @return success code
 */
DLLExport int MQTTV5Auth(MQTTClient* client, unsigned char reasonCode, MQTTProperties* properties);

DLLExport int MQTTV5SetAuthHandler(MQTTClient* c, controlHandler authHandler);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @return success code
 */
DLLExport int MQTTV5Subscribe(MQTTClient* client, const char* topicFilter, enum MQTTQoS qos, 
  messageHandler messageHandler, MQTTProperties* properties, MQTTSubscribe_options options);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @param data - suback granted QoS returned
 *  @return success code
 */
DLLExport int MQTTV5SubscribeWithResults(MQTTClient* client, const char* topicFilter, 
  enum MQTTQoS qos, messageHandler messageHandler, MQTTSubackData* data);

/** MQTT Unsubscribe - send an MQTT unsubscribe packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @return success code
 */
DLLExport int MQTTV5Unsubscribe(MQTTClient* client, const char* topicFilter, MQTTProperties* properties);


DLLExport int MQTTV5UnsubscribeWithResults(MQTTClient* client, const char* topicFilter, MQTTProperties* properties, MQTTV5UnsubackData* data);

/** MQTT Disconnect - send an MQTT disconnect packet and close the connection
 *  @param client - the client object to use
 *  @return success code
 */
DLLExport int MQTTV5Disconnect(MQTTClient* client, unsigned char reasonCode, MQTTProperties* properties);

DLLExport int MQTTV5SetDisconnectHandler(MQTTClient* c, controlHandler disconnectHandler);

#if defined(__cplusplus)
     }
#endif

#endif // MQTTV5_CLIENT_H
