#ifndef MQTT_CLIENT_C_NETWORK_H
#define MQTT_CLIENT_C_NETWORK_H

int mqttread(void *network, unsigned char* read_buffer, int, int);
int mqttwrite(void *network, unsigned char* send_buffer, int, int);

void *NetworkInit();
int NetworkConnect(void *network, char*, int);
void NetworkDisconnect(void *network);
void destroyNetwork(void *network);

#endif
