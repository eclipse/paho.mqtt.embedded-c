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

#include <stdbool.h>
#include "../MQTTClient.h"
#include "MQTTV5Packet.h"


/**
 * @brief Data structure containing MQTTv5 UNSUBACK information.
 * 
 */
typedef struct MQTTV5UnsubackData
{
    /// @brief The MQTTv5 message properties.
    MQTTProperties* properties;
    /// @brief The MQTTv5 reason code.
    enum MQTTReasonCodes reasonCode;
} MQTTV5UnsubackData;

/**
 * @brief Create an `MQTTClient` for an MQTTv5 connection.
 * 
 * @param client The `MQTTClient` object to initialize.
 * @param network The `Network` object to use. 
 * @param command_timeout_ms The command timeout value in milliseconds. 
 * @param sendbuf The send buffer.
 * @param sendbuf_size The size of the `sendbuf` buffer. 
 * @param readbuf The read buffer.
 * @param readbuf_size The size of the `readbuf` buffer.
 * @param recvProperties The MQTTv5 receive properties. This allocation will be used for all MQTTv5 packets.
 * @param truncateRecvProperties If true, the MQTTv5 properties will be truncated if they do not fit in the `recvProperties` buffer.
 * 
 * @remark If `recvProperties` does not contain sufficient space for the received properties and `truncateRecvProperties` is false, 
 *       `MQTTYield` will fail with `MQTTCLIENT_BUFFER_OVERFLOW`.
 * 
 * @remark If the `MQTTClient` is initialized with `MQTTV5ClientInit`, any of the non-versioned functions (e.g. `MQTTPublish`) will
 *       work as expected. However, v5 functions (e.g. `MQTTV5Publish`) must be used to access MQTTv5 features.
 */
DLLExport void MQTTV5ClientInit(MQTTClient* client, Network* network, unsigned int command_timeout_ms,
		unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size, 
    MQTTProperties* recvProperties, bool truncateRecvProperties);

/**
 * @brief MQTT Connect - send an MQTTv5 connect packet down the network and wait for a CONNACK.
 * @note The network object must be connected to the network endpoint before calling this.
 * 
 * @param client The `MQTTClient` object to use.
 * @param options The connect options.
 * @param connectProperties The MQTTv5 connect properties.
 * @param willProperties The MQTTv5 will properties.
 * @param connack CONNACK response information.
 * @return 0 on success. If negative, an #MQTTClientReturnCode indicating success or failure. 
 *         If positive, the `MQTTReasonCodes` returned by the server.
 */
DLLExport int MQTTV5ConnectWithResults(MQTTClient* client, MQTTPacket_connectData* options,
    MQTTProperties* connectProperties, MQTTProperties* willProperties, MQTTConnackData* connack);

/**
 * @brief MQTT Connect - send an MQTTv5 connect packet down the network and wait for a CONNACK.
 * @note The network object must be connected to the network endpoint before calling this.
 * 
 * @param client The `MQTTClient` object to use.
 * @param options The connect options.
 * @param connectProperties The MQTTv5 connect properties.
 * @param willProperties The MQTTv5 will properties.
 * @return 0 on success. If negative, an #MQTTClientReturnCode indicating success or failure. 
 *         If positive, the `MQTTReasonCodes` returned by the server.
 */
DLLExport int MQTTV5Connect(MQTTClient* client, MQTTPacket_connectData* options, 
  MQTTProperties* connectProperties, MQTTProperties* willProperties);

