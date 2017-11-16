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

#if !defined(MQTTV5PUBLISH_H_)
#define MQTTV5PUBLISH_H_

#if !defined(DLLImport)
  #define DLLImport
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif

#include "MQTTPublish.h"

DLLExport int MQTTV5Serialize_publish(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned char retained,
  unsigned short packetid, MQTTString topicName, MQTTProperties* props, unsigned char* payload, int payloadlen);

DLLExport int MQTTV5Deserialize_publish(unsigned char* dup, int* qos, unsigned char* retained, unsigned short* packetid, MQTTString* topicName,
		MQTTProperties* props, unsigned char** payload, int* payloadlen, unsigned char* buf, int len);

DLLExport int MQTTV5Serialize_puback(unsigned char* buf, int buflen, unsigned short packetid);
DLLExport int MQTTV5Serialize_pubrel(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid);
DLLExport int MQTTV5Serialize_pubcomp(unsigned char* buf, int buflen, unsigned short packetid);

#endif /* MQTTV5PUBLISH_H_ */
