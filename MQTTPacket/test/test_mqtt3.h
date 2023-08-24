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

/***
  Test MQTTPacket against a broker.
***/
int test_v3(struct Options options) 
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
	int msgid = 0;
	char* test_topic = "MQTTV3/test/test35_topic";
	int i = 0;

	mysock = transport_open(options.host, options.port);
	if(mysock < 0)
		return mysock;

	fprintf(xml, "<testcase classname=\"test1\" name=\"MQTTV3_scenario\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 1 - simple MQTT V3 scenario");

	MyLog(LOGA_INFO, "Sending to hostname %s port %d", options.host, options.port);

	data.clientID.cstring = "mqtt3_test3_test1";
	data.keepAliveInterval = 20;
	data.cleansession = 1;
	data.username.cstring = "testuser";
	data.password.cstring = "testpassword";
	data.MQTTVersion = 4;

	len = MQTTSerialize_connect((unsigned char *)buf, buflen, &data);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

	/* wait for connack */
	rc = MQTTPacket_read(buf, buflen, transport_getdata);
	assert("Should receive connack", rc == CONNACK, "did not get connack %d\n", rc);
	if (rc == CONNACK)
	{
		unsigned char sessionPresent, reasonCode;

		rc = MQTTDeserialize_connack(&sessionPresent, &reasonCode, buf, buflen);

		assert("rc should be 1", rc == 1, "rc was not 1 %d\n", rc);
		assert("reasonCode should be 0", reasonCode == 0, "Unable to connect, return code %d\n", reasonCode);
		assert("sessionPresent should be 0", sessionPresent == 0, "Session present was not 0 %d\n", sessionPresent);
	}

	/* subscribe */
	topicString.cstring = test_topic;
	unsigned char req_qos = 2;

	len = MQTTSerialize_subscribe(buf, buflen, 0, ++msgid, 1, &topicString, &req_qos);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

	rc = MQTTPacket_read(buf, buflen, transport_getdata); /* wait for suback */
	assert("Should receive suback", rc == SUBACK, "did not get suback %d\n", rc);
	if (rc == SUBACK)
	{
		unsigned short submsgid = -1;
		int subcount = 0;
		unsigned char granted_qos = -1;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);

		assert("rc should be 1", rc == 1, "rc was not 1 %d\n", rc);
		assert("subcount should be 1", subcount == 1, "subcount was not 1 %d\n", subcount);
		assert("submsgid should be msgid", submsgid == msgid, "submsgid was not msgid %d\n", submsgid);
		assert("granted_qos should be req_qos", granted_qos == req_qos, "granted_qos was %d\n", granted_qos);
	}

	topicString.cstring = test_topic;
	len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char *)payload, payloadlen);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

	rc = MQTTPacket_read(buf, buflen, transport_getdata); /* wait for publish */
	assert("Should receive publish", rc == PUBLISH, "did not get publish %d\n", rc);
	if (rc == PUBLISH)
	{
		unsigned char* payload2;
		MQTTString topicString2;
		int payloadlen2 = 0;
		unsigned char qos2 = -1;
		unsigned char retained2 = 0, dup2 = 0;
		unsigned short msgid2 = 999;

		rc = MQTTDeserialize_publish(&dup2, &qos2, &retained2, &msgid2, &topicString2,
				&payload2, &payloadlen2, buf, buflen);

		assert("rc should be 1", rc == 1, "rc was not 1 %d\n", rc);
		assert("msgid2 should be unchanged", msgid2 == 999, "msgid was not unchanged %d\n", msgid2);
		assert("retained2 should be 0", retained2 == 0, "retained2 was not 0 %d\n", retained2);
		assert("topic should be test_topic", memcmp(topicString2.lenstring.data, test_topic, topicString2.lenstring.len) == 0,
		       "topic was not test_topic %s\n", topicString2.cstring);
	}

	/* Publish QoS 1 this time */
	topicString.cstring = test_topic;
	len = MQTTSerialize_publish(buf, buflen, 0, 1, 0, ++msgid, topicString, (unsigned char *)payload, payloadlen);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

  i = 0;
	while (i < 2)
	{
	  rc = MQTTPacket_read(buf, buflen, transport_getdata); /* wait for publish and puback */
		assert("Should receive publish or puback", rc == PUBACK || rc == PUBLISH, "did not get puback or publish %d\n", rc);
	  if (rc == PUBLISH)
	  {
			unsigned char* payload2;
			MQTTString topicString2;
			int payloadlen2 = 0;
			unsigned char qos2 = -1;
			unsigned char retained2 = 0, dup2 = 0;
			unsigned short msgid2 = 999;
			static int pubcount = 0;

			++pubcount;
			assert("should get only 1 publish",  pubcount == 1, "pubcount %d\n", pubcount);
			rc = MQTTDeserialize_publish(&dup2, &qos2, &retained2, &msgid2, &topicString2,
					&payload2, &payloadlen2, buf, buflen);

			assert("qos should be 1", qos2 == 1, "qos was not 1 %d\n", qos2);

			len = MQTTSerialize_puback(buf, buflen, msgid2);
			rc = transport_sendPacketBuffer(mysock, buf, len);
			assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);
	  }
	  else
	  {
			static int ackcount = 0;
			unsigned short msgid2 = 999;
			unsigned char packettype = 99, dup = 8;

			ackcount++;
			assert("should get only 1 puback", ackcount == 1, "ackcount %d\n", ackcount);
			len = MQTTDeserialize_ack(&packettype, &dup, &msgid2, buf, buflen);
			assert("packettype should be PUBACK", packettype == PUBACK, "packettype was %d\n", packettype);
			assert("msgid should be msgid2", msgid == msgid2, "msgid was not msgid2 %d\n", msgid2);
    }
		++i;
	}

	/* Publish QoS 2 this time */
	topicString.cstring = test_topic;
	len = MQTTSerialize_publish(buf, buflen, 0, 2, 0, ++msgid, topicString, (unsigned char *)payload, payloadlen);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

	i = 0;
	while (i < 4)
	{
		static unsigned short pubmsgid = 999;

		rc = MQTTPacket_read(buf, buflen, transport_getdata); /* wait for publish, pubrel and pubcomp */
		assert("Should receive publish, pubrec, pubrel or pubcomp", rc == PUBREC || rc == PUBREL || rc == PUBLISH || rc == PUBCOMP,
		       "did not get pubrec, pubrel, pubcomp or publish %d\n", rc);
		if (rc == PUBLISH)
		{
			unsigned char* payload2;
			MQTTString topicString2;
			int payloadlen2 = 0;
			unsigned char qos2 = -1;
			unsigned char retained2 = 0, dup2 = 0;
			static int pubcount = 0;

			++pubcount;
			assert("should get only 1 publish",  pubcount == 1, "pubcount %d\n", pubcount);
			rc = MQTTDeserialize_publish(&dup2, &qos2, &retained2, &pubmsgid, &topicString2,
					&payload2, &payloadlen2, buf, buflen);
			assert("qos should be 2", qos2 == 2, "qos was not 2 %d\n", qos2);

			len = MQTTSerialize_pubrec(buf, buflen, pubmsgid);
			rc = transport_sendPacketBuffer(mysock, buf, len);
			assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);
		}
		else if (rc == PUBREL)
		{
			unsigned char* payload2;
			MQTTString topicString2;
			unsigned char dup2 = 0;
			unsigned char packettype = 99;
			unsigned short msgid2 = 999;
			static int pubrelcount = 0;

			++pubrelcount;
			assert("should get only 1 pubrel",  pubrelcount == 1, "pubrelcount %d\n", pubrelcount);
			len = MQTTDeserialize_ack(&packettype, &dup2, &msgid2, buf, buflen);
			assert("packettype should be PUBREL", packettype == PUBREL, "packettype was %d\n", packettype);
			assert("pubmsgid should be msgid2", pubmsgid == msgid2, "pubmsgid was not msgid2 %d\n", msgid2);

			len = MQTTSerialize_pubcomp(buf, buflen, msgid2);
			rc = transport_sendPacketBuffer(mysock, buf, len);
			assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);
		}
		else if (rc == PUBREC)
		{
			unsigned char* payload2;
			MQTTString topicString2;
			unsigned char dup2 = 0;
			unsigned short msgid2 = 999;
			unsigned char packettype = 99;
			static int pubreccount = 0;

			++pubreccount;
			assert("should get only 1 pubrec",  pubreccount == 1, "pubreccount %d\n", pubreccount);
			len = MQTTDeserialize_ack(&packettype, &dup2, &msgid2, buf, buflen);
			assert("packettype should be PUBREC", packettype == PUBREC, "packettype was %d\n", packettype);
			assert("msgid should be msgid2", msgid == msgid2, "msgid was not msgid2 %d\n", msgid2);

			len = MQTTSerialize_pubrel(buf, buflen, 0, msgid2);
			rc = transport_sendPacketBuffer(mysock, buf, len);
			assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);
		}
		else
		{
			static int pubcompcount = 0;
			unsigned short msgid2 = 999;
			unsigned char packettype = 99, dup = 8;

			pubcompcount++;
			assert("should get only 1 puback", pubcompcount == 1, "pubcompcount %d\n", pubcompcount);
			len = MQTTDeserialize_ack(&packettype, &dup, &msgid2, buf, buflen);
			assert("packettype should be PUBCOMP", packettype == PUBCOMP, "packettype was %d\n", packettype);
			assert("msgid should be msgid2", msgid == msgid2, "msgid was not msgid2 %d\n", msgid2);
		}
		++i;
	}

	/* unsubscribe */
	topicString.cstring = test_topic;
	len = MQTTSerialize_unsubscribe(buf, buflen, 0, ++msgid, 1, &topicString);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

	rc = MQTTPacket_read(buf, buflen, transport_getdata); /* wait for unsuback */
	assert("Should receive unsuback", rc == UNSUBACK, "did not get unsuback %d\n", rc);
	if (rc == UNSUBACK)
	{
		unsigned short unsubmsgid = 9999;

		rc = MQTTDeserialize_unsuback(&unsubmsgid, buf, buflen);

		assert("rc should be 1", rc == 1, "rc was not 1 %d\n", rc);
		assert("unsubmsgid should be msgid", unsubmsgid == msgid, "unsubmsgid was not msgid %d\n", unsubmsgid);
	}

	len = MQTTSerialize_disconnect(buf, buflen);
	rc = transport_sendPacketBuffer(mysock, buf, len);
	assert("rc and len should be the same",  rc == len, "rc was different %d\n", rc);

exit:
	transport_close(mysock);

	MyLog(LOGA_INFO, "TESTv3: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;    

    return 0;
}
