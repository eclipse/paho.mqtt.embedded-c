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

#include "V5/MQTTV5Packet.h"

static const char* v5property_identifier_to_string(int identifier)
{
    switch (identifier) 
    {
        case PAYLOAD_FORMAT_INDICATOR:
            return "PAYLOAD_FORMAT_INDICATOR";
        case MESSAGE_EXPIRY_INTERVAL:
            return "MESSAGE_EXPIRY_INTERVAL";
        case CONTENT_TYPE:
            return "CONTENT_TYPE";
        case RESPONSE_TOPIC:
            return "RESPONSE_TOPIC";
        case CORRELATION_DATA:
                return "CORRELATION_DATA";
        case SUBSCRIPTION_IDENTIFIER:
                return "SUBSCRIPTION_IDENTIFIER";
        case SESSION_EXPIRY_INTERVAL:
                return "SESSION_EXPIRY_INTERVAL";
        case ASSIGNED_CLIENT_IDENTIFER:
                return "ASSIGNED_CLIENT_IDENTIFER";
        case SERVER_KEEP_ALIVE:
                return "SERVER_KEEP_ALIVE";
        case AUTHENTICATION_METHOD:
                return "AUTHENTICATION_METHOD";
        case AUTHENTICATION_DATA:
                return "AUTHENTICATION_DATA";
        case REQUEST_PROBLEM_INFORMATION:
                return "REQUEST_PROBLEM_INFORMATION";
        case WILL_DELAY_INTERVAL:
                return "WILL_DELAY_INTERVAL";
        case REQUEST_RESPONSE_INFORMATION:
                return "REQUEST_RESPONSE_INFORMATION";
        case RESPONSE_INFORMATION:
                return "RESPONSE_INFORMATION";
        case SERVER_REFERENCE:
                return "SERVER_REFERENCE";
        case REASON_STRING:
                return "REASON_STRING";
        case RECEIVE_MAXIMUM:
                return "RECEIVE_MAXIMUM";
        case TOPIC_ALIAS_MAXIMUM:
                return "TOPIC_ALIAS_MAXIMUM";
        case TOPIC_ALIAS:
                return "TOPIC_ALIAS";
        case MAXIMUM_QOS:
                return "MAXIMUM_QOS";
        case RETAIN_AVAILABLE:
                return "RETAIN_AVAILABLE";
        case USER_PROPERTY:
                return "USER_PROPERTY";
        case MAXIMUM_PACKET_SIZE:
                return "MAXIMUM_PACKET_SIZE";
        case WILDCARD_SUBSCRIPTION_AVAILABLE:
                return "WILDCARD_SUBSCRIPTION_AVAILABLE";
        case SUBSCRIPTION_IDENTIFIER_AVAILABLE:
                return "SUBSCRIPTION_IDENTIFIER_AVAILABLE";
        case SHARED_SUBSCRIPTION_AVAILABLE:
                return "SHARED_SUBSCRIPTION_AVAILABLE";
        default:
            return "UNKNOWN";
    }
}

static const char* v5property_type_to_string(int type)
{
    switch (type)
    {
        case BYTE:
            return "BYTE";
        case TWO_BYTE_INTEGER:
            return "TWO_BYTE_INTEGER";
        case FOUR_BYTE_INTEGER:
            return "FOUR_BYTE_INTEGER";
        case VARIABLE_BYTE_INTEGER:
            return "VARIABLE_BYTE_INTEGER";
        case BINARY_DATA:
            return "BINARY_DATA";
        case UTF_8_ENCODED_STRING:
            return "UTF_8_ENCODED_STRING";
        case UTF_8_STRING_PAIR:
            return "UTF_8_STRING_PAIR";
        default:
            return "UNKNOWN";
    }
}

void v5property_print(MQTTProperty property)
{
    const char* identifier_str = v5property_identifier_to_string(property.identifier);
    int type = MQTTProperty_getType(property.identifier);
    const char* type_str = v5property_type_to_string(type);

    switch(type)
    {
        case BYTE:
            printf("\t%s (%s) = %d\n", identifier_str, type_str, (int)property.value.byte);
            break;
        case TWO_BYTE_INTEGER:
            printf("\t%s (%s) = %d\n", identifier_str, type_str, (int)property.value.integer2);
            break;
        case FOUR_BYTE_INTEGER:
            printf("\t%s (%s) = %d\n", identifier_str, type_str, (int)property.value.integer4);
            break;
        case VARIABLE_BYTE_INTEGER:
            printf("\t%s (%s) = %d\n", identifier_str, type_str, (int)property.value.integer4);
            break;
        case BINARY_DATA:
            printf("\t%s (%s) = [%.*s]\n", identifier_str, type_str, 
                property.value.data.len, property.value.data.data);
            break;
        case UTF_8_ENCODED_STRING:
            printf("\t%s (%s) = [%.*s]\n", identifier_str, type_str, 
                property.value.data.len, property.value.data.data);
            break;
        case UTF_8_STRING_PAIR:
            printf("\t%s: (%s) = [%.*s], [%.*s]\n", identifier_str, type_str, 
                property.value.string_pair.key.len, property.value.string_pair.key.data,
                property.value.string_pair.val.len, property.value.string_pair.val.data);
            break;
        default:
            printf("\tUNKNOWN Property\n");
    }
}

