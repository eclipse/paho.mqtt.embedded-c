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

#ifndef MQTTUNSUBSCRIBE_H_
#define MQTTUNSUBSCRIBE_H_

int MQTTSerialize_unsubscribe(char* buf, int buflen, int dup, int msgid, int count, MQTTString topicString[]);

int MQTTDeserialize_unsubscribe(int* dup, int* msgid, int max_count, int* count, MQTTString topicString[], char* buf, int len);

int MQTTSerialize_unsuback(char* buf, int buflen, int msgid);

int MQTTDeserialize_unsuback(int* msgid, char* buf, int len);

#endif /* MQTTUNSUBSCRIBE_H_ */
