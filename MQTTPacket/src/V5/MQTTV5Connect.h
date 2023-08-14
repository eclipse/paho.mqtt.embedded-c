/*******************************************************************************
 * Copyright (c) 2017, 2023 IBM Corp., Ian Craggs and others
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

#ifndef MQTT5CONNECT_H_
#define MQTT5CONNECT_H_

#include "MQTTConnectInternal.h"
#include "V5/MQTTProperties.h"

/**
 * Defines the MQTT "Last Will and Testament" (LWT) settings for
 * the connect packet.
 */
typedef struct
{
	/** The eyecatcher for this structure.  must be MQTW. */
	char struct_id[4];
	/** The LWT topic to which the LWT message will be published. */
	MQTTString topicName;
	/** The LWT payload. */
	MQTTString message;
	/**
      * The retained flag for the LWT message (see MQTTAsync_message.retained).
      */
	unsigned char retained;
	/**
      * The quality of service setting for the LWT message (see
      * MQTTAsync_message.qos and @ref qos).
      */
	char qos;
	/**
	  * LWT properties.
	  */
	MQTTProperties* properties;
} MQTTV5Packet_willOptions;

#define MQTTV5Packet_willOptions_initializer { {'M', 'Q', 'T', 'W'}, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0, NULL }

typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** Version of MQTT to be used.  5 = 5.0
	  */
	unsigned char MQTTVersion;
	MQTTString clientID;
	unsigned short keepAliveInterval;
	unsigned char cleanstart;
	unsigned char willFlag;

	MQTTV5Packet_willOptions will;
	MQTTString username;
	MQTTString password;

} MQTTV5Packet_connectData;

#define MQTTV5Packet_connectData_initializer { {'M', 'Q', 'T', 'C'}, 5, {NULL, {0, NULL}}, 60, 1, 0, \
		MQTTV5Packet_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

DLLExport int32_t MQTTV5Serialize_connect(unsigned char* buf, int32_t buflen, MQTTV5Packet_connectData* options,
  MQTTProperties* connectProperties);

DLLExport int32_t MQTTV5Deserialize_connect(MQTTProperties* connectProperties, MQTTV5Packet_connectData* data, 
  unsigned char* buf, int32_t len);

DLLExport int32_t MQTTV5Serialize_connack(unsigned char* buf, int32_t buflen, unsigned char connack_rc,
  unsigned char sessionPresent, MQTTProperties* connackProperties);

DLLExport int32_t MQTTV5Deserialize_connack(MQTTProperties* connackProperties,
  unsigned char* sessionPresent, unsigned char* connack_rc, unsigned char* buf, int32_t buflen);

DLLExport int32_t MQTTV5Serialize_disconnect(unsigned char* buf, int32_t buflen, unsigned char reasonCode,
  MQTTProperties* properties);

DLLExport int32_t MQTTV5Deserialize_disconnect(MQTTProperties* properties, unsigned char* reasonCode,
  unsigned char* buf, int32_t buflen);

DLLExport int32_t MQTTV5Serialize_auth(unsigned char* buf, int32_t buflen, unsigned char reasonCode,
  MQTTProperties* properties);

DLLExport int32_t MQTTV5Deserialize_auth(MQTTProperties* properties, unsigned char* reasonCode,
  unsigned char* buf, int32_t buflen);

DLLExport int MQTTV5Serialize_pingreq(unsigned char* buf, int32_t buflen);

#endif /* MQTTV5CONNECT_H_ */