/**
 * @brief MQTT Publish - send an MQTTv5 publish packet and wait for all acks (PUBACK or PUBCOMP) to complete for all QoSs.
 * 
 * @param client The `MQTTClient` object to use. 
 * @param topicName The topic to publish to.
 * @param message The `MQTTMessage` message to send.
 * @param properties The MQTTv5 publish properties.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5Publish(MQTTClient* client, const char* topicName, MQTTMessage* message, 
  MQTTProperties* properties);

/**
 * @brief MQTT Publish - send an MQTTv5 publish packet and wait for all acks (PUBACK or PUBCOMP) to complete for all QoSs.
 * @note This function blocks until the QoS1 PUBACK or QoS2 PUBCOMP is received.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topic The topic to publish to.
 * @param message The message to send.
 * @param properties The MQTTv5 publish properties.
 * @param ack Acknowledgement information (from either a PUBACK or PUBCOMP message).
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5PublishWithResults(MQTTClient* client, const char* topic, MQTTMessage* message, 
  MQTTProperties* properties, MQTTPubDoneData* ack);

/**
 * @brief Set or remove a per topic message (Publish) receive handler. This works for MQTTv5 clients only.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topicFilter The topic filter for the message handler.
 * @param messageHandler The message handler function or NULL to remove.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5SetMessageHandler(MQTTClient* client, const char* topicFilter, messageHandler messageHandler);

/**
 * @brief MQTT AUTH - send an MQTTv5 AUTH packet.
 * 
 * @param client The `MQTTClient` object to use.
 * @param reasonCode The MQTTv5 reason code.
 * @param properties The MQTTv5 properties.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5Auth(MQTTClient* client, unsigned char reasonCode, MQTTProperties* properties);

/**
 * @brief Set or remove an MQTTv5 AUTH receive handler.
 * 
 * @param client The `MQTTClient` object to use.
 * @param authHandler The AUTH receive handler function or NULL to remove.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5SetAuthHandler(MQTTClient* client, controlHandler authHandler);

/**
 * @brief MQTT Subscribe - send an MQTTv5 subscribe packet and wait for SUBACK before returning.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topicFilter The topic filter to subscribe.
 * @param requestedQoS The requested QoS.
 * @param properties The MQTTv5 subscribe properties.
 * @param options The MQTTv5 subscribe options.
 * @param messageHandler The message handler function. If `NULL`, it will remove an existing messageHandler for this topicFilter.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5Subscribe(MQTTClient* client, const char* topicFilter, enum MQTTQoS requestedQoS, 
  MQTTProperties* properties, MQTTSubscribe_options options, messageHandler messageHandler);

/**
 * @brief MQTT Subscribe - send an MQTTv5 subscribe packet and wait for SUBACK before returning.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topicFilter The topic filter to subscribe.
 * @param requestedQoS The requested QoS.
 * @param properties The MQTTv5 subscribe properties.
 * @param options The MQTTv5 subscribe options.
 * @param messageHandler The message handler function. If `NULL`, it will remove an existing messageHandler for this topicFilter.
 * @param suback Subscription acknowledgement information.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5SubscribeWithResults(MQTTClient* client, const char* topicFilter, enum MQTTQoS requestedQoS, 
  MQTTProperties* properties, MQTTSubscribe_options options, messageHandler messageHandler, MQTTSubackData* suback);

/**
 * @brief MQTT Unsubscribe - send an MQTTv5 unsubscribe packet and wait for UNSUBACK before returning.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topicFilter The topic filter to unsubscribe.
 * @param properties The MQTTv5 unsubscribe properties.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5Unsubscribe(MQTTClient* client, const char* topicFilter, MQTTProperties* properties);

/**
 * @brief MQTT Unsubscribe - send an MQTTv5 unsubscribe packet and wait for UNSUBACK before returning.
 * 
 * @param client The `MQTTClient` object to use.
 * @param topicFilter The topic filter to unsubscribe.
 * @param properties The MQTTv5 unsubscribe properties.
 * @param unsuback Unsubscription acknowledgement information.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5UnsubscribeWithResults(MQTTClient* client, const char* topicFilter, MQTTProperties* properties, 
  MQTTV5UnsubackData* unsuback);

/**
 * @brief MQTT Disconnect - send an MQTTv5 disconnect packet and close the connection.
 * 
 * @param client The `MQTTClient` object to use.
 * @param reasonCode The MQTTv5 reason code.
 * @param properties The MQTTv5 disconnect properties.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5Disconnect(MQTTClient* client, unsigned char reasonCode, MQTTProperties* properties);

/**
 * @brief Set or remove an MQTTv5 disconnect receive handler.
 * 
 * @param client The `MQTTClient` object to use.
 * @param disconnectHandler The AUTH receive handler function or NULL to remove.
 * @return An #MQTTClientReturnCode indicating success or failure.
 */
DLLExport int MQTTV5SetDisconnectHandler(MQTTClient* client, controlHandler disconnectHandler);

#if defined(__cplusplus)
     }
#endif

#endif // MQTTV5_CLIENT_H
