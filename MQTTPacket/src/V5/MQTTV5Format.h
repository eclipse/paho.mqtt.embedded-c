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
 *******************************************************************************/

#if !defined(MQTTV5FORMAT_H)
#define MQTTV5FORMAT_H

#include "StackTrace.h"
#include "MQTTV5Packet.h"

const char* MQTTV5Packet_getName(unsigned short packetid);
int MQTTV5StringFormat_connect(char* strbuf, int strbuflen, MQTTPacket_connectData* data);
int MQTTV5StringFormat_connack(char* strbuf, int strbuflen, unsigned char connack_rc, unsigned char sessionPresent);
int MQTTV5StringFormat_publish(char* strbuf, int strbuflen, unsigned char dup, int qos, unsigned char retained,
		unsigned short packetid, MQTTString topicName, unsigned char* payload, int payloadlen);
int MQTTV5StringFormat_ack(char* strbuf, int strbuflen, unsigned char packettype, unsigned char dup, unsigned short packetid);
int MQTTV5StringFormat_subscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid, int count,
		MQTTString topicFilters[], unsigned char requestedQoSs[]);
int MQTTV5StringFormat_suback(char* strbuf, int strbuflen, unsigned short packetid, int count, unsigned char* grantedQoSs);
int MQTTV5StringFormat_unsubscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid,
		int count, MQTTString topicFilters[]);
char* MQTTV5Format_toClientString(char* strbuf, int strbuflen, unsigned char* buf, int32_t buflen);
char* MQTTV5Format_toServerString(char* strbuf, int strbuflen, unsigned char* buf, int32_t buflen);

#endif
