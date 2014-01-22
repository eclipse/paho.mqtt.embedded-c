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

#include "StackTrace.h"
#include "MQTTPacket.h"

/**
  * Deserializes the buffer into connect options.
  * @param data the options to be used to build the connect packet
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the supplied buffer
  * @return error code.  0 is success
  */
int MQTTDeserialize_connect(MQTTPacket_connectData* data, char* buf, int len)
{
	MQTTHeader header;
	MQTTConnectFlags flags;
	char* curdata = data;
	char* enddata = &data[len];
	int rc;
	MQTTString* Protocol;
	int version;

	FUNC_ENTRY;

	header.byte = readChar(&curdata);

	if ((Protocol = readUTF(Protocol, &curdata, enddata)) == NULL || /* should be "MQIsdp" */
		enddata - curdata < 0) /* can we read protocol version char? */
		goto exit;

	version = (int)readChar(&curdata); /* Protocol version */
	/* If we don't recognize the protocol version, we don't parse the connect packet on the
	 * basis that we don't know what the format will be.
	 */
	if (MQTTPacket_checkVersion(version))
	{
		flags.all = readChar(&curdata);
		data->keepAliveInterval = readInt(&curdata);
		if ((data->clientID = readUTF(&curdata, enddata)) == NULL)
			goto exit;
		if (flags.bits.will)
		{
			if ((data->will->topicName = readUTF(&curdata, enddata)) == NULL ||
				  (data->will->message = readUTF(&curdata, enddata)) == NULL)
				goto exit;
			//Log(TRACE_MAX, 18, NULL, pack->willTopic, pack->willMsg, pack->flags.bits.willRetain);
		}
		if (flags.bits.username)
		{
			if (enddata - curdata < 3 || (data->username = readUTF(&curdata,enddata)) == NULL)
				goto exit; /* username flag set, but no username supplied - invalid */
			if (flags.bits.password &&
					(enddata - curdata < 3 || (data->password = readUTF(&curdata,enddata)) == NULL))
				goto exit; /* password flag set, but no password supplied - invalid */
		}
		else if (flags.bits.password)
			goto exit; /* password flag set without username - invalid */
	}
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



