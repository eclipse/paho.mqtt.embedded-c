
#if !defined(MQTTETHERNET_H)
#define MQTTETHERNET_H

#include "MQTT_mbed.h"
#include "EthernetInterface.h"
#include "MQTTSocket.h"

class MQTTEthernet : public MQTTSocket
{
public:    
    MQTTEthernet()
    {
        eth.init();                          // Use DHCP
        eth.connect();
    }
    
private:

    EthernetInterface eth;
    
};


#endif
