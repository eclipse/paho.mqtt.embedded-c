/*
 *******************************************************************************
 *
 * Purpose: Example of using the Arduino MqttClient LWT with Esp8266WiFiClient.
 * Project URL: https://github.com/monstrenyatko/ArduinoMqtt
 *
 *******************************************************************************
 * Copyright Oleg Kovalenko 2017.
 *
 * Distributed under the MIT License.
 * (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *******************************************************************************
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 1
// Include library
#include <MqttClient.h>


#define LOG_PRINTFLN(fmt, ...)	logfln(fmt, ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void logfln(const char *fmt, ...) {
	char buf[LOG_SIZE_MAX];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, LOG_SIZE_MAX, fmt, ap);
	va_end(ap);
	Serial.println(buf);
}

#define HW_UART_SPEED									115200L
#define MQTT_ID											"TEST-ID"
char MQTT_TOPIC_STATUS[] = "test/" MQTT_ID "/status";

static MqttClient *mqtt = NULL;
static WiFiClient network;

// ============== Object to supply system functions ============================
class System: public MqttClient::System {
public:

	unsigned long millis() const {
		return ::millis();
	}

	void yield(void) {
		::yield();
	}
};

// ============== Setup all objects ============================================
void setup() {
	// Setup hardware serial for logging
	Serial.begin(HW_UART_SPEED);
	while (!Serial);

	// Setup WiFi network
	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	WiFi.hostname("ESP_" MQTT_ID);
	WiFi.begin("ssid", "passphrase");
	LOG_PRINTFLN("\n");
	LOG_PRINTFLN("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		LOG_PRINTFLN(".");
	}
	LOG_PRINTFLN("Connected to WiFi");
	LOG_PRINTFLN("IP: %s", WiFi.localIP().toString().c_str());

	// Setup MqttClient
	MqttClient::System *mqttSystem = new System;
	MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
	MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(network, *mqttSystem);
	//// Make 128 bytes send buffer
	MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
	//// Make 128 bytes receive buffer
	MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
	//// Allow up to 2 subscriptions simultaneously
	MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
	//// Configure client options
	MqttClient::Options mqttOptions;
	////// Set command timeout to 10 seconds
	mqttOptions.commandTimeoutMs = 10000;
	//// Make client object
	mqtt = new MqttClient(
		mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
		*mqttRecvBuffer, *mqttMessageHandlers
	);
}

// ============== Main loop ====================================================
void loop() {
	LOG_PRINTFLN("Connecting");
	// Start TCP connection
	network.connect("test.mosquitto.org", 1883);
	if (!network.connected()) {
		LOG_PRINTFLN("Can't establish the TCP connection");
		delay(5000);
		ESP.reset();
	}
	// Start MQTT connection
	{
		MqttClient::ConnectResult connectResult;
		MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
		options.MQTTVersion = 4;
		options.clientID.cstring = (char*)MQTT_ID;
		options.cleansession = true;
		options.keepAliveInterval = 15; // 15 seconds
		// Setup LWT
		options.willFlag = true;
		options.will.topicName.cstring = MQTT_TOPIC_STATUS;
		options.will.message.cstring = "I am disconnected";
		options.will.retained = true;
		options.will.qos = MqttClient::QOS0;
		MqttClient::Error::type rc = mqtt->connect(options, connectResult);
		if (rc != MqttClient::Error::SUCCESS) {
			LOG_PRINTFLN("Connection error: %i", rc);
			delay(5000);
			ESP.reset();
		}
	}
	// Publish `connected` message
	{
		const char* buf = "I am connected";
		MqttClient::Message message;
		message.qos = MqttClient::QOS0;
		message.retained = true;
		message.dup = false;
		message.payload = (void*) buf;
		message.payloadLen = strlen(buf) + 1;
		MqttClient::Error::type rc = mqtt->publish(MQTT_TOPIC_STATUS, message);
		if (rc != MqttClient::Error::SUCCESS) {
			LOG_PRINTFLN("Publish error: %i", rc);
			delay(5000);
			ESP.reset();
		}
	}
	LOG_PRINTFLN("Status is Connected");
	// Some delay
	delay(5000);
	// Disconnect the network and wait for some time until the LWT is triggered by the broker
	network.stop();
	LOG_PRINTFLN("Status will be changed to Disconnected by the broker");
	delay(10000);
	// Start from the beginning
	ESP.reset();
}
