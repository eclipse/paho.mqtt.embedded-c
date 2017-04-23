/*
 *******************************************************************************
 *
 * Purpose: Simple Synchronous MQTT Client implementation.
 * Based on Eclipse Paho MQTT Client (https://github.com/eclipse/paho.mqtt.embedded-c)
 *
 *******************************************************************************
 * Copyright Oleg Kovalenko 2016.
 *
 * Distributed under the MIT License.
 * (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *******************************************************************************
 */

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

/* External Includes */
#include <stdarg.h>
/* Internal Includes */
#include "MQTTPacket/MQTTPacket.h"


#define MQTT_LOG_SIZE_MAX								128

#ifndef MQTT_LOG_ENABLED
	#define MQTT_LOG_ENABLED							0
#endif

#ifdef __AVR__
	#define MQTT_PSTR(s)								PSTR(s)
	#define MQTT_PSTR_ENABLED							1
#else
	#define MQTT_PSTR(s)								s
	#define MQTT_PSTR_ENABLED							0
#endif

#if MQTT_LOG_ENABLED
	#define MQTT_LOG_PRINTFLN(fmt, ...)	mLogger.printfln_P(MQTT_PSTR("MQTT - " fmt), ##__VA_ARGS__)
#else
	#define MQTT_LOG_PRINTFLN(fmt, ...)	((void)0)
#endif


class MqttClient {

	const unsigned long									NET_MAX_TM_MS = 1000L;
	const unsigned long									NET_MIN_TM_MS = 100L;

public:

	class Logger {
		public:
			virtual ~Logger() {}
			/** Logs the specified string */
			virtual void println(const char*) = 0;
		private:
			friend class MqttClient;

			void printfln_P(const char *fmt, ...) {
				char buf[MQTT_LOG_SIZE_MAX];
				va_list ap;
				va_start(ap, fmt);
#if MQTT_PSTR_ENABLED
				vsnprintf_P(buf, MQTT_LOG_SIZE_MAX, fmt, ap);
#else
				vsnprintf(buf, MQTT_LOG_SIZE_MAX, fmt, ap);
#endif
				va_end(ap);
				println(buf);
			}
	};

	template<class Log>
	class LoggerImpl: public Logger {
		public:
			LoggerImpl(Log &logger): mLogger(logger) {}
			void println(const char* v) {mLogger.println(v);}
		protected:
			Log											&mLogger;
	};

	class System {
		public:
			virtual ~System() {}

			/** Returns the current time in milliseconds. */
			virtual unsigned long millis() const = 0;

			/** Calls the background system functions.
			 *
			 * It will be called regularly while long waits.
			 * Some systems like ESP requires calling the `yield` regularly.
			 * Implement this method with all required actions.
			 */
			virtual void yield(void) {}
	};

	class Network {
		public:
			virtual ~Network() {}

			/**
			 * Reads the specified number of bytes from network.
			 *
			 * @param buffer - array to store received bytes
			 * @param len - desired/maximum number of bytes to read
			 * @param timeoutMs - time to wait bytes from network, in milliseconds
			 * @return number of received bytes
			 */
			virtual int read(unsigned char* buffer, int len, unsigned long timeoutMs) = 0;

			/**
			 * Writes the specified number of bytes to network.
			 *
			 * @param buffer - array of the source bytes
			 * @param len - number of bytes to write
			 * @param timeoutMs - maximum time of bytes processing by network, in milliseconds
			 * @return number of written bytes
			 */
			virtual int write(unsigned char* buffer, int len, unsigned long timeoutMs) = 0;
	};

	template<class Net>
	class NetworkImpl: public Network {
		public:
			NetworkImpl(Net &network, System& system): mNetwork(network), mSystem(system) {}

			int read(unsigned char* buffer, int len, unsigned long timeoutMs) {
				Timer timer(mSystem, timeoutMs);
				int qty = 0;
				while(!timer.expired() && qty<len) {
					mSystem.yield();
					int tmpRes = mNetwork.read(buffer+qty, len-qty, timer.leftMs());
					if (tmpRes > 0) {
						qty+=tmpRes;
					}
				}
				return qty;
			}

