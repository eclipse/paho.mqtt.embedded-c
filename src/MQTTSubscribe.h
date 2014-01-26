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

#ifndef MQTTSUBSCRIBE_H_
#define MQTTSUBSCRIBE_H_

int MQTTSerialize_subscribe(char* buf, int buflen, int dup, int msgid, int count, MQTTString topicString[], int reqQos[]);

int MQTTDeserialize_subscribe(int* dup, int* msgid, int max_count, int* count, MQTTString topicString[], int reqQos[], char* buf, int len);

int MQTTSerialize_suback(char* buf, int buflen, int msgid, int count, int* granted_qos);

int MQTTDeserialize_suback(int* msgid, int max_count, int* count, int granted_qos[], char* buf, int len);


#endif /* MQTTSUBSCRIBE_H_ */
