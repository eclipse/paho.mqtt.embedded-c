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
int MQTTSerialize_unsubscribeLength(int count, MQTTString topicString[])
{
	int i;
	int len = 2; /* msgid */

	for (i = 0; i < count; ++i)
		len += 2 + MQTTstrlen(topicString[i]); /* length + topic*/
	return len;
}


int MQTTSerialize_unsubscribe(char* buf, int buflen, int dup, int msgid, int count, MQTTString topicString[])
{
	char *ptr = buf;
	MQTTHeader header;
	int rem_len = 0;
	int rc = -1;
	int i = 0;

	FUNC_ENTRY;
	if (MQTTPacket_len(rem_len = MQTTSerialize_unsubscribeLength(count, topicString)) > buflen)
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
		writeMQTTString(&ptr, topicString[i]);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTDeserialize_unsuback(int* msgid, char* buf, int buflen)
{
	int type = 0;
	int dup = 0;
	int rc = -1;

	FUNC_ENTRY;
	rc = MQTTDeserialize_ack(&type, &dup, msgid, buf, buflen);
	if (type == UNSUBACK)
		rc = 1;
	FUNC_EXIT_RC(rc);
	return rc;
}


