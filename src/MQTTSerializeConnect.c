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

/**
  * Determines the length of the MQTT connect packet that would be produced using the supplied
  * connect options.
  * @param options the options to be used to build the connect packet
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSerialize_connectLength(MQTTPacket_connectData* options)
{
	int len = 0;

	FUNC_ENTRY;
	len = 12 + strlen(options->clientID)+2;
	if (options->will)
		len += strlen(options->will->topicName)+2 + strlen(options->will->message)+2;
	if (options->username)
		len += strlen(options->username)+2;
	if (options->password)
		len += strlen(options->password)+2;

	FUNC_EXIT_RC(len);
	return len;
}


/**
  * Serializes the connect options into the buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @return serialized length, or error if <0
  */
int MQTTSerialize_connect(char* buf, int buflen,
		MQTTPacket_connectData* options)
{
	char *ptr = buf;
	MQTTHeader header;
	MQTTConnectFlags flags;
	int len;

	FUNC_ENTRY;
	if ((len = MQTTSerialize_connectLength(options)) > buflen)
	{
		len = MQTTPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}

	header.byte = 0;
	header.bits.type = CONNECT;
	writeChar(&ptr, header.byte); /* write header */

	ptr += MQTTPacket_encode(ptr, len); /* write remaining length */

	if (options->MQTTVersion == 4)
	{
		writeUTF(&ptr, "MQTT");
		writeChar(&ptr, (char) 4);
	}
	else
	{
		writeUTF(&ptr, "MQIsdp");
		writeChar(&ptr, (char) 3);
	}

	flags.all = 0;
	flags.bits.cleanstart = options->cleansession;
	flags.bits.will = (options->will) ? 1 : 0;
	if (flags.bits.will)
	{
		flags.bits.willQoS = options->will->qos;
		flags.bits.willRetain = options->will->retained;
	}

	if (options->username)
		flags.bits.username = 1;
	if (options->password)
		flags.bits.password = 1;

	writeChar(&ptr, flags.all);
	writeInt(&ptr, options->keepAliveInterval);
	writeUTF(&ptr, options->clientID);
	if (options->will)
	{
		writeUTF(&ptr, options->will->topicName);
		writeUTF(&ptr, options->will->message);
	}
	if (options->username)
		writeUTF(&ptr, options->username);
	if (options->password)
		writeUTF(&ptr, options->password);

	exit: FUNC_EXIT_RC(len);
	return len;
}
