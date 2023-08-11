/*******************************************************************************
 * Copyright (c) 2014, 2023 IBM Corp. and others
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
 *    Ian Craggs - add connack return code definitions
 *    Xiang Rong - 442039 Add makefile to Embedded C client
 *    Ian Craggs - fix for issue #64, bit order in connack response
 *******************************************************************************/

#ifndef MQTTCONNECT_H_
#define MQTTCONNECT_H_

#include <stdint.h>
#include "MQTTConnectCommon.h"

#define MQTTPacket_connectData_initializer { {'M', 'Q', 'T', 'C'}, 4, {NULL, {0, NULL}}, 60, 1, 0, \
		MQTTPacket_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

DLLExport int MQTTSerialize_connect(unsigned char* buf, int32_t buflen, MQTTPacket_connectData* options);
DLLExport int MQTTDeserialize_connect(MQTTPacket_connectData* data, unsigned char* buf, int32_t len);

DLLExport int MQTTSerialize_connack(unsigned char* buf, int32_t buflen, unsigned char connack_rc, unsigned char sessionPresent);
DLLExport int MQTTDeserialize_connack(unsigned char* sessionPresent, unsigned char* connack_rc, unsigned char* buf, int32_t buflen);

DLLExport int MQTTSerialize_disconnect(unsigned char* buf, int32_t buflen);
DLLExport int MQTTDeserialize_disconnect(unsigned char* buf, int32_t buflen);
DLLExport int MQTTSerialize_pingreq(unsigned char* buf, int32_t buflen);

#endif /* MQTTCONNECT_H_ */
