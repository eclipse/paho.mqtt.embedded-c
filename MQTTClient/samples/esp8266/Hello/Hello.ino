/*******************************************************************************
 * Copyright (c) 2016 IBM Corp. and others
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Benjamin Cabe - adapt to IPStack, and add Yun instructions
 *    Rafael de Lucena Valle - adapt WifiStack to ESP8266 WiFi module
 *******************************************************************************/

#define MQTTCLIENT_QOS2 1

#include <WifiIPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

const char* topic = "esp8266-sample";
char printbuf[100];
int arrivedcount = 0;

WifiIPStack ipstack;
MQTT::Client<WifiIPStack, Countdown> client = MQTT::Client<WifiIPStack, Countdown>(ipstack);

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;

    sprintf(printbuf, "Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n",
            ++arrivedcount, message.qos, message.retained, message.dup, message.id);
    Serial.print(printbuf);
    sprintf(printbuf, "Payload %s\n", (char*)message.payload);
    Serial.print(printbuf);
}

void wifi_connect()
{
    char ssid[] = "your.wifi.ssid";
    char password[] = "your.wifi.password";
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void connect()
{
    char hostname[] = "iot.eclipse.org";
    int port = 1883;
    sprintf(printbuf, "Connecting to %s:%d\n", hostname, port);
    Serial.print(printbuf);
    int rc = ipstack.connect(hostname, port);
    if (rc != 1)
    {
        sprintf(printbuf, "rc from TCP connect is %d\n", rc);
        Serial.print(printbuf);
    }

    Serial.println("MQTT connecting");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*)"esp8266-sample";
    rc = client.connect(data);
    if (rc != 0)
    {
        sprintf(printbuf, "rc from MQTT connect is %d\n", rc);
        Serial.print(printbuf);
    }
    Serial.println("MQTT connected");

    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0)
    {
        sprintf(printbuf, "rc from MQTT subscribe is %d\n", rc);
        Serial.print(printbuf);
    }
    Serial.println("MQTT subscribed");
}

void setup()
{
    Serial.begin(9600);
    Serial.println("MQTT Hello example");
    wifi_connect();
    connect();
}

void loop()
{
    if (!client.isConnected())
        connect();

    MQTT::Message message;

    arrivedcount = 0;

    // Send and receive QoS 0 message
    char buf[100];
    sprintf(buf, "Hello World! QoS 0 message");
    Serial.println(buf);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client.publish(topic, message);
    while (arrivedcount == 0)
        client.yield(1000);

    // Send and receive QoS 1 message
    sprintf(buf, "Hello World!  QoS 1 message");
    Serial.println(buf);
    message.qos = MQTT::QOS1;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    while (arrivedcount == 1)
        client.yield(1000);

    // Send and receive QoS 2 message
    sprintf(buf, "Hello World!  QoS 2 message");
    Serial.println(buf);
    message.qos = MQTT::QOS2;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    while (arrivedcount == 2)
        client.yield(1000);

    delay(2000);
}
