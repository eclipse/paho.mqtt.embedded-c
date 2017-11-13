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

#include "MQTTPacket.h"

#include "MQTTReasonCodes.h"

#include "MQTTProperties.h"

#include "MQTTV5Connect.h"

void writeInt4(unsigned char** pptr, int anInt);
int readInt4(unsigned char** pptr);

void writeMQTTLenString(unsigned char** pptr, MQTTLenString lenstring);

int MQTTLenStringRead(MQTTLenString* lenstring, unsigned char** pptr, unsigned char* enddata);
