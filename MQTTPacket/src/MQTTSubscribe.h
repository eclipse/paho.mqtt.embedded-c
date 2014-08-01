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

int MQTTSerialize_subscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid,
		int count, MQTTString topicFilters[], int requestedQoSs[]);

int MQTTDeserialize_subscribe(unsigned char* dup, unsigned short* packetid,
		int maxcount, int* count, MQTTString topicFilters[], int requestedQoSs[], unsigned char* buf, int len);

int MQTTSerialize_suback(unsigned char* buf, int buflen, unsigned short packetid, int count, int* grantedQoSs);

int MQTTDeserialize_suback(unsigned short* packetid, int maxcount, int* count, int grantedQoSs[], unsigned char* buf, int len);


#endif /* MQTTSUBSCRIBE_H_ */
