/*******************************************************************************
 * Copyright (c) 2017, 2023 IBM Corp.
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

#ifndef MQTTV5PACKET_H_
#define MQTTV5PACKET_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include "MQTTPacketInternal.h"
#include "V5/MQTTReasonCodes.h"
#include "V5/MQTTProperties.h"
#include "V5/MQTTV5Connect.h"
#include "V5/MQTTV5Publish.h"
#include "V5/MQTTV5Subscribe.h"
#include "V5/MQTTV5Unsubscribe.h"

void writeInt4(unsigned char** pptr, int anInt);
int readInt4(unsigned char** pptr);
void writeMQTTLenString(unsigned char** pptr, MQTTLenString lenstring);
int MQTTLenStringRead(MQTTLenString* lenstring, unsigned char** pptr, unsigned char* enddata);

DLLExport int32_t MQTTV5Serialize_ack(unsigned char* buf, int32_t buflen, unsigned char packettype, unsigned char dup, unsigned short packetid,
	unsigned char reasonCode, MQTTProperties* properties);
DLLExport int32_t MQTTV5Deserialize_ack(unsigned char* packettype, unsigned char* dup, unsigned short* packetid,
	unsigned char *reasonCode, MQTTProperties* properties, unsigned char* buf, int32_t buflen);

DLLExport int MQTTV5Packet_equals(MQTTString* a, char* b);
DLLExport int32_t MQTTV5Packet_encode(unsigned char* buf, int32_t length);
DLLExport int MQTTV5Packet_read(unsigned char* buf, int32_t buflen, int (*getfn)(unsigned char*, int));
typedef struct {
	int (*getfn)(void *, unsigned char*, int); /* must return -1 for error, 0 for call again, or the number of bytes read */
	void *sck;	/* pointer to whatever the system may use to identify the transport */
	int multiplier;
	int rem_len;
	int32_t len;
	char state;
} MQTTV5Transport;

DLLExport int MQTTV5Packet_readnb(unsigned char* buf, int32_t buflen, MQTTV5Transport *trp);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif /* MQTTV5PACKET_H_ */
