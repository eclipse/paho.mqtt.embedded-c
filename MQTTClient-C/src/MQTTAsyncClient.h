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

#if !defined(MQTTAsync_CLIENT_H)
#define MQTTAsync_CLIENT_H

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


/* according to the MQTTAsync specification - do not change! */
#define MAX_PACKET_ID 65535 

enum QoS { QOS0, QOS1, QOS2, SUBFAIL = 0x80 };

/* all failure return codes must be negative */
enum returnCode { MQTTAsync_NOMEM = -2, MQTTAsync_FAILURE = -1, MQTTAsync_SUCCESS = 0 };

/* Timer and mutex structure for mqtt. */
typedef void * MQTTAsyncTimer;
typedef void * MQTTAsyncMutex;


/* Platform routine and variable for mqtt. */
typedef struct _MQTTAsyncPlatform
{
	/* User's context pointer. */
	void * context_ptr;
	
	/* Network operation routine. */
	int (*MqttRead)(struct _MQTTAsyncPlatform *plat_ptr, unsigned char *buff_ptr, int len, int timeout_ms);
	int (*MqttWrite)(struct _MQTTAsyncPlatform *plat_ptr, unsigned char *buff_ptr, int len, int timeout_ms);

	/* Timer routine. */
	void (*TimerInit)(MQTTAsyncTimer *time_ptr);
	void (*TimerDeinit)(MQTTAsyncTimer *time_ptr);
	char (*TimerIsExpired)(MQTTAsyncTimer timer);
	void (*TimerCountdownMS)(MQTTAsyncTimer timer, unsigned int timeout_ms);
	void (*TimerCountdown)(MQTTAsyncTimer timer, unsigned int timeout_s);
	int  (*TimerLeftMS)(MQTTAsyncTimer timer);

    /* Mutex routine. */
    void (*MutexInit)(MQTTAsyncMutex *mutex_ptr);
    int  (*MutexLock)(MQTTAsyncMutex *mutex_ptr);
    int  (*MutexUnlock)(MQTTAsyncMutex *mutex_ptr);
    int  (*MutexDeinit)(MQTTAsyncMutex *mutex_ptr);

    /* Time delay. */
    void (*DelayMs)(unsigned int delay_ms);

	/* Memory allocate and free routine. */
    void * (*MemCalloc)(unsigned int n, unsigned int size);
    void   (*MemFree)(void *addr_ptr);
} MQTTAsyncPlatform;

typedef struct _MQTTAsyncMessage
{
    enum QoS           qos;
    unsigned char retained;
    unsigned char      dup;
    unsigned short      id;
    
    void         * payload;
    size_t      payloadlen;
} MQTTAsyncMessage;

typedef struct _MQTTAsyncMessageData
{
    MQTTAsyncMessage  * message;
    MQTTString      * topicName;
} MQTTAsyncMessageData;

typedef struct _MQTTAsyncConnackData
{
    unsigned char             rc;
    unsigned char sessionPresent;
} MQTTAsyncConnackData;

typedef struct _MQTTAsyncSubackData
{
    int grantedQoS;
} MQTTAsyncSubackData;

typedef void (*messageHandler)(void * context_ptr, MQTTAsyncMessageData*);
typedef struct _MQTTAsyncMessageHandlers
{
    const char  * topicFilter;
    void (*fp) (void *context_ptr, MQTTAsyncMessageData*);
    void        * context_ptr;
} MQTTAsyncMessageHandlers;

/* Packet store structure for async mode. */
typedef struct _MQTTAsyncPacket
{
    unsigned char  packet_type;
    unsigned char * packet_ptr;
    int              packetlen;
} MQTTAsyncPacket;

/* Init config structure for mqtt. */
typedef struct _MQTTAsyncClientCfg
{
    unsigned int        cmdTimeoutMs;  /* Command waitting timeout, unit: ms. */
    unsigned int   keepAliveInterval;  /* Keepalive interval, unit: s. */
    unsigned char             tryCnt;  /* Try count for every data send process. */

    void (*defaultMessageHandler) (void *context_ptr, MQTTAsyncMessageData*);
    void     * defaultMessageCtx_ptr;  /* Default Message handler and context. */

} MQTTAsyncClientCfg;

typedef struct _MQTTAsyncClient
{
    unsigned int          next_packetid;
    
    char               ping_outstanding;
    int                     isconnected;
    int                    cleansession;
	unsigned char           nopong_stat;  /* Set nopong state and no need to wait server's pong frame. */

    int                   max_message_handlers;  /* Message handlers are indexed by subscription topic. */
    MQTTAsyncMessageHandlers * messageHandlers;
    
    MQTTAsyncPlatform        * plat_ptr;  /* Platform routine for mqtt use. */
    MQTTAsyncClientCfg    * initcfg_ptr;  /* Init configure structure. */
    
    MQTTAsyncTimer            last_sent;
	MQTTAsyncTimer        last_received;
    MQTTAsyncMutex           read_mutex;
    MQTTAsyncMutex          write_mutex;

    /* Stored ack packet for async mode. */
    MQTTAsyncPacket async_ackpacket[10];
    MQTTAsyncMutex          inner_mutex;
    
} MQTTAsyncClient;

