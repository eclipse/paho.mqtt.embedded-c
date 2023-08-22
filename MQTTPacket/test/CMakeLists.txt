PROJECT(mqttpacket-tests)

IF (WIN32)
  SET(MQTT_TEST_BROKER "tcp://mqtt.iotree.co.uk:1883" CACHE STRING "Hostname of a test MQTT broker to use")
  SET(MQTT_TEST_PROXY "tcp://localhost:1883" CACHE STRING "Hostname of the test proxy to use")
  SET(MQTT_SSL_HOSTNAME "mqtt.iotree.co.uk" CACHE STRING "Hostname of a test SSL MQTT broker to use")
  SET(CERTDIR $ENV{APPVEYOR_BUILD_FOLDER}/test/ssl)
ELSE ()
  SET(MQTT_TEST_BROKER "tcp://localhost:1883" CACHE STRING "Hostname of a test MQTT broker to use")
  SET(MQTT_TEST_PROXY "tcp://localhost:1884" CACHE STRING "Hostname of the test proxy to use")
  SET(MQTT_SSL_HOSTNAME "localhost" CACHE STRING "Hostname of a test SSL MQTT broker to use")
  SET(CERTDIR $ENV{TRAVIS_BUILD_DIR}/test/ssl)
ENDIF ()

# transport.h/c are reused from ../samples:
include_directories(../src ../src/V5 ../samples)

# test1 - MQTTv3 serializer test
ADD_EXECUTABLE(
	test1
	test1.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test1 PRIVATE -Wno-deprecated-declarations)

TARGET_LINK_LIBRARIES(
	test1
	paho-embed-mqtt3c
)

ADD_TEST(
	NAME test1
	COMMAND "test1" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test1
	PROPERTIES TIMEOUT 540
)

# test2 - MQTTv5 serializer test
ADD_EXECUTABLE(
	test2
	test2.c
)

TARGET_LINK_LIBRARIES(
	test2
	paho-embed-mqtt5c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test2 PRIVATE -Wno-deprecated-declarations)

ADD_TEST(
	NAME test2
	COMMAND "test2" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test2
	PROPERTIES TIMEOUT 540
)

# test3 - MQTTv3 broker test
ADD_EXECUTABLE(
	test3
	test3.c
	../samples/transport.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test3 PRIVATE -Wno-deprecated-declarations)

TARGET_LINK_LIBRARIES(
	test3
	paho-embed-mqtt3c
)

ADD_TEST(
	NAME test3
	COMMAND "test3" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test3
	PROPERTIES TIMEOUT 540
)

# test4 - MQTTv5 broker test
ADD_EXECUTABLE(
	test4
	test4.c
	../samples/transport.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test4 PRIVATE -Wno-deprecated-declarations)

TARGET_LINK_LIBRARIES(
	test4
	paho-embed-mqtt5c
)

ADD_TEST(
	NAME test4
	COMMAND "test4" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test4
	PROPERTIES TIMEOUT 540
)

# test5 - MQTTv3 side-by-side with MQTTv5 broker test
ADD_EXECUTABLE(
	test5
	test5.c
	../samples/transport.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test5 PRIVATE -Wno-deprecated-declarations)
target_compile_definitions(test5 PRIVATE NOSIGPIPE)

TARGET_LINK_LIBRARIES(
	test5
	paho-embed-mqtt3c
	paho-embed-mqtt5c
)

ADD_TEST(
	NAME test5
	COMMAND "test5" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test5
	PROPERTIES TIMEOUT 540
)
