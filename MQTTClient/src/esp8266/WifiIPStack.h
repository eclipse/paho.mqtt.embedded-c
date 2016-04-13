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
 *    Rafael de Lucena Valle - add WifiStack for ESP8266 WiFi module
 *******************************************************************************/

#ifndef ESP8266WIFIIPSTACK_H
#define ESP8266WIFIIPSTACK_H

#include <ESP8266WiFi.h>

class WifiIPStack 
{
public:
    WifiIPStack()
    {
        //WiFi.begin();              // Use DHCP
        iface.setTimeout(1000);    // 1 second Timeout 
    }

    int connect(char* hostname, int port)
    {
        return iface.connect(hostname, port);
    }

    int connect(uint32_t hostname, int port)
    {
        return iface.connect(hostname, port);
    }

    int read(unsigned char* buffer, int len, int timeout)
    {
        int count = 0;
        int start = millis();

        while((iface.available() < len) && ((millis() - start) < timeout)) {
            yield();
        }

        count = iface.available();

        if (count > len) {
            count = len;
        }

        return iface.readBytes(buffer, count);
    }

    int write(unsigned char* buffer, int len, int timeout)
    {
        iface.setTimeout(timeout);  
        return iface.write((uint8_t*)buffer, len);
    }

    int disconnect()
    {
        iface.stop();
        return 0;
    }

private:

    WiFiClient iface;
};

#endif
