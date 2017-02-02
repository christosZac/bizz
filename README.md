# Save the bees! A 6loWPAN approach

## Introduction

This repository contains the code and libraries used for [Save the bees](https://github.com/save-the-bees). This project includes monitoring the ambient conditions and status of distant beehives on the field. The data can be visualized and be accessible worldwide through a cloud service. The current implementation aims at using constrained devices with priority to low energy protocols regarding radio communication and hardware management.    

## Requirements
### Hardware
* Zolertia [Remote](https://github.com/Zolertia/Resources/wiki/RE-Mote) or [Firefly](https://github.com/Zolertia/Resources/wiki/Firefly) board.
* [AM 2315](https://cdn-shop.adafruit.com/datasheets/AM2315.pdf) I2C humidity/temperature sensor.  
* [HIH 6130](https://www.sparkfun.com/products/11295) I2C humidity/temperature sensor.  
* 4 x [FC 2231](http://www.mouser.com/ds/2/418/FC22-710299.pdf) load cell sensors. 

*Only one humidity/temperature sensor is used. You can enable/disable sensors at compile time.* 

### Software
* A **6loWPAN gateway**. This will be the link to the IPv4 world. It encorporates a border-router to communicate with 6loWPAN motes and a proxy-server to receive and forward CoAP requests. Analytic installation steps can be found [here]().
* **ContikiOS** tools and libraries, in order to flash the beehive nodes. [Here](https://github.com/Zolertia/Resources/wiki/Toolchain-and-tools) you can find more information for installation.
* [relayr Dashboard](https://dev.relayr.io) account.


## Beehive Client
The client running on the beehive mote. The mote is responsible for tracking information such as the internal/external temperature & humidity, as well as the current weight of the beehive. The beehives create a mesh network using a sub-GHz frequency radio. The ones closer to the border-router communicate directly and forward traffic from and to distant beehives. Through the border-router (6loWPAN gateway), the data are uploaded to a cloud service.

### Setting up
* Gateway settings:  
First the address of the border-router (default: ```fd00::1```), and the port of the CoAP server (default: ```8181```) must be specified: 

	```c
	#define SERVER_NODE(ipaddr) uip_ip6addr(ipaddr, 0xfd00, 	0, 0, 0, 0, 0, 0, 1) 
	#define REMOTE_PORT     UIP_HTONS(8181)
	```
Then the path the server is listening to:

	```c
	#define URL_PATH "/target"
	```
* Radio settings:  
The motes can work both in 2.4 GHz and Sub-GHz frequencies. This can be changed from the configuration header ```project-conf.c```, by (un)defining ```CC1200_CONF_SUBGHZ_50KBPS_MODE```.

* Sensor settings:  
The humidity/temperature sensors are calibrated from the box and need no further adjustment.  
The load cells must be calibrated due to the fact that we use four of them and usually the sensing values are not identical for the same weight. The constants used for converting to grams can be set through the ```fc2231.h``` file, by defining ```FC2231_CONSTANT``` and ```FC2231_SLOPE``` constants. More info on how to calculate these values can be found in the **Calibrating** section below.

### Compiling
1. Plug in the Zolertia mote.
2. Go to the ```beehive/``` directory. 
3. Execute:

	```shell 
	make TARGET=zoul SENSOR=am2315 client.upload
	```
	*Note that ```SENSOR``` can also be hih6130*.	
4. The mote is ready to deploy!


### Calibrating
The values reported here are pre-calculated and defined. Slight changes between various cells can be found though, so here is a quick guide on calibrating the sensors:  
The load sensors give out a voltage linear to the weight applied on them. That means that the weight-voltage diagram is a straight line and therefore can be calculated using a slope and a constant value. In order to get these values we need two points (aka two known weights) on the plot. A quick reminder of calculating the straight line from two given points can be found [here](https://en.wikipedia.org/wiki/Linear_equation) and an [online tool](http://www.mathportal.org/calculators/analytic-geometry/two-point-form-calculator.php) to do so. The steps for calibrating are:  

1. In the ```dev/fc2231.c``` enable ```DEBUG 1```.
2. Flash the new version by executing 
	
	```shell
	make TARGET=zoul SENSOR=am2315 client.upload
	```
3. From the ```beehive/``` directory execute:

	```shell
	make TARGET=zoul login
	```
	That will give access to serial debugging console.
4. Press the user button on the Remote board to tare the scale. Now write down the total voltage value you get from the sensors as seen on the debug console.
5. Put a known weight on the scale. Write down the sensor reading once more.
6. Now you have the two points needed. Calculate the straight line slope and constant. 
7. Change the ```cf2231.h``` constants: ```FC2231_SLOPE``` and ```FC2231_CONSTANT```.
8. Flash the code again and you are ready to go! 

### Tracking


## References
* [ContikiOS]().
* [relayr. cloud]().
* More on [load cells]()

##TODO
1. Adding more sensors to the beehive
2. Have a hollistic image of the current beehive condition
3. Visualize and aggregate data in a more sophisticated way