			int write(unsigned char* buffer, int len, unsigned long timeoutMs) {
				mSystem.yield();
				return mNetwork.write(buffer, len, timeoutMs);
			}
		private:
			Net											&mNetwork;
			System										&mSystem;
	};

	template<class Client>
	class NetworkClientImpl: public Network {
		public:
		NetworkClientImpl(Client &client, System& system): mClient(client), mSystem(system) {}

			int read(unsigned char* buffer, int len, unsigned long timeoutMs) {
				Timer timer(mSystem, timeoutMs);
				int qty = 0;
				while(!timer.expired() && qty<len) {
					mSystem.yield();
					int tmpRes = mClient.read((uint8_t*)(buffer+qty), len-qty);
					if (tmpRes > 0) {
						qty+=tmpRes;
					}
				}
				return qty;
			}

			int write(unsigned char* buffer, int len, unsigned long timeoutMs) {
				mSystem.yield();
				mClient.setTimeout(timeoutMs);
				return mClient.write((const uint8_t*)buffer, len);
			}
		private:
			Client										&mClient;
			System										&mSystem;
	};

	struct Buffer {
		/** Gets pointer to array */
		virtual unsigned char* get() = 0;
		/** Gets the array size */
		virtual int size() const = 0;
	};

	template<int BUFFER_SIZE = 100>
	struct ArrayBuffer: public Buffer {
			unsigned char* get() {return buf;}
			int size() const {return BUFFER_SIZE;}
		private:
			unsigned char								buf[BUFFER_SIZE];
	};

	struct Options {
		/** Maximum time of the MQTT message transmission, in milliseconds.
		 * Also used as delay of keep-alive messages in case of reception timer
		 * expiration while another message has been just sent.
		 */
		unsigned long									commandTimeoutMs = 5000;
	};

	enum QoS {QOS0, QOS1, QOS2};

	struct Error {
		typedef int										type;
		enum {
			BUFFER_OVERFLOW = -8,
			DECODING_FAILURE_REM_LENGHT = -7,
			WAIT_TIMEOUT = -6,
			NETWORK_FAILURE = -5,
			DECODING_FAILURE = -4,
			ENCODING_FAILURE = -3,
			REFUSED = -2,
			FAILURE = -1,
			SUCCESS = 0
		};
	};

	struct Message {
		QoS												qos;
		bool											retained;
		bool											dup;
		unsigned short									id;
		void											*payload;
		int												payloadLen;
	};

	struct MessageData {
		MessageData(MQTTString &aTopicName, struct Message &aMessage)
		: topicName(aTopicName), message(aMessage) {}

		MQTTString										&topicName;
		Message											&message;
	};

	typedef void (*MessageHandlerCbk)(MessageData&);

	struct MessageHandler {
		const char										*topic = NULL;
		MessageHandlerCbk								cbk = NULL;

		/** Checks handler association with the topic/callback. True if associated */
		bool isUsed() const {
			return topic != NULL;
		}

		/** Removes topic/callback association */
		void reset() {
			topic = NULL;
			cbk = NULL;
		}
	};

	class MessageHandlers {
		public:
			virtual ~MessageHandlers() {}
			/** Gets the size of handlers array */
			virtual int size() const = 0;
			/** Checks empty slot availability. True if not available */
			virtual bool isFull() const = 0;
			/** Gets array of handlers. Used and not used */
			virtual MessageHandler* get() = 0;
			/** Sets/Updates handler by topic */
			virtual bool set(const char* topic, MessageHandlerCbk handler) = 0;
			/** Removes handler by topic */
			virtual void reset(const char* topic) = 0;
			/** Removes all handlers */
			virtual void reset() = 0;
	};

	template<int SIZE = 5>
	class MessageHandlersImpl: public MessageHandlers {
		public:
			int size() const {return SIZE;}

			bool isFull() const {
				for (int i = 0; i < size(); ++i) {
					if (!handlers[i].isUsed()) {
						return false;
					}
				}
				return true;
			}

			MessageHandler* get() {return handlers;}

