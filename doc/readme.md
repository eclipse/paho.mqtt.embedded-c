# Generating Paho Embedded Documentation

## Requirements

Download and install Doxygen. On Ubuntu, this can be insatalled using `sudo apt install doxygen`.

## Genrating HTML documentation

From the root of this repository:

```
# Generate the MQTTPacket Documentation
doxygen ./doc/DoxyfileMQTTPacket.in

# Generate the MQTTClient-C Documentation
doxygen ./doc/DoxyfileMQTTClient-C.in

# Generate the MQTTClient (C++) Documentation
doxygen ./doc/DoxyfileMQTTClient.in

```