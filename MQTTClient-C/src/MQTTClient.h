/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - documentation and platform specific header
 *    Ian Craggs - add setMessageHandler function
 *******************************************************************************/

#if !defined(MQTT_CLIENT_H)
#define MQTT_CLIENT_H

#if defined(__cplusplus)
 extern "C" {
#endif

#if defined(WIN32_DLL) || defined(WIN64_DLL)
  #define DLLImport __declspec(dllimport)
  #define DLLExport __declspec(dllexport)
#elif defined(LINUX_SO)
  #define DLLImport extern
  #define DLLExport  __attribute__ ((visibility ("default")))
#else
  #define DLLImport
  #define DLLExport
#endif

#include "MQTTPacket.h"


/* according to the MQTT specification - do not change! */
#define MAX_PACKET_ID 65535 

enum QoS { QOS0, QOS1, QOS2, SUBFAIL = 0x80 };

/* all failure return codes must be negative */
enum returnCode { MQTT_BUFFER_OVERFLOW = -2, MQTT_FAILURE = -1, MQTT_SUCCESS = 0 };

/* Timer structure for mqtt. */
typedef void * MQTTTimer;

/* Platform routine and variable for mqtt. */
typedef struct MQTTPlatform
{
	/* User's context pointer. */
	void * context_ptr;
	
	/* Network operation routine. */
	int (*MqttRead)(struct MQTTPlatform *plat_ptr, unsigned char * buff_ptr, int len, int timeout_ms);
	int (*MqttWrite)(struct MQTTPlatform *plat_ptr, unsigned char *buff_ptr, int len, int timeout_ms);

	/* Timer routine. */
	void (*TimerInit)(MQTTTimer *time_ptr);
	void (*TimerDeinit)(MQTTTimer *time_ptr);
	char (*TimerIsExpired)(MQTTTimer timer);
	void (*TimerCountdownMS)(MQTTTimer timer, unsigned int timeout_ms);
	void (*TimerCountdown)(MQTTTimer timer, unsigned int timeout_s);
	int  (*TimerLeftMS)(MQTTTimer timer);

	/* Memory allocate and free routine. */
    void * (*MemCalloc)(unsigned int n, unsigned int size);
    void   (*MemFree)(void *addr_ptr);
	
} MQTTPlatform;

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

typedef struct MessageData
{
    MQTTMessage* message;
    MQTTString* topicName;
} MessageData;

typedef struct MQTTConnackData
{
    unsigned char rc;
    unsigned char sessionPresent;
} MQTTConnackData;

typedef struct MQTTSubackData
{
    enum QoS grantedQoS;
} MQTTSubackData;

typedef void (*messageHandler)(void * context_ptr, MessageData*);

typedef struct MessageHandlers
{
    const char* topicFilter;
    void (*fp) (void *context_ptr, MessageData*);
    void        * context_ptr;
} MessageHandlers;

typedef struct MQTTClient
{
    unsigned int       next_packetid;
	unsigned int  command_timeout_ms;
	
    size_t                  buf_size;
	size_t              readbuf_size;
    unsigned char              * buf;
	unsigned char          * readbuf;
    
    unsigned int   keepAliveInterval;
    char            ping_outstanding;
    int                  isconnected;
    int                 cleansession;

	/* Try count for every send request. */
    unsigned char            try_cnt;

	/* Message handlers are indexed by subscription topic. */
    int         max_message_handlers;    
    MessageHandlers *messageHandlers;

    /* Default Message handler and context. */
    void (*defaultMessageHandler) (void *context_ptr, MessageData*);
    void     * defaultMessageCtx_ptr;

	/* Platform routine for mqtt use. */
    MQTTPlatform          * plat_ptr;
	
    MQTTTimer              last_sent;
	MQTTTimer          last_received;
	
} MQTTClient;

#define DefaultClient {0, 0, 0, 0, NULL, NULL, 0, 0, 0}


/**
 * Create an MQTT client object
 * @param platform
 * @param network
 * @param command_timeout_ms
 * @param
 */
DLLExport void MQTTClientInit(MQTTClient* c, MQTTPlatform* platform, unsigned int command_timeout_ms);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The nework object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @return success code
 */
DLLExport int MQTTConnectWithResults(MQTTClient* client, MQTTPacket_connectData* options, MQTTConnackData* data);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The nework object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @return success code
 */
DLLExport int MQTTConnect(MQTTClient* client, MQTTPacket_connectData* options);

/** MQTT Publish - send an MQTT publish packet and wait for all acks to complete for all QoSs
 *  @param client - the client object to use
 *  @param topic - the topic to publish to
 *  @param message - the message to send
 *  @return success code
 */
DLLExport int MQTTPublish(MQTTClient* client, const char*, MQTTMessage*);

/** MQTT SetMessageHandler - set or remove a per topic message handler
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter set the message handler for
 *  @param messageHandler - pointer to the message handler function or NULL to remove
 *  @return success code
 */
DLLExport int MQTTSetMessageHandler(MQTTClient* client, const char* topicFilter, messageHandler msgHandler, void *context_ptr);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @return success code
 */
DLLExport int MQTTSubscribe(MQTTClient* client, const char* topicFilter, enum QoS, messageHandler, void *context_ptr);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @param data - suback granted QoS returned
 *  @return success code
 */
DLLExport int MQTTSubscribeWithResults(MQTTClient* client, const char* topicFilter, enum QoS, messageHandler, void *context_ptr, MQTTSubackData* data);

/** MQTT Subscribe - send an MQTT unsubscribe packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @return success code
 */
DLLExport int MQTTUnsubscribe(MQTTClient* client, const char* topicFilter);

/** MQTT Disconnect - send an MQTT disconnect packet and close the connection
 *  @param client - the client object to use
 *  @return success code
 */
DLLExport int MQTTDisconnect(MQTTClient* client);

/** MQTT Yield - MQTT background
 *  @param client - the client object to use
 *  @param time - the time, in milliseconds, to yield for
 *  @return success code
 */
DLLExport int MQTTYield(MQTTClient* client, int time);

/** MQTT isConnected
 *  @param client - the client object to use
 *  @return truth value indicating whether the client is connected to the server
 */
DLLExport int MQTTIsConnected(MQTTClient* client);

#if defined(__cplusplus)
     }
#endif

#endif