const char* v5reasoncode_to_string(enum ReasonCodes reasoncode)
{
    switch (reasoncode)
    {
        case MQTTV5_SUCCESS:
            return "MQTTV5_SUCCESS";
        case MQTTV5_NO_MATCHING_SUBSCRIBERS:
            return "MQTTV5_NO_MATCHING_SUBSCRIBERS";
        case MQTTV5_NO_SUBSCRIPTION_FOUND:
            return "MQTTV5_NO_SUBSCRIPTION_FOUND";
        case MQTTV5_CONTINUE_AUTHENTICATION:
            return "MQTTV5_CONTINUE_AUTHENTICATION";
        case MQTTV5_RE_AUTHENTICATE:
            return "MQTTV5_RE_AUTHENTICATE";
        case MQTTV5_UNSPECIFIED_ERROR:
            return "MQTTV5_UNSPECIFIED_ERROR";
        case MQTTV5_MALFORMED_PACKET:
            return "MQTTV5_MALFORMED_PACKET";
        case MQTTV5_PROTOCOL_ERROR:
            return "MQTTV5_PROTOCOL_ERROR";
        case MQTTV5_IMPLEMENTATION_SPECIFIC_ERROR:
            return "MQTTV5_IMPLEMENTATION_SPECIFIC_ERROR";
        case MQTTV5_UNSUPPORTED_PROTOCOL_VERSION:
            return "MQTTV5_UNSUPPORTED_PROTOCOL_VERSION";
        case MQTTV5_CLIENT_IDENTIFIER_NOT_VALID:
            return "MQTTV5_CLIENT_IDENTIFIER_NOT_VALID";
        case MQTTV5_BAD_USER_NAME_OR_PASSWORD:
            return "MQTTV5_BAD_USER_NAME_OR_PASSWORD";
        case MQTTV5_NOT_AUTHORIZED:
            return "MQTTV5_NOT_AUTHORIZED";
        case MQTTV5_SERVER_UNAVAILABLE:
            return "MQTTV5_SERVER_UNAVAILABLE";
        case MQTTV5_SERVER_BUSY:
            return "MQTTV5_SERVER_BUSY";
        case MQTTV5_BANNED:
            return "MQTTV5_BANNED";
        case MQTTV5_SERVER_SHUTTING_DOWN:
            return "MQTTV5_SERVER_SHUTTING_DOWN";
        case MQTTV5_BAD_AUTHENTICATION_METHOD:
            return "MQTTV5_BAD_AUTHENTICATION_METHOD";
        case MQTTV5_KEEP_ALIVE_TIMEOUT:
            return "MQTTV5_KEEP_ALIVE_TIMEOUT";
        case MQTTV5_SESSION_TAKEN_OVER:
            return "MQTTV5_SESSION_TAKEN_OVER";
        case MQTTV5_TOPIC_FILTER_INVALID:
            return "MQTTV5_TOPIC_FILTER_INVALID";
        case MQTTV5_TOPIC_NAME_INVALID:
            return "MQTTV5_TOPIC_NAME_INVALID";
        case MQTTV5_PACKET_IDENTIFIER_IN_USE:
            return "MQTTV5_PACKET_IDENTIFIER_IN_USE";
        case MQTTV5_PACKET_IDENTIFIER_NOT_FOUND:
            return "MQTTV5_PACKET_IDENTIFIER_NOT_FOUND";
        case MQTTV5_RECEIVE_MAXIMUM_EXCEEDED:
            return "MQTTV5_RECEIVE_MAXIMUM_EXCEEDED";
        case MQTTV5_TOPIC_ALIAS_INVALID:
            return "MQTTV5_TOPIC_ALIAS_INVALID";
        case MQTTV5_PACKET_TOO_LARGE:
            return "MQTTV5_PACKET_TOO_LARGE";
        case MQTTV5_MESSAGE_RATE_TOO_HIGH:
            return "MQTTV5_MESSAGE_RATE_TOO_HIGH";
        case MQTTV5_QUOTA_EXCEEDED:
            return "MQTTV5_QUOTA_EXCEEDED";
        case MQTTV5_ADMINISTRATIVE_ACTION:
            return "MQTTV5_ADMINISTRATIVE_ACTION";
        case MQTTV5_PAYLOAD_FORMAT_INVALID:
            return "MQTTV5_PAYLOAD_FORMAT_INVALID";
        case MQTTV5_RETAIN_NOT_SUPPORTED:
            return "MQTTV5_RETAIN_NOT_SUPPORTED";
        case MQTTV5_QOS_NOT_SUPPORTED:
            return "MQTTV5_QOS_NOT_SUPPORTED";
        case MQTTV5_USE_ANOTHER_SERVER:
            return "MQTTV5_USE_ANOTHER_SERVER";
        case MQTTV5_SERVER_MOVED:
            return "MQTTV5_SERVER_MOVED";
        case MQTTV5_SHARED_SUBSCRIPTION_NOT_SUPPORTED:
            return "MQTTV5_SHARED_SUBSCRIPTION_NOT_SUPPORTED";
        case MQTTV5_CONNECTION_RATE_EXCEEDED:
            return "MQTTV5_CONNECTION_RATE_EXCEEDED";
        case MQTTV5_MAXIMUM_CONNECT_TIME:
            return "MQTTV5_MAXIMUM_CONNECT_TIME";
        case MQTTV5_SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED:
            return "MQTTV5_SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED";
        case MQTTV5_WILDCARD_SUBSCRIPTION_NOT_SUPPORTED:
            return "MQTTV5_WILDCARD_SUBSCRIPTION_NOT_SUPPORTED";
        default:
            return "MQTTV5_UNKNOWN";
    }
}
