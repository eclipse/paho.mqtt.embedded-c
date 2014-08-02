#if !defined(MQTTSOCKET_H)
#define MQTTSOCKET_H

#include "MQTT_mbed.h"
#include "TCPSocketConnection.h"

class MQTTSocket
{
public:    
    int connect(char* hostname, int port, int timeout=1000)
    {
        mysock.set_blocking(false, timeout);    // 1 second Timeout 
        return mysock.connect(hostname, port);
    }

    int read(unsigned char* buffer, int len, int timeout)
    {
        mysock.set_blocking(false, timeout);  
        return mysock.receive((char*)buffer, len);
    }
    
    int write(unsigned char* buffer, int len, int timeout)
    {
        mysock.set_blocking(false, timeout);  
        return mysock.send((char*)buffer, len);
    }
    
    int disconnect()
    {
        return mysock.close();
    }
    
private:

    TCPSocketConnection mysock; 
    
};



#endif
