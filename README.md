ArduinoMqtt
===========

[![Build Status](https://travis-ci.org/monstrenyatko/ArduinoMqtt.svg?branch=master)](https://travis-ci.org/monstrenyatko/ArduinoMqtt)


About
=====

[MQTT](http://mqtt.org) Client library for Arduino based on the
[Eclipse Paho](https://github.com/eclipse/paho.mqtt.embedded-c) project.
This library bundles the `C/C++ MQTTPacket` library of the Eclipse Paho project
with simple synchronous `C++ MQTT Client` implementation to get the Arduino like API.


Features
========

- Support of `MQTT` `QoS0`, `QoS1` and simplified `QoS2`
- Synchronous implementation. The method `yield` must be called regularly to allow
incoming message processing and to send keep-alive packets in time
- `TCP` network communication is out of the library scope.
`MqttClient::Network` object must be provided with `read` and `write` implementations
- System current time and other functions are provided externally by `MqttClient::System` object
to allow easy adaptation to any other environments
- External logger. `MqttClient::Logger` object is used to print logs with any
convenient way for particular environment
- No heap memory allocations.
All required resources must be provided at the moment of `MqttClient` construction.
It allows the full control and limiting of the used resources externally:
    * `MqttClient::Buffer` objects are used as send/receive temporary buffers
    * `MqttClient::MessageHandlers` object is used as storage of subscription
    callback functions
- Idle interval calculator (See `getIdleInterval` method). Could be very useful If
you going to put board/radio into low-power mode between data transmissions


Usage
=====

See full `Arduino` [example](examples/PubSub/PubSub.ino) that covers Publish
and Subscribe logic.

Debug output
------------
Logging is disabled by default. Define `MQTT_LOG_ENABLED` equal `1` to enable.

If you can't add the define using compiler options (in case of Arduino IDE) just
define it before including the library header:
```c++
#define MQTT_LOG_ENABLED 1
#include <MqttClient.h>
```

External Resources
------------------

#### Logger

Provide class implementing the `MqttClient::Logger` interface.
Alternatively just instantiate the `MqttClient::LoggerImpl` template class that
allows direct use of the `Arduino` `HardwareSerial` class or any other object with
implemented `void println(const char*)` method:
```c++
#define HW_UART_SPEED									57600L
// Setup hardware serial for logging
Serial.begin(HW_UART_SPEED);
while (!Serial);
// Create MqttClient logger
MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
```

#### System resources

Provide class implementing the `MqttClient::System` interface to allow library
access to the system resources.

##### millis

Used to access the system time.
The implementation must return the current time in milliseconds.

`Arduino` simple implementation:
```c++
class System: public MqttClient::System {
public:
	...
	unsigned long millis() const {
		return ::millis();
	}
	...
}
```

General `C++` implementation might be like:
```c++
#include <chrono>

class TestSystem: public MqttClient::System {
	...
	unsigned long millis() const {
		return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	...
}
```

##### yield [optional]

Used to access the system `yield`.

Method is called regularly while long waits.
Some systems like ESP requires calling of the `yield` regularly to:
- maintain WiFi connection
- reset watchdog timer

`ESP8266` simple implementation:
```c++
class System: public MqttClient::System {
public:
	...
	void yield(void) {
		::yield();
	}
	...
}
```

#### Network

Provide class implementing the `MqttClient::Network` interface.

##### Custom helper

Instantiate the `MqttClient::NetworkImpl` template class that
allows usage of any class with implemented methods like:
- `int read(unsigned char* buffer, int len, unsigned long timeoutMs)`
- `int write(unsigned char* buffer, int len, unsigned long timeoutMs)`

Proposed example assumes that `TCP` stack is connected using serial interface to
`Arduino` pins 10(RX) and 11(TX). The `SoftwareSerial` library is used for actual
communication:
```c++
#define SW_UART_PIN_RX								10
#define SW_UART_PIN_TX								11
#define SW_UART_SPEED								9600L

class Network {
public:
	Network() {
		mNet = new SoftwareSerial(SW_UART_PIN_RX, SW_UART_PIN_TX);
		mNet->begin(SW_UART_SPEED);
	}

	int read(unsigned char* buffer, int len, unsigned long timeoutMs) {
		mNet->setTimeout(timeoutMs);
		return mNet->readBytes((char*) buffer, len);
	}

	int write(unsigned char* buffer, int len, unsigned long timeoutMs) {
		mNet->setTimeout(timeoutMs);
		for (int i = 0; i < len; ++i) {
			mNet->write(buffer[i]);
		}
		mNet->flush();
		return len;
	}

private:
	SoftwareSerial										*mNet;
}

MqttClient::System *mqttSystem = new System;
MqttClient::Network *mqttNetwork = new MqttClient::NetworkImpl<Network>(*network, *mqttSystem);
```

##### Arduino Client helper

Another alternative allows using of `Arduino Client` compatible network classes
like `EthernetClient` or `WiFiClient` of `ESP8266` etc...
To have this just instantiate the `MqttClient::NetworkClientImpl`template class:
```c++
EthernetClient network;
MqttClient::System *mqttSystem = new System;
MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<Client>(network, *mqttSystem);
```

#### Buffers

Provide two buffers the one for transmitting message and another one for receiving
message processing. Buffer is a class implementing the `MqttClient::Buffer` interface.
Use the `MqttClient::ArrayBuffer` template class to get simplest implementation
based on `C` array:
```c++
// Make 128 bytes send buffer
MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
// Make 128 bytes receive buffer
MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
```

#### Message Handlers storage

Provide class implementing the `MqttClient::MessageHandlers` interface to keep
subscription callback functions.

##### Simple helper

Instantiate the `MqttClient::MessageHandlersImpl` template
class to get simplest implementation based on `C` array:
```c++
// Allows up to 2 subscriptions simultaneously
MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
```
This implementation does not copy the topic string and keeps only pointer.
Please ensure the pointer remains valid while topic is subscribed.
This is preferred implementation because of minimal resources requirements.

##### Static helper

Instantiate the `MqttClient::MessageHandlersStaticImpl` template class:
```c++
// Allows up to 2 subscriptions simultaneously
// Allows up to 64 bytes for topic string including 1 byte for string termination symbol
MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersStaticImpl<2, 64>();
```
This implementation preallocates buffers to store topic strings in advance.
This implementation does copy the topic string.
This is preferred implementation when the maximum topic length could be predicted
because it avoids the heap allocations at the moment of `subscribe` call.

##### Dynamic helper

Instantiate the `MqttClient::MessageHandlersDynamicImpl` template class:
```c++
// Allows up to 2 subscriptions simultaneously
MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersDynamicImpl<2>();
```
This implementation allocates buffer to store topic string at the moment of `subscribe` call.
This implementation does copy the topic string.

Development
===========

Eclipse IDE (PlatformIO Core)
-----------------------------

See [guide](http://docs.platformio.org/en/latest/ide/eclipse.html).

1. Install Eclipse IDE for C/C++ Developers (With CDT plugin).
2. Install `CDT Cross GCC Built-in Compiler Settings` package using
Eclipse `Install New Software`.
3. Go to project directory:

	```sh
	cd <path to sources>
	```
4. Use `pio` to generate project:

	```sh
	pio init --ide eclipse --board <ID>
	```
5. Import the project (`File -> Import -> General -> Existing Projects into Workspace`)
6. In case of problems with `C` files indexing like some types are not resolved
but the definition is definitely available in the included header Need to update
the `Language Mappings` in `Eclipse Preferences` -> `C/C++` -> `Language Mappings`.
Add next mappings:
	- `C Header file` to `C++`
	- `C Source file` to `C++`


Useful links
============

- MQTT 3.1.1 [protocol](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)

