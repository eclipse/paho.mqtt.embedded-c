/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
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
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTV5Packet.h"
#include "transport.h"


int main(int argc, char *argv[])
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	int rc = 0;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	int mysock = 0;
	MQTTString topicString = MQTTString_initializer;
	char* payload = "mypayload";
	int payloadlen = strlen(payload);
	int len = 0;
	char *host = "localhost";
	int port = 1883;
	MQTTProperties properties = MQTTProperties_initializer;
	MQTTProperty props[10];
	MQTTProperty one;

	if (argc > 1)
		host = argv[1];

	if (argc > 2)
		port = atoi(argv[2]);

	mysock = transport_open(host,port);
	if(mysock < 0)
		return mysock;

	printf("Sending to hostname %s port %d\n", host, port);

	data.clientID.cstring = "me";
	data.keepAliveInterval = 20;
	data.cleansession = 1;
	data.username.cstring = "testuser";
	data.password.cstring = "testpassword";
	data.MQTTVersion = 5;

	properties.max_count = 10;
	properties.array = props;

	one.identifier = SESSION_EXPIRY_INTERVAL;
	one.value.integer4 = 45;
	rc = MQTTProperties_add(&properties, &one);

	len = MQTTV5Serialize_connect((unsigned char *)buf, buflen, &data, &properties, NULL);
	rc = transport_sendPacketBuffer(mysock, buf, len);

	/* wait for connack */
	if (MQTTPacket_read(buf, buflen, transport_getdata) == CONNACK)
	{
		unsigned char sessionPresent, reasonCode;

		if (MQTTV5Deserialize_connack(&properties, &sessionPresent, &reasonCode, buf, buflen) != 1 || reasonCode != 0)
		{
			printf("Unable to connect, return code %d\n", reasonCode);
			goto exit;
		}
	}
	else
	{
		printf("Failed to read connack\n");
		goto exit;
	}

	topicString.cstring = "mytopic";
	//len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char *)payload, payloadlen);
	//rc = transport_sendPacketBuffer(mysock, buf, len);

	len = MQTTV5Serialize_disconnect(buf, buflen, 0, &properties);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	if (rc == len)
		printf("Successful disconnect\n");
	else
		printf("Publish failed\n");

exit:
	transport_close(mysock);

	return 0;
}
