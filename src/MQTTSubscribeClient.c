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
 * @return the remaining length
 */
int MQTTSerialize_subscribeLength(int count, MQTTString topicString[])
{
	int i;
	int len = 2; /* msgid */

	for (i = 0; i < count; ++i)
		len += 2 + MQTTstrlen(topicString[i]) + 1; /* length + topic + req_qos */
	return len;
}


int MQTTSerialize_subscribe(char* buf, int buflen, int dup, int msgid, int count, MQTTString topicString[], int reqQos[])
{
	char *ptr = buf;
	MQTTHeader header;
	int rem_len = 0;
	int rc = -1;
	int i = 0;

	FUNC_ENTRY;
	if (MQTTPacket_len(rem_len = MQTTSerialize_subscribeLength(count, topicString)) > buflen)
	{
		rc = MQTTPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}

	header.byte = 0;
	header.bits.type = SUBSCRIBE;
	header.bits.dup = dup;
	header.bits.qos = 1;
	writeChar(&ptr, header.byte); /* write header */

	ptr += MQTTPacket_encode(ptr, rem_len); /* write remaining length */;

	writeInt(&ptr, msgid);

	for (i = 0; i < count; ++i)
	{
		writeMQTTString(&ptr, topicString[i]);
		writeChar(&ptr, reqQos[i]);
	}

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



int MQTTDeserialize_suback(int* msgid, int max_count, int* count, int granted_qos[], char* buf, int buflen)
{
	MQTTHeader header;
	char* curdata = buf;
	char* enddata = NULL;
	int rc = -1;
	int mylen;

	FUNC_ENTRY;
	header.byte = readChar(&curdata);

	curdata += (rc = MQTTPacket_decodeBuf(curdata, &mylen)); /* read remaining length */
	enddata = curdata + mylen;
	if (enddata - curdata < 2)
		goto exit;

	*msgid = readInt(&curdata);

	*count = 0;
	while (curdata < enddata)
	{
		if (*count > max_count)
		{
			rc = -1;
			goto exit;
		}
		granted_qos[(*count)++] = readChar(&curdata);
	}

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


