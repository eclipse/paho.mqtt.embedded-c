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

#include "MQTTPacket.h"
#include "StackTrace.h"

#include <string.h>

/**
 * @return the remaining length size
 */
int MQTTSerialize_publishLength(int qos, MQTTString topicString, int payloadlen)
{
	int len = 0;

	len += 2 + MQTTstrlen(topicString) + payloadlen;
	if (qos > 0)
		len += 2; /* msgid */
	return len;
}


int MQTTSerialize_publish(char* buf, int buflen, int dup, int qos, int retained, int msgid, MQTTString topicString,
		char* payload, int payloadlen)
{
	char *ptr = buf;
	MQTTHeader header;
	int rem_len = 0;
	int rc = -1;

	FUNC_ENTRY;
	if (MQTTPacket_len(rem_len = MQTTSerialize_publishLength(qos, topicString, payloadlen)) > buflen)
	{
		rc = MQTTPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}

	header.bits.type = PUBLISH;
	header.bits.dup = dup;
	header.bits.qos = qos;
	header.bits.retain = retained;
	writeChar(&ptr, header.byte); /* write header */

	ptr += MQTTPacket_encode(ptr, rem_len); /* write remaining length */;

	writeMQTTString(&ptr, topicString);

	if (qos > 0)
		writeInt(&ptr, msgid);

	memcpy(ptr, payload, payloadlen);
	ptr += payloadlen;

	rc = ptr - buf;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTSerialize_ack(char* buf, int buflen, int type, int msgid, int dup)
{
	MQTTHeader header;
	int rc = -1;
	char *ptr = buf;

	FUNC_ENTRY;
	if (buflen < 4)
	{
		rc = MQTTPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	header.bits.type = type;
	header.bits.dup = dup;
	header.bits.qos = 0;
	writeChar(&ptr, header.byte); /* write header */

	ptr += MQTTPacket_encode(ptr, 2); /* write remaining length */
	writeInt(&ptr, msgid);
	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTSerialize_puback(char* buf, int buflen, int msgid)
{
	return MQTTSerialize_ack(buf, buflen, PUBACK, msgid, 0);
}


int MQTTSerialize_pubrel(char* buf, int buflen, int msgid, int dup)
{
	return MQTTSerialize_ack(buf, buflen, PUBREL, msgid, dup);
}


int MQTTSerialize_pubcomp(char* buf, int buflen, int msgid)
{
	return MQTTSerialize_ack(buf, buflen, PUBCOMP, msgid, 0);
}