#define DefaultClient {0, 0, 0, 0, NULL, NULL, 0, 0, 0}


/**
 * Create an MQTTAsync client object
 * @param platform
 * @param network
 * @param cmdTimeoutMs
 * @param
 */
DLLExport void MQTTAsyncClientInit(MQTTAsyncClient* client, MQTTAsyncPlatform* platform, MQTTAsyncClientCfg* initcfg);

/** MQTTAsync Connect - send an MQTTAsync connect packet down the network and wait for a Connack
 *  The nework object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncConnect(MQTTAsyncClient* client, MQTTPacket_connectData* options, unsigned char isAsyncAck);

/** MQTTAsync Publish - send an MQTTAsync publish packet and wait for all acks to complete for all QoSs
 *  @param client - the client object to use
 *  @param topic - the topic to publish to
 *  @param message - the message to send
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncPublish(MQTTAsyncClient* client, const char*, MQTTAsyncMessage*, unsigned char isAsyncAck);

/** MQTTAsync Subscribe - send an MQTTAsync subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncSubscribe(MQTTAsyncClient* client, const char* topicFilter, enum QoS, messageHandler msgHandler, void *context_ptr, unsigned char isAsyncAck);

/** MQTTAsync Subscribe - send an MQTTAsync subscribe packet to sub many topic and wait for suback before returning.
 *  @param client - the client object to use
 *  @param count - topic count for msgHandlers array
 *  @param msgHandlers - message handler array, contain topicFilter, fp, context_ptr domain
 *  @param qos - qos array for each message
 *  @param grantedQoS - array that contain the returned qos, user should check every qos value when return failure
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncSubscribeMany(MQTTAsyncClient* client, int count, MQTTAsyncMessageHandlers msgHandlers[], int qos[], int grantedQoS[], unsigned char isAsyncAck);

/** MQTTAsync Subscribe - send an MQTTAsync unsubscribe packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncUnsubscribe(MQTTAsyncClient* client, const char* topicFilter, unsigned char isAsyncAck);

/** MQTTAsync Subscribe - send an MQTTAsync unsubscribe many packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param count - topic count
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncUnsubscribeMany(MQTTAsyncClient* client, int count, const char* topicFilter[], unsigned char isAsyncAck);

/** MQTTAsync Disconnect - send an MQTTAsync disconnect packet and close the connection
 *  @param client - the client object to use
 *  @param isSendpacket - send MQTTAsync disconnect packet or not, some platform will crash when call mbedtls_write on link error condition
 *  @return success code
 */
DLLExport int MQTTAsyncDisconnect(MQTTAsyncClient* client, int isSendPacket);

/** MQTTAsync Yield - MQTTAsync background
 *  @param client - the client object to use
 *  @param time - the time, in milliseconds, to yield for
 *  @param waitWhenNodata - donot wait flag when no data in receive buffer
 *  @param isAsyncAck - is ack packet in asynchronous mode or not
 *  @return success code
 */
DLLExport int MQTTAsyncYield(MQTTAsyncClient* client, int time, unsigned char waitWhenNodata, unsigned char isAsyncAck);

/** MQTTAsync Keepalive time left - Get keepalive left time in ms.
 *  @param client - the client object to use
 *  @return Left time in ms format.
 */
DLLExport int MQTTAsyncKeppaliveLeftMS(MQTTAsyncClient *client);

/** MQTTAsync set nopong state - Set mqtt's nopong state.
 *  @param client - the client object to use
 *  @param isNopong - no pong flag, non-zero means no need to wait server's pong frame
 *  @return none
 */
DLLExport void MQTTAsyncSetNopongStat(MQTTAsyncClient *client, int isNopong);

/** MQTTAsync IsConnected
 *  @param client - the client object to use
 *  @return truth value indicating whether the client is connected to the server
 */
DLLExport int MQTTAsyncIsConnected(MQTTAsyncClient* client);

/** MQTTAsync Send heartbeat frame
 *  @param client - the client object to use
 *  @param timeout_ms - timeout for waitting pong frame, 0 means only send ping and no wait pong
 *  @return success code
 */
DLLExport int MQTTAsyncSendHeartBeat(MQTTAsyncClient* client, unsigned int timeout_ms);

#if defined(__cplusplus)
     }
#endif

#endif
