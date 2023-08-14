/*******************************************************************************
 * Copyright (c) 2023 Microsoft Corporation. All rights reserved.
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
 *******************************************************************************/

#ifndef MQTTCONNECTINTERNAL_H_
#define MQTTCONNECTINTERNAL_H_

#include <stdint.h>
#if !defined(DLLImport)
  #define DLLImport
#endif
#if !defined(DLLExport)
  #define DLLExport
#endif

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
		unsigned int cleansession : 1;	  /**< V3 clean session or V5 clean start flag */
		unsigned int : 1;	  	          /**< unused */
	} bits;
#else
	struct
	{
		unsigned int : 1;	     					/**< unused */
		unsigned int cleansession : 1;	  /**< V3 clean session or V5 cleanstart flag */
		unsigned int will : 1;			    /**< will flag */
		unsigned int willQoS : 2;				/**< will QoS value */
		unsigned int willRetain : 1;		/**< will retain setting */
		unsigned int password : 1; 			/**< 3.1 password */
		unsigned int username : 1;			/**< 3.1 user name */
	} bits;
#endif
} MQTTConnectFlags;	/**< connect flags byte */

typedef union
{
	unsigned char all;	/**< all connack flags */
#if defined(REVERSED)
	struct
	{
    unsigned int reserved : 7;	  	    /**< unused */
		unsigned int sessionpresent : 1;    /**< session present flag */
	} bits;
#else
	struct
	{
		unsigned int sessionpresent : 1;    /**< session present flag */
    unsigned int reserved: 7;	     			/**< unused */
	} bits;
#endif
} MQTTConnackFlags;	/**< connack flags byte */

#endif /* MQTTCONNECTINTERNAL_H_ */
