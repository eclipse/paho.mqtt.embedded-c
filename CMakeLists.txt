cmake_minimum_required(VERSION 3.0.0)
project (embedded-mqtt)
set(CMAKE_BUILD_TYPE Debug)

# Library
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

# Tests
add_executable(test1
"${CMAKE_CURRENT_SOURCE_DIR}/MQTTPacket/test/test1.c")
target_link_libraries(test1 paho-embed-mqtt3c)
