/*******************************************************************************
 * Copyright (c) 2016 IBM Corp. and others
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
 *    Rafael de Lucena Valle - add support for ESP8266 WiFi module
 *******************************************************************************/

#if !defined(MQTT_LOGGING_H)
#define MQTT_LOGGING_H

#if !defined(DEBUG)
#define DEBUG(...)    \
    {\
    Serial.printf("DEBUG:   %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    Serial.printf(__VA_ARGS__); \
    Serial.flush(); \
    }
#endif
#if !defined(LOG)
#define LOG(...)    \
    {\
    Serial.printf("LOG:   %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    Serial.printf(__VA_ARGS__); \
    Serial.flush(); \
    }
#endif
#if !defined(WARN)
#define WARN(...)   \
    { \
    Serial.printf("WARN:  %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    Serial.printf(__VA_ARGS__); \
    Serial.flush(); \
    }
#endif
#if !defined(ERROR)
#define ERROR(...)  \
    { \
    Serial.printf("ERROR: %s L#%d ", __PRETTY_FUNCTION__, __LINE__); \
    Serial.printf(__VA_ARGS__); \
    Serial.flush(); \
    exit(1); \
    }
#endif

#endif
