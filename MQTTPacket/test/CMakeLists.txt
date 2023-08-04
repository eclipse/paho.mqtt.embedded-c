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

include_directories(../src ../src/V5 ../samples)

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

ADD_EXECUTABLE(
	test15
	test1.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test15 PRIVATE -Wno-deprecated-declarations)

# To allow existing MQTTv3 applications using the MQTTv5 library, MQTTV5 structures must be used.
target_compile_definitions(test15 PRIVATE MQTTV5)

TARGET_LINK_LIBRARIES(
	test15
	paho-embed-mqtt5c
)

ADD_TEST(
	NAME test15
	COMMAND "test15" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test15
	PROPERTIES TIMEOUT 540
)

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


ADD_EXECUTABLE(
	test3
	test3.c
)

# Disabling deprecation warnings:
#  warning: ‘ftime’ is deprecated [-Wdeprecated-declarations]
target_compile_options(test3 PRIVATE -Wno-deprecated-declarations)

TARGET_LINK_LIBRARIES(
	test3
	paho-embed-mqtt5c
)

ADD_TEST(
	NAME test3
	COMMAND "test3" "--connection" ${MQTT_TEST_BROKER}
)

SET_TESTS_PROPERTIES(
	test3
	PROPERTIES TIMEOUT 540
)