			bool set(const char* topic, MessageHandlerCbk handler) {
				bool res = false;
				int emptyIdx = -1;
				// Try to update existing handler with the same topic
				for (int i = 0; i < size(); ++i) {
					if (handlers[i].isUsed()) {
						if (strcmp(handlers[i].topic, topic) == 0) {
							// Replace
							handlers[i].topic = topic;
							handlers[i].cbk = handler;
							res = true;
							break;
						}
					} else if (emptyIdx < 0) {
						// Store empty slot index
						emptyIdx = i;
					}
				}
				// Check result and try to use empty slot if available
				if (!res && emptyIdx >= 0) {
					// Set to the first empty slot
					handlers[emptyIdx].topic = topic;
					handlers[emptyIdx].cbk = handler;
					res = true;
				}
				return res;
			}

			void reset(const char* topic) {
				for (int i = 0; i < size(); ++i) {
					if (handlers[i].isUsed() && strcmp(handlers[i].topic, topic) == 0) {
						handlers[i].reset();
						break;
					}
				}
			}

			void reset() {
				for (int i = 0; i < size(); ++i) {
					if (handlers[i].isUsed()) {
						handlers[i].reset();
					}
				}
			}

		private:
			MessageHandler								handlers[SIZE];
	};

	struct ConnectResult {
		struct Code {
			typedef unsigned char						type;
			enum {
				SUCCESS = 0,
				REFUSED_PROTO_VER,
				REFUSED_ID,
				REFUSED_SRV_UNV,
				REFUSED_USR_PSWD,
				REFUSED_AUTH
			};
		};
		Code::type										code;
		bool											sessionPresent;
	};

	/**
	 * Constructs the client.
	 *
	 * @param options - the client options
	 * @param logger - the logger used for printing logs if MQTT_LOG_ENABLED
	 *     set to 1, see LoggerImpl
	 * @param system - used to access system functions like time, yield, etc...
	 * @param network - used to send/receive data to/from broker, see NetworkImpl
	 * @param sendBuffer - buffer to temporarily store the transmitted message, see ArrayBuffer
	 * @param recvBuffer - buffer to temporarily store the received message, see ArrayBuffer
	 * @param messageHandlers - storage for subscription callback functions, see MessageHandlersImpl
	 */
	MqttClient(const Options& options, Logger& logger, System& system, Network& network,
		Buffer& sendBuffer, Buffer& recvBuffer, MessageHandlers& messageHandlers)
	: mOptions(options), mLogger(logger), mSystem(system), mNetwork(network),
	  mSendBuffer(sendBuffer), mRecvBuffer(recvBuffer), mMessageHandlers(messageHandlers),
	  mSession(system)
	{}

	/**
	 * Checks the connection status.
	 *
	 * @return true if the client is connected
	 */
	bool isConnected() {
		return mSession.isConnected;
	}

