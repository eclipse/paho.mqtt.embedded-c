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

// TODO: Add MQTTv5 properties implementation (some of the code exists in v5log.h), application must provide memory.
#include "StackTrace.h"
#include "MQTTV5Packet.h"

#include <string.h>

// The index of each packet name as described by the MQTTv5.0 spec, section 2.1.2.
// E.g. MQTTV5Packet_names[15] == "AUTH"
const char* MQTTV5Packet_names[] =
{
	"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL",
	"PUBCOMP", "SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK",
	"PINGREQ", "PINGRESP", "DISCONNECT", "AUTH"
};


const char* MQTTV5Packet_getName(unsigned short packetid)
{
	return MQTTV5Packet_names[packetid];
}


int MQTTV5StringFormat_connect(char* strbuf, int strbuflen, MQTTPacket_connectData* data)
{
	int strindex = 0;

	strindex = snprintf(strbuf, strbuflen,
			"CONNECT MQTT version %d, client id %.*s, clean start %d, keep alive %d",
			(int)data->MQTTVersion, (int) data->clientID.lenstring.len, data->clientID.lenstring.data,
			(int)data->cleanstart, data->keepAliveInterval);
	if (data->willFlag)
		strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
				", will QoS %d, will retain %d, will topic %.*s, will message %.*s",
				data->will.qos, data->will.retained,
				(int) data->will.topicName.lenstring.len, data->will.topicName.lenstring.data,
				(int) data->will.message.lenstring.len, data->will.message.lenstring.data);
	if (data->username.lenstring.data && data->username.lenstring.len > 0)
		strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
				", user name %.*s", (int) data->username.lenstring.len, data->username.lenstring.data);
	if (data->password.lenstring.data && data->password.lenstring.len > 0)
		strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
				", password %.*s", (int) data->password.lenstring.len, data->password.lenstring.data);
	return strindex;
}


int MQTTV5StringFormat_connack(char* strbuf, int strbuflen, enum MQTTReasonCodes reason_code, unsigned char sessionPresent)
{
	int strindex = snprintf(strbuf, strbuflen, "CONNACK session present %d, rc %d", sessionPresent, reason_code);
	return strindex;
}


int MQTTV5StringFormat_publish(char* strbuf, int strbuflen, unsigned char dup, int qos, unsigned char retained,
		unsigned short packetid, MQTTString topicName, unsigned char* payload, int payloadlen)
{
	int strindex = snprintf(strbuf, strbuflen,
				"PUBLISH dup %d, QoS %d, retained %d, packet id %d, topic %.*s, payload length %d, payload %.*s",
				dup, qos, retained, packetid,
				(topicName.lenstring.len < 20) ? (int) topicName.lenstring.len : 20, topicName.lenstring.data,
				(int) payloadlen, (payloadlen < 20) ? (int) payloadlen : 20, payload);
	return strindex;
}


int MQTTV5StringFormat_ack(char* strbuf, int strbuflen, unsigned char packettype, unsigned char dup, unsigned char reason, unsigned short packetid)
{
	int strindex = snprintf(strbuf, strbuflen, "%s, packet id %d reason %d", MQTTV5Packet_names[packettype], packetid, reason);
	if (dup)
		strindex += snprintf(strbuf + strindex, strbuflen - strindex, ", dup %d", dup);
	return strindex;
}


int MQTTV5StringFormat_subscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid, int count,
		MQTTString topicFilters[], unsigned char requestedQoSs[])
{
	return snprintf(strbuf, strbuflen,
		"SUBSCRIBE dup %d, packet id %d count %d topic %.*s qos %d",
		dup, packetid, count,
		(int) topicFilters[0].lenstring.len, topicFilters[0].lenstring.data,
		requestedQoSs[0]);
}


int MQTTV5StringFormat_suback(char* strbuf, int strbuflen, unsigned short packetid, int count, unsigned char* grantedQoSs)
{
	return snprintf(strbuf, strbuflen,
		"SUBACK packet id %d count %d granted qos %d", packetid, count, grantedQoSs[0]);
}


int MQTTV5StringFormat_unsubscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid,
		int count, MQTTString topicFilters[])
{
	return snprintf(strbuf, strbuflen,
					"UNSUBSCRIBE dup %d, packet id %d count %d topic %.*s",
					dup, packetid, count,
					(int) topicFilters[0].lenstring.len, topicFilters[0].lenstring.data);
}


