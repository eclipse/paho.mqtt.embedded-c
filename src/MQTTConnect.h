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
			bool username : 1;			/**< 3.1 user name */
			bool password : 1; 			/**< 3.1 password */
			bool willRetain : 1;		/**< will retain setting */
			unsigned int willQoS : 2;	/**< will QoS value */
			bool will : 1;			    /**< will flag */
			bool cleanstart : 1;	    /**< clean session flag */
			int : 1;	                /**< unused */
		} bits;
#else
		struct
		{
			int : 1;	                /**< unused */
			bool cleanstart : 1;	    /**< cleansession flag */
			bool will : 1;			    /**< will flag */
			unsigned int willQoS : 2;	/**< will QoS value */
			bool willRetain : 1;		/**< will retain setting */
			bool password : 1; 			/**< 3.1 password */
			bool username : 1;			/**< 3.1 user name */
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
	char* topicName;
	/** The LWT payload. */
	char* message;
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


#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, NULL, NULL, 0, 0 }



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
	char* clientID;
	int keepAliveInterval;
	int cleansession;
	MQTTPacket_willOptions* will;
	char* username;
	char* password;
} MQTTPacket_connectData;



#endif /* MQTTCONNECT_H_ */
