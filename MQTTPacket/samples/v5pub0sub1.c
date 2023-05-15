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
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *    Cristian Pop - adding MQTTv5 sample
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "V5/MQTTV5Packet.h"
#include "transport.h"
#include "v5log.h"

/* This is in order to get an asynchronous signal to stop the sample,
as the code loops waiting for msgs on the subscribed topic.
Your actual code will depend on your hw and approach*/
#include <signal.h>

int toStop = 0;

void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}

void stop_init(void)
{
	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);
}
/* */

int main(int argc, char *argv[])
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	int rc = 0;
	int mysock = 0;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	int msgid = 1;
	MQTTString topicString = MQTTString_initializer;
	int req_qos = 0;
	char* payload = "mypayload";
	int payloadlen = strlen(payload);
	int len = 0;
	char *host = "test.mosquitto.org";
	int port = 1884;
	MQTTProperty connack_properties_array[5];
	MQTTProperties connack_properties = MQTTProperties_initializer;

	connack_properties.array = connack_properties_array;
	connack_properties.max_count = 5;

	MQTTProperty pub_properties_array[2];
	MQTTProperties pub_properties = MQTTProperties_initializer;
	pub_properties.array = pub_properties_array;
	pub_properties.max_count = 2;

	MQTTProperty v5topic_alias = {
		.identifier = TOPIC_ALIAS,
		.value.integer2 = 1
	};

	rc = MQTTProperties_add(&pub_properties, &v5topic_alias);
	if (rc)
	{
		printf("Failed to add topic alias\n");
		goto exit;
	}
	
	MQTTProperty v5property = {
		.identifier = USER_PROPERTY,
		.value.string_pair.key.data = "user key",
		.value.string_pair.key.len = strlen(v5property.value.string_pair.key.data),
		.value.string_pair.val.data = "user value",
		.value.string_pair.val.len = strlen(v5property.value.string_pair.val.data),
	};

	rc = MQTTProperties_add(&pub_properties, &v5property);
	if (rc)
	{
		printf("Failed to add user property\n");
		goto exit;
	}

	stop_init();
	if (argc > 1)
		host = argv[1];

	if (argc > 2)
		port = atoi(argv[2]);

	mysock = transport_open(host, port);
	if(mysock < 0)
		return mysock;

	printf("Sending to hostname %s port %d\n", host, port);

	data.clientID.cstring = "paho-emb-v5pub0sub1";
	data.keepAliveInterval = 20;
	data.cleansession = 1;
	data.username.cstring = "rw";
	data.password.cstring = "readwrite";
	data.MQTTVersion = 5;

	MQTTProperties conn_properties = MQTTProperties_initializer;
	MQTTProperties will_properties = MQTTProperties_initializer;

	len = MQTTV5Serialize_connect(buf, buflen, &data, &conn_properties, &will_properties);
	rc = transport_sendPacketBuffer(mysock, buf, len);

	/* wait for connack */
	if (MQTTPacket_read(buf, buflen, transport_getdata) == CONNACK)
	{
		unsigned char sessionPresent, connack_rc;

		if (MQTTV5Deserialize_connack(&connack_properties, &sessionPresent, &connack_rc, buf, buflen) != 1 
			|| connack_rc != 0)
		{
			printf("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
	}
	else
		goto exit;

	printf("MQTTv5 connected: (%d properties)\n", connack_properties.count);
	for(int i = 0; i < connack_properties.count; i++)
	{
		v5property_print(connack_properties.array[i]);
	}

	// TODO: v5 SUBSCRIBE

	/* subscribe */
	topicString.cstring = "substopic";
	len = MQTTV5Serialize_subscribe(buf, buflen, 0, msgid, &sub_properties, 1, &topicString, &req_qos);

	rc = transport_sendPacketBuffer(mysock, buf, len);
	if (MQTTPacket_read(buf, buflen, transport_getdata) == SUBACK) 	/* wait for suback */
	{
		unsigned short submsgid;
		int subcount;
		int granted_qos;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
		if (granted_qos != 0)
		{
			printf("granted qos != 0, %d\n", granted_qos);
			goto exit;
		}
	}
	else
		goto exit;

	/* loop getting msgs on subscribed topic */
	topicString.cstring = "pubtopic";
	while (!toStop)
	{
		/* transport_getdata() has a built-in 1 second timeout,
		your mileage will vary */
		if (MQTTPacket_read(buf, buflen, transport_getdata) == PUBLISH)
		{
			unsigned char dup;
			int qos;
			unsigned char retained;
			unsigned short msgid;
			int payloadlen_in;
			unsigned char* payload_in;
			int rc;
			MQTTString receivedTopic;

			// TODO: V5 deserialize
			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
					&payload_in, &payloadlen_in, buf, buflen);
			printf("message arrived %.*s\n", payloadlen_in, payload_in);
		}

		printf("publishing reading\n");
		len = MQTTV5Serialize_publish(buf, buflen, 0, 0, 0, 0, topicString, &pub_properties, 
					(unsigned char *)payload, payloadlen);
		rc = transport_sendPacketBuffer(mysock, buf, len);
	}

	printf("disconnecting\n");
	len = MQTTSerialize_disconnect(buf, buflen);
	rc = transport_sendPacketBuffer(mysock, buf, len);

exit:
	transport_close(mysock);

	return 0;
}
