/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef MQTTCONNECT_H_
#define MQTTCONNECT_H_

	typedef union
	{
		unsigned char all;	/**< all connect flags */
#if defined(REVERSED)
		struct
		{
			unsigned int username : 1;			/**< 3.1 user name */
			unsigned int password : 1; 			/**< 3.1 password */
			unsigned int willRetain : 1;		/**< will retain setting */
			unsigned int willQoS : 2;				/**< will QoS value */
			unsigned int will : 1;			    /**< will flag */
			unsigned int cleansession : 1;	  /**< clean session flag */
			unsigned int : 1;	  	          /**< unused */
		} bits;
#else
		struct
		{
			unsigned int : 1;	     					/**< unused */
			unsigned int cleansession : 1;	  /**< cleansession flag */
			unsigned int will : 1;			    /**< will flag */
			unsigned int willQoS : 2;				/**< will QoS value */
			unsigned int willRetain : 1;		/**< will retain setting */
			unsigned int password : 1; 			/**< 3.1 password */
			unsigned int username : 1;			/**< 3.1 user name */
		} bits;
#endif
	} MQTTConnectFlags;	/**< connect flags byte */



/**
 * Defines the MQTT "Last Will and Testament" (LWT) settings for
 * the connect packet.
 */
typedef struct
{
	/** The eyecatcher for this structure.  must be MQTW. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0 */
	int struct_version;
	/** The LWT topic to which the LWT message will be published. */
	MQTTString topicName;
	/** The LWT payload. */
	MQTTString message;
	/**
      * The retained flag for the LWT message (see MQTTAsync_message.retained).
      */
	int retained;
	/**
      * The quality of service setting for the LWT message (see
      * MQTTAsync_message.qos and @ref qos).
      */
	int qos;
} MQTTPacket_willOptions;


#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }


typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0, 1 or 2.
	  * 0 signifies no SSL options and no serverURIs
	  * 1 signifies no serverURIs
	  */
	int struct_version;
	/** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
	  */
	int MQTTVersion;
	MQTTString clientID;
	int keepAliveInterval;
	int cleansession;
	int willFlag;
	MQTTPacket_willOptions will;
	MQTTString username;
	MQTTString password;
} MQTTPacket_connectData;

#define MQTTPacket_connectData_initializer { {'M', 'Q', 'T', 'W'}, 0, 3, {NULL, {0, NULL}}, 0, 1, 0, \
		MQTTPacket_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

int MQTTSerialize_connect(char* buf, int buflen, MQTTPacket_connectData* options);
int MQTTDeserialize_connect(MQTTPacket_connectData* data, char* buf, int len);

int MQTTSerialize_connack(char* buf, int buflen, int connack_rc);
int MQTTDeserialize_connack(int* connack_rc, char* buf, int buflen);

int MQTTSerialize_disconnect(char* buf, int buflen);

#endif /* MQTTCONNECT_H_ */
