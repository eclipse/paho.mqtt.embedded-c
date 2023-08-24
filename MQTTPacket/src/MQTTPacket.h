/*******************************************************************************
 * Copyright (c) 2014, 2023 IBM Corp., Ian Craggs and others
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
 *    Xiang Rong - 442039 Add makefile to Embedded C client
 *******************************************************************************/

#ifndef MQTTPACKET_H_
#define MQTTPACKET_H_

#include <stdint.h>

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include "MQTTPacketInternal.h"
#include "V3/MQTTConnect.h"
#include "V3/MQTTPublish.h"
#include "V3/MQTTSubscribe.h"
#include "V3/MQTTUnsubscribe.h"
#include "V3/MQTTFormat.h"

DLLExport int32_t MQTTSerialize_ack(unsigned char* buf, int32_t buflen, unsigned char type, unsigned char dup, unsigned short packetid);
DLLExport int32_t MQTTDeserialize_ack(unsigned char* packettype, unsigned char* dup, unsigned short* packetid, unsigned char* buf, int32_t buflen);

DLLExport int MQTTPacket_equals(MQTTString* a, char* b);
DLLExport int32_t MQTTPacket_encode(unsigned char* buf, int32_t length);
DLLExport int MQTTPacket_read(unsigned char* buf, int32_t buflen, int (*getfn)(unsigned char*, int));
typedef struct {
	int (*getfn)(void *, unsigned char*, int); /* must return -1 for error, 0 for call again, or the number of bytes read */
	void *sck;	/* pointer to whatever the system may use to identify the transport */
	int multiplier;
	int rem_len;
	int32_t len;
	char state;
} MQTTTransport;

DLLExport int MQTTPacket_readnb(unsigned char* buf, int32_t buflen, MQTTTransport *trp);

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
}
#endif


#endif /* MQTTPACKET_H_ */
