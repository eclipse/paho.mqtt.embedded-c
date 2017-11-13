/*******************************************************************************
 * Copyright (c) 2017 IBM Corp.
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

enum ReasonCodes {
  SUCCESS = 0,
  NORMAL_DISCONNECTION = 0,
  GRANTED_QOS_0 = 0,
  GRANTED_QOS_1 = 1,
  GRANTED_QOS_2 = 2,
  DISCONNECT_WITH_WILL_MESSAGE = 4,
  NO_MATCHING_SUBSCRIBERS = 16,
  NO_SUBSCRIPTION_EXISTED = 17,
  CONTINUE_AUTHENTICATION = 24,
  RE_AUTHENTICATE = 25,
  UNSPECIFIED_ERROR = 128,
  MALFORMED_PACKET = 129,
  PROTOCOL_ERROR = 130,
  IMPLEMENTATION_SPECIFIC_ERROR = 131,
  UNSUPPORTED_PROTOCOL_VERSION = 132,
  /* ... */
  WILDCARD_SUBSCRIPTION_NOT_SUPPORTED = 162
};
