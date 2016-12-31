/*
 *******************************************************************************
 *
 * Purpose: Example of using the Arduino MqttClient
 * Project URL: https://github.com/monstrenyatko/ArduinoMqtt
 *
 *******************************************************************************
 * Copyright Oleg Kovalenko 2016.
 *
 * Distributed under the MIT License.
 * (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *******************************************************************************
 */

#include <Arduino.h>
#include <SoftwareSerial.h>

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 1
// Include library
#include <MqttClient.h>


#define LOG_PRINTFLN(fmt, ...)	printfln_P(PSTR(fmt), ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void printfln_P(const char *fmt, ...) {
	char buf[LOG_SIZE_MAX];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf_P(buf, LOG_SIZE_MAX, fmt, ap);
	va_end(ap);
	Serial.println(buf);
}

#define HW_UART_SPEED									57600L
#define MQTT_ID											"TEST-ID"
const char* MQTT_TOPIC_SUB = "test/"MQTT_ID"/sub";
const char* MQTT_TOPIC_PUB = "test/"MQTT_ID"/pub";
MqttClient *mqtt = NULL;

// ============== Object to supply current time ================================
class Time: public MqttClient::Time {
public:
	unsigned long millis() const {
		return ::millis();
	}
} time;

// ============== Object to implement network connectivity =====================
// Current example assumes the network TCP stack is connected using serial
// interface to pins 10(RX) and 11(TX). The SoftwareSerial library is used
// for actual communication.
#define SW_UART_PIN_RX								10
#define SW_UART_PIN_TX								11
#define SW_UART_SPEED								9600L
class Network {
public:
	Network() {
		mNet = new SoftwareSerial(SW_UART_PIN_RX, SW_UART_PIN_TX);
		mNet->begin(SW_UART_SPEED);
	}

	int connect(const char* hostname, int port) {
		// TCP connection is already established otherwise do it here
		return 0;
	}

	int read(unsigned char* buffer, int len, unsigned long timeoutMs) {
		unsigned long startMs = time.millis();
		mNet->setTimeout(timeoutMs);
		do {
			int qty = mNet->readBytes((char*) buffer, len);
			if (qty > 0) {
				return qty;
			}
		} while(time.millis() - startMs < timeoutMs);
		return 0;
	}

	int write(unsigned char* buffer, int len, unsigned long timeoutMs) {
		mNet->setTimeout(timeoutMs);
		for (int i = 0; i < len; ++i) {
			mNet->write(buffer[i]);
		}
		mNet->flush();
		return len;
	}

	int disconnect() {
		// Implement TCP network disconnect here
		return 0;
	}

private:
	SoftwareSerial										*mNet;
} *network = NULL;

// ============== Setup all objects ============================================
void setup() {
	// Setup hardware serial for logging
	Serial.begin(HW_UART_SPEED);
	while (!Serial);
	// Setup network
	network = new Network;
	// Setup MqttClient
	MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
	MqttClient::Network * mqttNetwork = new MqttClient::NetworkImpl<Network>(*network, time);
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
	mqtt = new MqttClient (
		mqttOptions, *mqttLogger, time, *mqttNetwork, *mqttSendBuffer,
		*mqttRecvBuffer, *mqttMessageHandlers
	);
}

// ============== Subscription callback ========================================
void processMessage(MqttClient::MessageData& md) {
	const MqttClient::Message& msg = md.message;
	char payload[msg.payloadLen + 1];
	memcpy(payload, msg.payload, msg.payloadLen);
	payload[msg.payloadLen] = '\0';
	LOG_PRINTFLN(
		"Message arrived: qos %d, retained %d, dup %d, packetid %d, payload:[%s]",
		msg.qos, msg.retained, msg.dup, msg.id, payload
	);
}

// ============== Main loop ====================================================
void loop() {
	// Check connection status
	if (!mqtt->isConnected()) {
		// Re-establish TCP connection with MQTT broker
		network->disconnect();
		network->connect("mymqttserver.com", 1883);
		// Start new MQTT connection
		LOG_PRINTFLN("Connecting");
		MqttClient::ConnectResult connectResult;
		// Connect
		{
			MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
			options.MQTTVersion = 4;
			options.clientID.cstring = (char*)MQTT_ID;
			options.cleansession = true;
			options.keepAliveInterval = 15; // 15 seconds
			MqttClient::Error::type rc = mqtt->connect(options, connectResult);
			if (rc != MqttClient::Error::SUCCESS) {
				LOG_PRINTFLN("Connection error: %i", rc);
				return;
			}
		}
		// Subscribe
		{
			MqttClient::Error::type rc = mqtt->subscribe(
				MQTT_TOPIC_SUB, MqttClient::QOS0, processMessage
			);
			if (rc != MqttClient::Error::SUCCESS) {
				LOG_PRINTFLN("Subscribe error: %i", rc);
				LOG_PRINTFLN("Drop connection");
				mqtt->disconnect();
				return;
			}
		}
	} else {
		// Publish
		{
			const char* buf = "Hello";
			MqttClient::Message message;
			message.qos = MqttClient::QOS0;
			message.retained = false;
			message.dup = false;
			message.payload = (void*) buf;
			message.payloadLen = strlen(buf) + 1;
			mqtt->publish(MQTT_TOPIC_PUB, message);
		}
		// Idle for 30 seconds
		mqtt->yield(30000L);
	}
}
