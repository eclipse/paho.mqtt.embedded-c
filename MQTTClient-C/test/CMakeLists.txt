PROJECT(mqtt-tests)

SET(MQTT_TEST_BROKER "tcp://localhost:1883" CACHE STRING "Hostname of a test MQTT broker to use")
SET(MQTT_TEST_PROXY "tcp://localhost:1884" CACHE STRING "Hostname of the test proxy to use")
SET(MQTT_SSL_HOSTNAME "localhost" CACHE STRING "Hostname of a test SSL MQTT broker to use")
SET(CERTDIR $ENV{TRAVIS_BUILD_DIR}/test/ssl)

ADD_EXECUTABLE(
	testc1
	test1.c
)

target_link_libraries(testc1 paho-embed-mqtt3cc paho-embed-mqtt3c)
target_include_directories(testc1 PRIVATE "../src" "../src/linux")
target_compile_definitions(testc1 PRIVATE MQTTCLIENT_PLATFORM_HEADER=MQTTLinux.h)

ADD_TEST(
	NAME testc1
	COMMAND "testc1" "--connection" ${MQTT_TEST_BROKER}
)
