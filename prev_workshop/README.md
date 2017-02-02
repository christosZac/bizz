# Code examples for the Decentralized architectures & sensor networks in IoT: 6LoWPAN, border routers and gateways workshop

## Introduction

Below you can find general info about setting, flashing, using and
accessing the CoAP server and client implementations used in the workshop.

## Hardware used

 1. [Zolertia Firefly](zolertia.io/product/hardware/firefly): the
    sensor node.
 2. [HIH 6130](https://www.sparkfun.com/products/11295) breakout
    board: the sensor (temperature + humidity).
 3. [AM 2315]() I<sup>2</sup>C sensor (temperature + humidity). (Optional; can be used in client example)

## Requirements

Repositories and libraries must have been set and installed as
described
[here](https://github.com/relayr/workshop-6lowpan/blob/workshop/README.md).

## Server: `er-server.c`

This CoAP server allows you to remotely access its resources using
REST requests. By default the server has the following resources:
    

| Path |  Resource   	     | Method  | Description 
|---   |---			         |---	   |---
|      | `.well-known/core/` | `GET`   | Provides list of available resources.
| `sensors/`    | `hih6130/temperature/`</br>`hih6130/humidity/` | `GET` | Returns ambient temperature in Celsius and relative humidity from the HIH6130 sensor.
| `actuators/`  | `toggle/`  	  | `POST`    | Toggles device LED (on/off).
| `observables/`| `humidityALERT/` | `OBSERVE` | Subscribes to resource and gets notified when humidity exceeds a certain level.
 

### Activating resources
 
 You can activate or deactivate resources simply by including a
 function, which binds them with a unique path. Do not forget to also
 activate any hardware a resource might need. The activation is done
 just after the REST engine is initialized in the `PROCESS`:
 
 ```c
/* Initialize the REST engine. */
rest_init_engine();

/* Set our resources paths and also activate the sensor. */
rest_activate_resource(&res_hih6130_temp, "sensors/hih6130/temperature");
rest_activate_resource(&res_hih6130_hum, "sensors/hih6130/humidity");

/* Activating the HIH6130 sensor. */
SENSORS_ACTIVATE(hih6130);
 ```
### Compiling & Flashing the server on the Firefly

In the `workshop` directory:
   
 * For compiling issue the command:
```shell
 make TARGET=zoul BOARD=firefly er-server
```

 * For uploading the code issue the command: 
```shell
 make TARGET=zoul BOARD=firefly er-server.upload
```

Now the mote is accessible using its IPv6 address.
 
## Client: `er-client.c`
 
The CoAP client is used for sending data to the
[relayr](https://developer.relayr.io) cloud.  Periodically a CoAP
request is sent to a given server address. In the given example, the
client probes the [HIH6130](https://www.sparkfun.com/products/11295)
sensor and sends two messages (temperature and humidity) to a remote
CoAP server, which then sends the data to the relayr cloud.
 
### Setting up the client
 
Inside the code many variables must be set before flashing the code:

 1. Begin with setting the border router address (default [fd00::1]),
    the CoAP port the server is listening to (default 5683) as well as
    the path, where the request should be done:

 ```c
/* Define the border router settings. */
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0x1)
#define REMOTE_PORT     	  UIP_HTONS(8181)
#define URL_PATH 			  "/target"
 ```

 2. Next, specify the credentials in order to successfully connect to
    the cloud, and choose the time interval in seconds, for probing
    the data:

 ```c
 /* Interval for setting data to the server. */
 #define TOGGLE_INTERVAL 5
 
 /* Path info and cloud credentials. */
 #define DEVICE_ID "<Your device ID here>"
 #define USER_TOKEN "<Your token here>" 
 ```

### Flashing the client on a Firefly mote

### Compiling & Flashing the server on the Firefly

In the `workshop` directory:
   
 * For compiling issue the command:
```shell
make TARGET=zoul BOARD=firefly SENSOR=hih6130 er-client
```
 
 * For uploading the code: 

```shell
make TARGET=zoul BOARD=firefly SENSOR=hih6130 er-client.upload
``` 

 * Optionally, the AM2315 sensor can be used by setting the compile flag to ```SENSOR=am2315```

## References

Additional information on some of the topics addressed in the workshop.

 * [CoAP](http://coap.technology).
 * [Zolertia Firefly](https://github.com/Zolertia/Resources/wiki/Firefly).
 * [relayr cloud](http://docs.relayr.io).
 * [Erbium REST engine](http://people.inf.ethz.ch/mkovatsc/erbium.php).

## TODO

 1. Improve code for extracting temperature and humidity
    values. Organize the code better: .h and .c.
 2. Implement [RPL](https://tools.ietf.org/html/rfc6550) demo to
    explore routing and network topologies for constrained devices.
 3. Implement [CBOR](http://cbor.io) as the data format that is most
    appropriate for constrained devices.
 4. Implement
    [SenML](https://datatracker.ietf.org/doc/draft-ietf-core-senml)
    as the proper way to use JSON with constrained devices.
 5. Experiment with radio range using better antennas then the simple
    whip.  Something like this
    [Yagi](https://www.alibaba.com/product-detail/Gold-Supplier-16dbi-3g-yagi-antenna_60506856488.html).
 6. Experiment with battery based power supply and deep sleep. 
