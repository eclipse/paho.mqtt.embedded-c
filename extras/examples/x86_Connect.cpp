#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <iomanip>
#include <chrono>

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 1
// Include library
#include <MqttClient.h>

// ============== Object to supply current time ================================
class TestSystem: public MqttClient::System {
	unsigned long millis() const {
		return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count();
	}
};

// ============== Object to implement logging ==================================
class TestLogger {
public:
	void println(const char* v) {
		std::cout<<v<<std::endl;
	}
};

// ============== Object to implement network connectivity =====================
class TestNetwork {

	int sockfd = -1;
public:
	TestNetwork() {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		char buffer[256];
		if (sockfd < 0) {
			std::cerr << "Can't open socket" << std::endl;
			exit(0);
		}
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
		if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			std::cerr << "Can't set socket RECV timeout" << std::endl;
			exit(0);
		}
		if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			std::cerr << "Can't set socket SEND timeout" << std::endl;
			exit(0);
		}
		const char* hostName = "test.mosquitto.org";
		int portno = 1883;
		std::cout << "Connecting to " << hostName << ":" << portno << std::endl;
		hostent *server = gethostbyname(hostName);
		if (server == NULL) {
			std::cerr << "No such host" << std::endl;
			exit(0);
		}
		sockaddr_in serv_addr;
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr,
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length
		);
		serv_addr.sin_port = htons(portno);
		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
			std::cerr << "Can't connect" << std::endl;
			exit(0);
		}
	}

	~TestNetwork() {
		close(sockfd);
	}

	int read(unsigned char* buffer, int len, unsigned long timeoutMs) {
		return ::read(sockfd, buffer, len);
	}

	int write(unsigned char* buffer, int len, unsigned long timeoutMs) {
		return ::write(sockfd, buffer, len);
	}
};

int main(int argc, char** argv) {
	// Init
	TestSystem testSystem;
	TestLogger testLogger;
	TestNetwork testNetwork;
	MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<TestLogger>(testLogger);
	MqttClient::Network *mqttNetwork = new MqttClient::NetworkImpl<TestNetwork>(testNetwork, testSystem);
	MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
	MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
	MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<1>();
	MqttClient::Options options;
	options.commandTimeoutMs = 5000; // Set command timeout to 10 seconds
	MqttClient *mqtt = new MqttClient (options, *mqttLogger, testSystem, *mqttNetwork, *mqttSendBuffer, *mqttRecvBuffer, *mqttMessageHandlers);
	// Connect
	MqttClient::ConnectResult connectResult;
	MQTTPacket_connectData connectOptions = MQTTPacket_connectData_initializer;
	connectOptions.MQTTVersion = 4;
	connectOptions.clientID.cstring = (char*)"TEST-ID";
	connectOptions.cleansession = true;
	connectOptions.keepAliveInterval = 10; // 10 seconds
	MqttClient::Error::type rc = mqtt->connect(connectOptions, connectResult);
	if (rc != MqttClient::Error::SUCCESS) {
		std::cout << "Connection error: " << rc << std::endl;
	}
	// Rest and process keep-alives for 20 seconds
	mqtt->yield(20000);
	// Disconnect
	mqtt->disconnect();
	return 0;
}

