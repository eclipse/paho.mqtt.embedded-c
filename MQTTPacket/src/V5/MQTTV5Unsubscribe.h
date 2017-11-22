/*******************************************************************************
 * Copyright (c) 2017 IBM Corp.
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

#ifndef MQTTV5UNSUBSCRIBE_H_
#define MQTTV5UNSUBSCRIBE_H_

#if !defined(DLLImport)
  #define DLLImport
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif

DLLExport int MQTTV5Serialize_unsubscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid,
	MQTTProperties* properties, int count, MQTTString topicFilters[]);

DLLExport int MQTTV5Deserialize_unsubscribe(unsigned char* dup, unsigned short* packetid, MQTTProperties* properties,
	int maxcount, int* count, MQTTString topicFilters[], unsigned char* buf, int len);

DLLExport int MQTTV5Serialize_unsuback(unsigned char* buf, int buflen, unsigned short packetid,
  MQTTProperties* properties, int count, int* reasonCodes);

DLLExport int MQTTV5Deserialize_unsuback(unsigned short* packetid, MQTTProperties* properties,
  int maxcount, int* count, int reasonCodes[], unsigned char* buf, int len);

int MQTTV5Deserialize_subunsuback(int type, unsigned short* packetid, MQTTProperties* properties,
  int maxcount, int* count, int reasonCodes[], unsigned char* buf, int buflen);


#endif /* MQTTV5UNSUBSCRIBE_H_ */