#if defined(MQTT_CLIENT)
char* MQTTV5Format_toClientString(char* strbuf, int strbuflen, unsigned char* buf, int32_t buflen)
{
	int index = 0;
	int rem_length = 0;
	MQTTHeader header = {0};
	int strindex = 0;

	header.byte = buf[index++];
	index += MQTTPacket_decodeBuf(&buf[index], &rem_length);

	switch (header.bits.type)
	{

	case CONNACK:
	{
		unsigned char sessionPresent, connack_rc;
#if defined(MQTT5)

#else
		if (MQTTV5Deserialize_connack(NULL, &sessionPresent, &connack_rc, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_connack(strbuf, strbuflen, connack_rc, sessionPresent);
#endif
	}
	break;
	case PUBLISH:
	{
		unsigned char dup, retained, *payload, qos;
		unsigned short packetid;
		int32_t payloadlen;
		MQTTString topicName = MQTTString_initializer;
		if (MQTTV5Deserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
				NULL, &payload, &payloadlen, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
					topicName, payload, payloadlen);
	}
	break;
	case PUBACK:
	case PUBREC:
	case PUBREL:
	case PUBCOMP:
	{
		unsigned char packettype, dup, reason;
		unsigned short packetid;
		if (MQTTV5Deserialize_ack(&packettype, &dup, &packetid, &reason, NULL, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_ack(strbuf, strbuflen, packettype, dup, reason, packetid);
	}
	break;
	case SUBACK:
	{
		unsigned short packetid;
		int maxcount = 1, count = 0;
		unsigned char grantedQoSs[1];
		unsigned char reasonCodes[1];
		if (MQTTV5Deserialize_suback(&packetid, NULL, maxcount, &count, reasonCodes, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_suback(strbuf, strbuflen, packetid, count, grantedQoSs);
	}
	break;
	case UNSUBACK:
	{
		unsigned short packetid;
		int maxcount = 1, count = 0;
		unsigned char reasonCodes[1];

		if (MQTTV5Deserialize_unsuback(&packetid, NULL, maxcount, &count, reasonCodes, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_ack(strbuf, strbuflen, UNSUBACK, 0, reasonCodes[0], packetid);
	}
	break;
	case PINGREQ:
	case PINGRESP:
	case DISCONNECT:
		strindex = snprintf(strbuf, strbuflen, "%s", MQTTV5Packet_names[header.bits.type]);
		break;
	}
	return strbuf;
}
#endif

#if defined(MQTT_SERVER)
char* MQTTV5Format_toServerString(char* strbuf, int strbuflen, unsigned char* buf, int32_t buflen)
{
	int index = 0;
	int rem_length = 0;
	MQTTHeader header = {0};
	int strindex = 0;

	header.byte = buf[index++];
	index += MQTTPacket_decodeBuf(&buf[index], &rem_length);

	switch (header.bits.type)
	{
	case CONNECT:
	{
		MQTTPacket_connectData data;
		int rc;
		if ((rc = MQTTV5Deserialize_connect(NULL, &data, buf, buflen)) == 1)
			strindex = MQTTV5StringFormat_connect(strbuf, strbuflen, &data);
	}
	break;
	case PUBLISH:
	{
		unsigned char dup, retained, *payload, qos;
		unsigned short packetid;
		int32_t payloadlen;
		MQTTString topicName = MQTTString_initializer;
		if (MQTTV5Deserialize_publish(&dup, &qos, &retained, &packetid, &topicName, NULL,
				&payload, &payloadlen, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
					topicName, payload, payloadlen);
	}
	break;
	case PUBACK:
	case PUBREC:
	case PUBREL:
	case PUBCOMP:
	{
		unsigned char packettype, dup, reason;
		unsigned short packetid;
		if (MQTTV5Deserialize_ack(&packettype, &dup, &packetid, &reason, NULL, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_ack(strbuf, strbuflen, packettype, dup, reason, packetid);
	}
	break;
	case SUBSCRIBE:
	{
		unsigned char dup;
		unsigned short packetid;
		int maxcount = 1, count = 0;
		MQTTString topicFilters[1];
		unsigned char requestedQoSs[1];
		MQTTSubscribe_options sub_options[1];

		if (MQTTV5Deserialize_subscribe(&dup, &packetid, NULL, maxcount, &count,
				topicFilters, requestedQoSs, sub_options, buf, buflen) == 1)
			strindex = MQTTV5StringFormat_subscribe(strbuf, strbuflen, dup, packetid, count, topicFilters, requestedQoSs);;
	}
	break;
	case UNSUBSCRIBE:
	{
		unsigned char dup;
		unsigned short packetid;
		int maxcount = 1, count = 0;
		MQTTString topicFilters[1];
		if (MQTTV5Deserialize_unsubscribe(&dup, &packetid, NULL, maxcount, &count, topicFilters, buf, buflen) == 1)
			strindex =  MQTTV5StringFormat_unsubscribe(strbuf, strbuflen, dup, packetid, count, topicFilters);
	}
	break;
	case PINGREQ:
	case PINGRESP:
	case DISCONNECT:
		strindex = snprintf(strbuf, strbuflen, "%s", MQTTV5Packet_names[header.bits.type]);
		break;
	}
	strbuf[strbuflen] = '\0';
	return strbuf;
}
#endif
