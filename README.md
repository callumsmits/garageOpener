# garageOpener
garageOpener provides code to control a garage door opener using two relays and an ultrasonic distance sensor to determine door state. This code merely provides an http interface - please see the associated project [garageController](https://github.com/callumsmits/garageController) for a web interface to interface with this easily.

The interface provided allows the power to the garage door opener to be turned on or off, the door open button to be 'pushed' and the state of the garage door to be determined.

## Hardware
* ESP8266 (e.g. a [D1 Mini](http://www.wemos.cc/Products/d1_mini.html) or a [Adafruit Feather HUZZAH with ESP8266](https://www.adafruit.com/product/2821))
* Two relays (e.g. [Relay shield](http://www.wemos.cc/Products/relay_shield_v2.html) or [Adafruit Power Relay FeatherWing](https://www.adafruit.com/products/3191))
* An ultrasonic distance sensor (e.g. [HC-SR04](http://www.brickelectric.com/measurement-system-c-4/ultrasonic-module-hcsr04-distance-measuring-transducer-sensor-p-15.html))

## Installation
Just clone this repository then install the Arduino IDE and support for the ESP8266. Then open the code, change the ssid and password to that of the desired wifi connection and click upload to install on a connected ESP8266.

## Use
After applying power the device should be available at [http://garage.local](http://garage.local). The following requests are possible:
* [http://garage.local/distance](http://garage.local/distance) returns the distance in cm measured by the ultrasonic distance sensor. It is intended this sensor will be installed such that a long distance is returned if the garage door is closed, but a short distance is returned if the door is open even a little.
* [http://garage.local/doorState](http://garage.local/doorState) A post request to this address containing the JSON `{ door: 1 }` will trigger a pulse of the door open relay. A successful request will return `{ door: 1 }`.
* [http://garage.local/secureState](http://garage.local/secureState) will return the current secureState, where '1' means the garage door opener is secure (i.e. off) and '0' means the door opener is on. A post request to this address containing the desired state (e.g. `{ secure: 1 }`) will set the relay state and return JSON containing the set state.