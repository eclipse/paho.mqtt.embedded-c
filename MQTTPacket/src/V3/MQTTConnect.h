/*******************************************************************************
 * Copyright (c) 2014, 2023 IBM Corp. and others
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
 *    Ian Craggs - add connack return code definitions
 *    Xiang Rong - 442039 Add makefile to Embedded C client
 *    Ian Craggs - fix for issue #64, bit order in connack response
 *******************************************************************************/

#ifndef MQTTCONNECT_H_
#define MQTTCONNECT_H_

#include <stdint.h>
#include "MQTTConnectInternal.h"

enum MQTTConnackReturnCodes
{
    MQTTCONNACK_CONNECTION_ACCEPTED = 0,
    MQTTCONNACK_UNNACCEPTABLE_PROTOCOL = 1,
    MQTTCONNACK_CLIENTID_REJECTED = 2,
    MQTTCONNACK_SERVER_UNAVAILABLE = 3,
    MQTTCONNACK_BAD_USERNAME_OR_PASSWORD = 4,
    MQTTCONNACK_NOT_AUTHORIZED = 5,
};

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
} MQTTPacket_willOptions;

#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
	  */
	unsigned char MQTTVersion;
	MQTTString clientID;
	unsigned short keepAliveInterval;
	unsigned char cleansession;
	unsigned char willFlag;

	MQTTPacket_willOptions will;
	MQTTString username;
	MQTTString password;

} MQTTPacket_connectData;

#define MQTTPacket_connectData_initializer { {'M', 'Q', 'T', 'C'}, 4, {NULL, {0, NULL}}, 60, 1, 0, \
		MQTTPacket_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

DLLExport int MQTTSerialize_connect(unsigned char* buf, int32_t buflen, MQTTPacket_connectData* options);
DLLExport int MQTTDeserialize_connect(MQTTPacket_connectData* data, unsigned char* buf, int32_t len);

DLLExport int MQTTSerialize_connack(unsigned char* buf, int32_t buflen, unsigned char connack_rc, unsigned char sessionPresent);
DLLExport int MQTTDeserialize_connack(unsigned char* sessionPresent, unsigned char* connack_rc, unsigned char* buf, int32_t buflen);

DLLExport int MQTTSerialize_disconnect(unsigned char* buf, int32_t buflen);
DLLExport int MQTTDeserialize_disconnect(unsigned char* buf, int32_t buflen);

DLLExport int MQTTSerialize_pingreq(unsigned char* buf, int32_t buflen);

#endif /* MQTTCONNECT_H_ */
