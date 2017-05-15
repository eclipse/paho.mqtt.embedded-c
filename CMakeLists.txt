cmake_minimum_required(VERSION 3.0.0)
project (embedded-mqtt)
set(CMAKE_BUILD_TYPE Debug)

# MQTTPacket Library
include_directories("MQTTPacket/src")
file(GLOB SOURCES "MQTTPacket/src/*.c")
add_library(paho-embed-mqtt3c SHARED ${SOURCES})
install(TARGETS paho-embed-mqtt3c DESTINATION /usr/lib)

# Samples
add_executable(pub0sub1 
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/pub0sub1.c"
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/transport.c")
target_link_libraries(pub0sub1 paho-embed-mqtt3c)

add_executable(pub0sub1_nb 
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/pub0sub1_nb.c"
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/transport.c")
target_link_libraries(pub0sub1_nb paho-embed-mqtt3c)

add_executable(qos0pub
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/qos0pub.c"
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/samples/transport.c")
target_link_libraries(qos0pub paho-embed-mqtt3c)

# MQTTClient Library - C++
include_directories("MQTTPacket/src", "MQTTClient/src", "MQTTClient/src/linux")
add_executable(hello
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTClient/samples/linux/hello.cpp")
target_link_libraries(hello paho-embed-mqtt3c)

include_directories("MQTTPacket/src", "MQTTClient/src", "MQTTClient/src/linux")
add_executable(stdoutsub
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTClient/samples/linux/stdoutsub.cpp")
target_link_libraries(stdoutsub paho-embed-mqtt3c)

# MQTTClient Library - C
include_directories("MQTTPacket/src", "MQTTClient-C/src", "MQTTClient-C/src/linux")
ADD_DEFINITIONS(-DMQTTCLIENT_PLATFORM_HEADER=MQTTLinux.h)
file(GLOB SOURCES "MQTTClient-C/src/*.c" "MQTTClient-C/src/linux/*.c")
add_library(paho-embed-mqtt3cc SHARED ${SOURCES})
install(TARGETS paho-embed-mqtt3cc DESTINATION /usr/lib)

add_executable(stdoutsubc
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTClient-C/samples/linux/stdoutsub.c")
target_link_libraries(stdoutsubc paho-embed-mqtt3cc paho-embed-mqtt3c)


ENABLE_TESTING()

ADD_SUBDIRECTORY(MQTTPacket/test)

