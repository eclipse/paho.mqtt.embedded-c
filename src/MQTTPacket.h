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

#ifndef MQTTPACKET_H_
#define MQTTPACKET_H_

typedef unsigned int bool;

enum errors
{
	MQTTPACKET_BUFFER_TOO_SHORT = -2,
	MQTTPACKET_READ_ERROR = -1,
	MQTTPACKET_READ_COMPLETE,
};

enum msgTypes
{
	CONNECT = 1, CONNACK, PUBLISH, PUBACK, PUBREC, PUBREL,
	PUBCOMP, SUBSCRIBE, SUBACK, UNSUBSCRIBE, UNSUBACK,
	PINGREQ, PINGRESP, DISCONNECT
};

/**
 * Bitfields for the MQTT header byte.
 */
typedef union
{
	/*unsigned*/ char byte;	/**< the whole byte */
#if defined(REVERSED)
	struct
	{
		unsigned int type : 4;	/**< message type nibble */
		bool dup : 1;			/**< DUP flag bit */
		unsigned int qos : 2;	/**< QoS value, 0, 1 or 2 */
		bool retain : 1;		/**< retained flag bit */
	} bits;
#else
	struct
	{
		bool retain : 1;		/**< retained flag bit */
		unsigned int qos : 2;	/**< QoS value, 0, 1 or 2 */
		bool dup : 1;			/**< DUP flag bit */
		unsigned int type : 4;	/**< message type nibble */
	} bits;
#endif
} MQTTHeader;

typedef struct
{
	int len;
	char [];
} MQTTString;

#include "MQTTConnect.h"


int MQTTPacket_encode(char* buf, int length);
int MQTTPacket_decode(int (*getcharfn)(), int* value);

int readInt(char** pptr);
char* readUTF(char** pptr, char* enddata);
char readChar(char** pptr);
void writeChar(char** pptr, char c);
void writeInt(char** pptr, int anInt);
void writeUTF(char** pptr, char* string);

#endif /* MQTTPACKET_H_ */