	/**
	 * Connects to the broker.
	 *
	 * @param connectOptions - connect options
	 * @param result - connect result
	 * @return execution status code
	 */
	Error::type connect(const MQTTPacket_connectData& connectOptions, ConnectResult& result) {
		Timer timer(mSystem, mOptions.commandTimeoutMs);
		MQTT_LOG_PRINTFLN("Connect, clean-session: %u, ts: %lu", connectOptions.cleansession, mSystem.millis());
		if (isConnected()) {
			return Error::REFUSED;
		}
		mSession.reset();
		// Make actual keep-alive timer slightly shorter to avoid expiration on broker side
		mSession.keepAliveTmSec = adjustKeepaliveTm(connectOptions.keepAliveInterval);
		int len = 0;
		if ((len = MQTTSerialize_connect(mSendBuffer.get(), mSendBuffer.size(), &(MQTTPacket_connectData&)connectOptions)) <= 0) {
			return Error::ENCODING_FAILURE;
		}
		Error::type rc = sendPacket(len, timer);
		if (rc != Error::SUCCESS) {
			MQTT_LOG_PRINTFLN("Can't send connect, rc: %i", rc);
			return rc;
		}
		rc = waitFor(CONNACK, timer);
		if (rc == Error::SUCCESS) {
			MQTT_LOG_PRINTFLN("Connect ack received");
			unsigned char ackRc = 255;
			unsigned char ackSessionPresent = 0;
			if (MQTTDeserialize_connack(&ackSessionPresent, &ackRc, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
				return Error::DECODING_FAILURE;
			}
			result.code = ackRc;
			result.sessionPresent = (ackSessionPresent > 0);
			MQTT_LOG_PRINTFLN("Connect ack, code: %u", result.code);
			mSession.isConnected = (result.code == ConnectResult::Code::SUCCESS);
			if (isConnected()) {
				// Start keep-alive timers
				mSession.lastSentTimer.set(mSession.keepAliveTmSec*1000L);
				mSession.lastRecvTimer.set(mSession.keepAliveTmSec*1000L);
				MQTT_LOG_PRINTFLN("Keepalive interval: %u sec", mSession.keepAliveTmSec);
				// Process session-present flag
				if (!result.sessionPresent) {
					MQTT_LOG_PRINTFLN("Session is not present => reset subscription");
					mMessageHandlers.reset();
				}
			}
			return isConnected() ? Error::SUCCESS : Error::REFUSED;
		} else {
			MQTT_LOG_PRINTFLN("Connect ack is not received, rc: %i, ts: %lu", rc, mSystem.millis());
			return rc;
		}
	}

	/**
	 * Disconnects from the broker.
	 *
	 * @return execution status code
	 */
	Error::type disconnect() {
		Timer timer(mSystem, mOptions.commandTimeoutMs);
		MQTT_LOG_PRINTFLN("Disconnecting, ts: %lu", mSystem.millis());
		int len = MQTTSerialize_disconnect(mSendBuffer.get(), mSendBuffer.size());
		if (len <= 0) {
			return Error::ENCODING_FAILURE;
		}
		Error::type rc = sendPacket(len, timer);
		mSession.reset();
		return rc;
	}

	/**
	 * Publishes message to the specific topic.
	 *
	 * @param topic - the topic name
	 * @param message - the message
	 * @return execution status code
	 */
	Error::type publish(const char* topic, Message& message) {
		Timer timer(mSystem, mOptions.commandTimeoutMs);
		MQTT_LOG_PRINTFLN("Publish, to: %s, size: %u", topic, message.payloadLen);
		if (!isConnected()) {
			return Error::FAILURE;
		}
		MQTTString topicString = MQTTString_initializer;
		topicString.cstring = (char*)topic;
		unsigned short id;
		switch (message.qos) {
			case QOS1:
			case QOS2:
				id = mPacketId.getNext();
				break;
			default:
				break;
		}
		int len = MQTTSerialize_publish(mSendBuffer.get(), mSendBuffer.size(),
			0, message.qos, message.retained, id,
			topicString, (unsigned char*)message.payload, message.payloadLen
		);
		if (len <= 0) {
			return Error::ENCODING_FAILURE;
		}
		Error::type rc = sendPacket(len, timer);
		if (rc == Error::SUCCESS) switch (message.qos) {
			case QOS1:
			{
				rc = waitFor(PUBACK, timer);
				if (rc == Error::SUCCESS) {
					unsigned short ackId;
					unsigned char ackType, ackDup;
					if (MQTTDeserialize_ack(&ackType, &ackDup, &ackId, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
						rc = Error::DECODING_FAILURE;
					} else {
						MQTT_LOG_PRINTFLN("Publish ack received");
					}
				}
			}
				break;
			case QOS2:
			{
				// Simplified QoS 2 implementation
				rc = waitFor(PUBREC, timer);
				if (rc == Error::SUCCESS) {
					unsigned short ackId;
					unsigned char ackType, ackDup;
					if (MQTTDeserialize_ack(&ackType, &ackDup, &ackId, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
						rc = Error::DECODING_FAILURE;
					} else {
						int len = MQTTSerialize_ack(mSendBuffer.get(), mSendBuffer.size(), PUBREL, 0, ackId);
						if (len > 0) {
							rc = sendPacket(len, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
							if (rc == Error::SUCCESS) {
								rc = waitFor(PUBCOMP, timer);
								if (rc == Error::SUCCESS) {
									if (MQTTDeserialize_ack(&ackType, &ackDup, &ackId, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
										rc = Error::DECODING_FAILURE;
									}
								}
							}
						} else {
							rc = Error::ENCODING_FAILURE;
						}
					}
				}
			}
				break;
			default:
				break;
		}
		if (rc != Error::SUCCESS) {
			mSession.reset();
		}
		return rc;
	}

	/**
	 * Subscribes on specific topic.
	 *
	 *  @param topic - the topic pattern which can include wildcards
	 *  @param qos - the MQTT QoS value
	 *  @param cbk - the callback function to be invoked when a message is received for this subscription
	 *  @return execution status code
	 */
	Error::type subscribe(const char* topic, enum QoS qos, MessageHandlerCbk cbk) {
		Timer timer(mSystem, mOptions.commandTimeoutMs);
		MQTT_LOG_PRINTFLN("Subscribe, to: %s, qos: %u", topic, qos);
		if (!isConnected()) {
			return Error::FAILURE;
		}
		// Set handler
		if (!mMessageHandlers.set(topic, cbk)) {
			MQTT_LOG_PRINTFLN("List of message handlers is full");
			return Error::FAILURE;
		}
		Error::type rc = Error::SUCCESS;
		// Prepare message
		int len = 0;
		{
			MQTTString mqttTopic = {(char*)topic, {0, 0}};
			int mqttQos = qos;
			len = MQTTSerialize_subscribe(mSendBuffer.get(), mSendBuffer.size(),
				0, mPacketId.getNext(), 1, &mqttTopic, &mqttQos
			);
		}
		if (len <= 0) {
			rc = Error::ENCODING_FAILURE;
		} else {
			// Send message
			rc = sendPacket(len, timer);
			if (rc != Error::SUCCESS) {
				MQTT_LOG_PRINTFLN("Can't send subscribe, rc: %i", rc);
			} else {
				// Wait the response message
				rc = waitFor(SUBACK, timer);
				if (rc == Error::SUCCESS) {
					MQTT_LOG_PRINTFLN("Subscribe ack received");
					int ackCount = 0, ackQoS = -1;
					unsigned short ackId;
					if (MQTTDeserialize_suback(&ackId, 1, &ackCount, &ackQoS, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
						rc = Error::DECODING_FAILURE;
					} else {
						// Process the response message
						if (ackQoS == 0x80) {
							rc = Error::REFUSED;
						}
					}
				} else {
					MQTT_LOG_PRINTFLN("Subscribe ack is not received, rc: %i, ts: %lu", rc, mSystem.millis());
				}
			}
		}
		// Release handler if failed
		if (rc != Error::SUCCESS) {
			mMessageHandlers.reset(topic);
		}
		return rc;
	}

	/**
	 * Unsubscribes from specific topic.
	 *
	 *  @param topic - the topic name used in subscribe call before
	 *  @return execution status code
	 */
	Error::type unsubscribe(const char* topic) {
		Timer timer(mSystem, mOptions.commandTimeoutMs);
		MQTT_LOG_PRINTFLN("Unsubscribe, from: %s", topic);
		if (!isConnected()) {
			return Error::FAILURE;
		}
		MQTTString mqttTopic = {(char*)topic, {0, 0}};
		int len = MQTTSerialize_unsubscribe(mSendBuffer.get(), mSendBuffer.size(),
			0, mPacketId.getNext(), 1, &mqttTopic
		);
		if (len <= 0) {
			return Error::ENCODING_FAILURE;
		}
		Error::type rc = sendPacket(len, timer);
		if (rc != Error::SUCCESS) {
			MQTT_LOG_PRINTFLN("Can't send unsubscribe, rc: %i", rc);
			return rc;
		}
		rc = waitFor(UNSUBACK, timer);
		if (rc == Error::SUCCESS) {
			MQTT_LOG_PRINTFLN("Unsubscribe ack received");
			unsigned short ackId;
			if (MQTTDeserialize_unsuback(&ackId, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
				rc = Error::DECODING_FAILURE;
			} else {
				mMessageHandlers.reset(topic);
			}
		} else {
			MQTT_LOG_PRINTFLN("Unsubscribe ack is not received, rc: %i, ts: %lu", rc, mSystem.millis());
		}
		return rc;
	}

	/**
	 * Processes incoming messages and keep-alive transmission during given interval.
	 * Call is interrupted if connection is lost.
	 * Mast be called regularly.
	 *
	 * @param timeoutMs - the time to wait, in milliseconds
	 */
	void yield(unsigned long timeoutMs = 1000L) {
		Timer timer(mSystem, timeoutMs);
		MQTT_LOG_PRINTFLN("Yield for %lu ms", timer.leftMs());
		do {
			ReadPacketResult result;
			cycle(result, timer);
		} while (isConnected() && !timer.expired());
	}

	/**
	 * Calculates the time interval until the next keep-alive transmission.
	 * Could be used to estimate time until next call to yield is required.
	 *
	 * @return time in milliseconds
	 */
	unsigned long getIdleInterval() {
		unsigned long res = 0;
		if (isConnected() && !mSession.keepaliveSent) {
			res = min(mSession.lastSentTimer.leftMs(), mSession.lastRecvTimer.leftMs());
		}
		return res;
	}

private:

	class PacketId {
		public:
			PacketId() {}

			unsigned short getNext() {
				return next = (next == MAX_PACKET_ID) ? 1 : (next + 1);
			}

		private:
			static const unsigned short					MAX_PACKET_ID = 65535;
			unsigned short								next = 0;
	};

	class Timer {
		public:
			Timer(const System& system): mSystem(system) {
			}

			Timer(const System& system, unsigned long durationMs): Timer(system) {
				set(durationMs);
			}

			Timer(const Timer &timer, unsigned long minDurationMs, unsigned long maxDurationMs): Timer(timer.mSystem) {
				unsigned long durationMs = (timer.leftMs() > maxDurationMs) ? maxDurationMs : timer.leftMs();
				set(durationMs > minDurationMs ? durationMs : minDurationMs);
			}

			void set(unsigned long durationMs) {
				mStartMs = mSystem.millis();
				mDuration = durationMs;
			}

			void reset() {
				set(mDuration);
			}

			bool expired() const {
				return elapsedMs() >= mDuration;
			}

			unsigned long leftMs() const {
				return expired() ? 0 : mDuration - elapsedMs();
			}

			unsigned long elapsedMs() const {
				return mSystem.millis() - mStartMs;
			}

		private:
			const System								&mSystem;
			unsigned long								mStartMs = 0;
			unsigned long								mDuration = 0;
	};

	struct ReadPacketResult {
		bool											isPacketReceived = false;
		enum msgTypes									packetType;

		void reset() {
			isPacketReceived = false;
		}
	};

	struct Session {
		bool											isConnected = false;
		unsigned int									keepAliveTmSec = 0;;
		Timer											lastSentTimer;
		Timer											lastRecvTimer;
		bool											keepaliveSent = false;
		Timer											keepaliveAckTimer;

		Session(const System& system): lastSentTimer(system), lastRecvTimer(system),
			keepaliveAckTimer(system) {}

		void reset() {
			isConnected = false;
			keepAliveTmSec = 0;
			keepaliveSent = false;
		}
	};

	PacketId											mPacketId;
	Options												mOptions;
	Logger												&mLogger;
	System												&mSystem;
	Network												&mNetwork;
	Buffer												&mSendBuffer;
	Buffer												&mRecvBuffer;
	MessageHandlers										&mMessageHandlers;
	Session												mSession;

	Error::type sendPacket(int length, const Timer& timer) {
		int sent = 0;
		while (sent < length && !timer.expired()) {
			int r = mNetwork.write(&(mSendBuffer.get()[sent]), length - sent, timer.leftMs());
			if (r < 0) {
				return Error::NETWORK_FAILURE;
			}
			sent += r;
		}
		if (sent == length) {
			mSession.lastSentTimer.reset();
			return Error::SUCCESS;
		} else {
			return Error::FAILURE;
		}
	}

	Error::type recvPacket(ReadPacketResult& result, const Timer& timer) {
		result.reset();
		Error::type rc = Error::SUCCESS;
		int len = 0;
		// Read the fixed header byte
		if (mNetwork.read(mRecvBuffer.get(), 1, timer.leftMs()) != 1) {
			return Error::WAIT_TIMEOUT;
		}
		len++;
		// Read the remaining length
		int remLen = 0;
		rc = recvRemainingLength(&remLen, timer);
		if (rc != Error::SUCCESS) {
			return rc;
		}
		// Put the original remaining length into the buffer
		len += MQTTPacket_encode(mRecvBuffer.get() + len, remLen);
		// Verify buffer size
		if ((remLen + len) > mRecvBuffer.size()) {
			return Error::BUFFER_OVERFLOW;
		}
		// Read the rest of the packet
		if (remLen > 0 && (mNetwork.read(mRecvBuffer.get() + len, remLen, timer.leftMs()) != remLen)) {
			return Error::NETWORK_FAILURE;
		}
		// Get packet type
		MQTTHeader header = {0};
		header.byte = mRecvBuffer.get()[0];
		result.packetType = (msgTypes)header.bits.type;
		result.isPacketReceived = true;
		mSession.lastRecvTimer.reset();
		return Error::SUCCESS;
	}

	Error::type recvRemainingLength(int* result, const Timer& timer) {
		unsigned char c;
		long multiplier = 1;
		const long MAX_MULTIPLIER = 128L*128L*128L;
		*result = 0;
		do {
			if (mNetwork.read(&c, 1, timer.leftMs()) != 1) {
				return Error::NETWORK_FAILURE;
			}
			*result += (c & 127) * multiplier;
			multiplier *= 128;
			if (multiplier > MAX_MULTIPLIER) {
				return Error::DECODING_FAILURE_REM_LENGHT;
			}
		} while ((c & 128) != 0);
		return Error::SUCCESS;
	}

	Error::type waitFor(enum msgTypes packetType, const Timer& timer) {
		MQTT_LOG_PRINTFLN("Wait for message, type: %i, tm: %lu ms", packetType, timer.leftMs());
		ReadPacketResult result;
		Error::type rc = Error::SUCCESS;
		do {
			if (result.isPacketReceived && result.packetType == packetType) {
				// done
				break;
			}
			if (timer.expired()) {
				rc = Error::WAIT_TIMEOUT;
				break;
			} else {
				rc = cycle(result, timer);
			}
		} while (rc == Error::SUCCESS || rc == Error::WAIT_TIMEOUT);
		return rc;
	}

	Error::type cycle(ReadPacketResult& result, const Timer& timer) {
		Error::type rc = recvPacket(result, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
		if (result.isPacketReceived) {
			rc = processPacket(result.packetType, timer);
		}
		if (mSession.keepaliveSent && mSession.keepaliveAckTimer.expired()) {
			MQTT_LOG_PRINTFLN("Keepalive ack failure, ts: %lu", mSystem.millis());
			mSession.reset();
		}
		if (isConnected()) {
			rc = keepalive(timer);
		}
		return rc;
	}

	Error::type processPacket(enum msgTypes packetType, const Timer& timer) {
		MQTT_LOG_PRINTFLN("Process message, type: %i", packetType);
		Error::type rc = Error::SUCCESS;
		switch (packetType) {
			default:
			case CONNACK:
			case PUBACK:
			case PUBCOMP:
			case SUBACK:
				break;
			case PINGRESP:
				MQTT_LOG_PRINTFLN("Keepalive ack received");
				mSession.keepaliveSent = false;
				break;
			case PUBLISH:
			{
				MQTTString topicName = MQTTString_initializer;
				Message msg;
				int msgQos;
				if (MQTTDeserialize_publish(
						(unsigned char*)&msg.dup, &msgQos, (unsigned char*)&msg.retained,
						(unsigned short*)&msg.id, &topicName,
						(unsigned char**)&msg.payload, (int*)&msg.payloadLen,
						mRecvBuffer.get(), mRecvBuffer.size()
					) != 1)
				{
					rc = Error::DECODING_FAILURE;
				} else {
					bool ignore = false;
					msg.qos = (QoS)msgQos;
					MQTT_LOG_PRINTFLN("Publish received, qos: %i", msg.qos);
					switch (msg.qos) {
						case QOS0:
							break;
						case QOS1:
						{
							int len = MQTTSerialize_ack(mSendBuffer.get(), mSendBuffer.size(), PUBACK, 0, msg.id);
							if (len > 0) {
								rc = sendPacket(len, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
							} else {
								rc = Error::ENCODING_FAILURE;
							}
						}
							break;
						case QOS2:
						{
							// Simplified QoS 2 implementation
							int len = MQTTSerialize_ack(mSendBuffer.get(), mSendBuffer.size(), PUBREC, 0, msg.id);
							if (len > 0) {
								rc = sendPacket(len, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
							} else {
								rc = Error::ENCODING_FAILURE;
							}
						}
							break;
						default:
							// Not supported => ignore
							ignore = true;
							break;
					}
					if (!ignore && rc == Error::SUCCESS) {
						deliverMessage(topicName, msg);
					}
				}
			}
				break;
			case PUBREL:
			{
				// Simplified QoS 2 implementation
				unsigned short ackId;
				unsigned char ackType, ackDup;
				if (MQTTDeserialize_ack(&ackType, &ackDup, &ackId, mRecvBuffer.get(), mRecvBuffer.size()) != 1) {
					rc = Error::DECODING_FAILURE;
				} else {
					int len = MQTTSerialize_ack(mSendBuffer.get(), mSendBuffer.size(), PUBCOMP, 0, ackId);
					if (len > 0) {
						rc = sendPacket(len, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
					} else {
						rc = Error::ENCODING_FAILURE;
					}
				}
			}
				break;
		}
		return rc;
	}

	Error::type keepalive(const Timer& timer) {
		if (!isConnected()) {
			return Error::FAILURE;
		}
		if (mSession.keepAliveTmSec == 0) {
			return Error::SUCCESS;
		}
		if (!mSession.keepaliveSent
			// Sent timer expires => send
			&& (mSession.lastSentTimer.expired() || (
				// Do not send if another message is just sent => try to wait the response
				mSession.lastSentTimer.elapsedMs() > mOptions.commandTimeoutMs
				// No message in-flight and Recv timer expires => send
				&& mSession.lastRecvTimer.expired())
			))
		{
			MQTT_LOG_PRINTFLN("Keepalive, ts: %lu", mSystem.millis());
			int len = MQTTSerialize_pingreq(mSendBuffer.get(), mSendBuffer.size());
			if (len <=0 ) {
				return Error::ENCODING_FAILURE;
			}
			Error::type rc = sendPacket(len, Timer(timer, NET_MIN_TM_MS, NET_MAX_TM_MS));
			if (rc == Error::SUCCESS) {
				mSession.keepaliveAckTimer.set(mOptions.commandTimeoutMs);
				mSession.keepaliveSent = true;
			}
			return rc;
		}
		return Error::SUCCESS;
	}

	unsigned int adjustKeepaliveTm(unsigned int tmSec) {
		unsigned int res = tmSec * 0.8;
		return (res < 1) ? 1 : res;
	}

	void deliverMessage(MQTTString& topic, Message& message) {
		for (int i = 0; i < mMessageHandlers.size(); ++i) {
			if (mMessageHandlers.get()[i].isUsed()) {
				if (MQTTPacket_equals(&topic, (char*)(mMessageHandlers.get()[i].topic))
					|| isTopicMatched((char*)(mMessageHandlers.get()[i].topic), topic))
				{
					MQTT_LOG_PRINTFLN("Deliver message for: %s", mMessageHandlers.get()[i].topic);
					MessageData md(topic, message);
					mMessageHandlers.get()[i].cbk(md);
				}
			}
		}
	}

	// Assume topic filter and name is in correct format
	// # can only be at end
	// + and # can only be next to separator
	bool isTopicMatched(char* topicFilter, MQTTString& topic) {
		char* curf = topicFilter;
		char* curn = topic.lenstring.data;
		char* curn_end = curn + topic.lenstring.len;
		while (*curf && curn < curn_end) {
			if (*curn == '/' && *curf != '/') break;
			if (*curf != '+' && *curf != '#' && *curf != *curn) break;
			if (*curf == '+') {
				// Skip until we meet the next separator, or end of string
				char* nextpos = curn + 1;
				while (nextpos < curn_end && *nextpos != '/') {
					nextpos = ++curn + 1;
				}
			} else if (*curf == '#') {
				// Skip until end of string
				curn = curn_end - 1;
			}
			curf++;
			curn++;
		}
		return (curn == curn_end) && (*curf == '\0');
	}

#ifndef min
	template <class T> inline const T& min(const T& a, const T& b) {
		return b < a ? b : a;
	}
#endif
};

#endif /* MQTT_CLIENT_H_ */
