# Running Paho Embedded Tests

## Requirements

The tests require both Python v2 and Mosquitto MQTT. On Ubuntu, these can be installed using
`sudo apt install python2 mosquitto`.

## Running the tests

1. Start the servers

```
cd build
mosquitto &
python3 ../test/mqttsas.py localhost 1883 1885 &
```

2. Run the tests

```ctest -VV --timeout 600```

3. Stop servers

```
    kill %% 
    kill %%
```
