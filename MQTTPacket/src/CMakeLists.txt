#*******************************************************************************
#  Copyright (c) 2017, 2023 IBM Corp.
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     Ian Craggs - initial version
#*******************************************************************************/

# MQTTPacket Library
file(GLOB SOURCES "*.c" "V3/*.c")
add_library(paho-embed-mqtt3c SHARED ${SOURCES})
install(TARGETS paho-embed-mqtt3c DESTINATION /usr/lib)
target_compile_definitions(paho-embed-mqtt3c PRIVATE MQTT_SERVER MQTT_CLIENT)

add_library(MQTTPacketClient SHARED V3/MQTTFormat MQTTPacket
            MQTTSerializePublish MQTTDeserializePublish
            MQTTConnectClient MQTTSubscribeClient MQTTUnsubscribeClient)
target_compile_definitions(MQTTPacketClient PRIVATE MQTT_CLIENT)

add_library(MQTTPacketServer SHARED V3/MQTTFormat MQTTPacket
            MQTTSerializePublish MQTTDeserializePublish
            MQTTConnectServer MQTTSubscribeServer MQTTUnsubscribeServer)
target_compile_definitions(MQTTPacketServer PRIVATE MQTT_SERVER)

file(GLOB SOURCES5 "*.c" "V5/*.c")

add_library(paho-embed-mqtt5c SHARED ${SOURCES5})
install(TARGETS paho-embed-mqtt5c DESTINATION /usr/lib)
target_compile_definitions(paho-embed-mqtt5c PRIVATE MQTT_SERVER MQTT_CLIENT MQTTV5)

add_library(MQTTPacketClient5 SHARED V5/MQTTV5Format MQTTPacket
            MQTTSerializePublish MQTTDeserializePublish V5/MQTTProperties V5/MQTTV5Packet
            MQTTConnectClient MQTTSubscribeClient MQTTUnsubscribeClient)
target_compile_definitions(MQTTPacketClient5 PRIVATE MQTT_CLIENT MQTTV5)

add_library(MQTTPacketServer5 SHARED V5/MQTTV5Format MQTTPacket
            MQTTSerializePublish MQTTDeserializePublish V5/MQTTProperties V5/MQTTV5Packet
            MQTTConnectServer MQTTSubscribeServer MQTTUnsubscribeServer)
target_compile_definitions(MQTTPacketServer5 PRIVATE MQTT_SERVER MQTTV5)
