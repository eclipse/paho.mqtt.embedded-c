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

#ifndef MQTTPUBLISH_H_
#define MQTTPUBLISH_H_

int MQTTSerialize_publish(char* buf, int buflen, int dup, int qos, int retained, int msgid, MQTTString topicString,
		char* payload, int payloadlen);

int MQTTDeserialize_publish(int* dup, int* qos, int* retained, int* msgid, MQTTString* topicString,
		char** payload, int* payloadlen, char* buf, int len);

int MQTTSerialize_puback(char* buf, int buflen, int msgid);
int MQTTSerialize_pubrel(char* buf, int buflen, int msgid, int dup);
int MQTTSerialize_pubcomp(char* buf, int buflen, int msgid);

#endif /* MQTTPUBLISH_H_ */
