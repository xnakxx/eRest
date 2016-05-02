<h1>aREST</h1>

## Overview

If you want to know more about aREST, go over to [http://arest.io/](http://arest.io/).

## Contents

- eREST.h: the library file.
- examples: well just the esp8266 one (that's all I care about)

## Supported hardware

### ESP8266

The library is compatible with most of the ESP8266 modules & ESP8266 development boards.

## Requirements

To use the library with Arduino boards you will need the latest version of the Arduino IDE:

- [Arduino IDE 1.6.8](http://arduino.cc/en/main/software)

### For WiFi using the ESP8266 chip

To use the library with the ESP8266 WiFi chip you will need to install the required module from the Boards Manager of the Arduino IDE. These are the steps to install the ESP8266 package inside the Arduino IDE:

1. Start the Arduino IDE and open the Preferences window
2. Enter `http://arduino.esp8266.com/package_esp8266com_index.json` into the Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
3. Open the Boards Manager from Tools > Board menu and install the esp8266 package (and after that don't forget to select your ESP8266 board from Tools > Board menu).


## Setup

To install the library, simply clone this repository in the /libraries folder of your Arduino folder.

## Quick test (ESP8266)

1. Connect a LED & resistor to pin number 5 of your ESP8266 board
2. Open the ESP8266 example sketch and modify the WiFi SSID & password
3. Upload the sketch
4. Open the Serial monitor to get the IP address of the board, for example 192.168.1.103
5. Go to a web browser and type `192.168.1.103/mode/5/o` to set the pin as an output
6. Now type `192.168.1.103/digital/5/1` and the LED should turn on

## API documentation

The API currently supports five type of commands: digital, analog, and mode, variables, and user-defined functions.

### Digital

Digital is to write or read on digital pins on the Arduino. For example:
  * `/digital/8/0` sets pin number 8 to a low state
  * `/digital/8/1` sets pin number 8 to a high state
  * `/digital/8` reads value from pin number 8 in JSON format (note that for compatibility reasons, `/digital/8/r` produces the same result)

### Analog

Analog is to write or read on analog pins on the Arduino. Note that you can only write on PWM pins for the Arduino Uno, and only read analog values from analog pins 0 to 5. For example:
  * `/analog/6/123` sets pin number 6 to 123 using PWM
  * `/analog/0` returns analog value from pin number A0 in JSON format (note that for compatibility reasons, `/analog/0/r` produces the same result)

### Mode

Mode is to change the mode on a pin. For example:
  * `/mode/8/o` sets pin number 8 as an output
  * `/mode/8/i` sets pin number 8 as an input

### Variables

You can also directly call variables that are defined in your sketch. Integer variables are supported by the library. Float and String variables are also supported, but only by the Arduino Mega board & by the ESP8266.

To access a variable in your sketch, you have to declare it first, and then call it from with a REST call. For example, if your aREST instance is called "rest" and the variable "temperature":
  * `rest.variable("temperature",&temperature);` declares the temperature in the Arduino sketch
  * `/temperature` returns the value of the variable in JSON format

### Functions

You can also define your own functions in your sketch that can be called using the REST API. To access a function defined in your sketch, you have to declare it first, and then call it from with a REST call. Note that all functions needs to take a String as the unique argument (for parameters to be passed to the function) and return an integer. For example, if your aREST instance is called "rest" and the function "ledControl":
  * `rest.function("led",ledControl);` declares the function in the Arduino sketch
  * `/led?params=0` executes the function

### Get data about the board

You can also access a description of all the variables that were declared on the board with a single command. This is useful to automatically build graphical interfaces based on the variables exposed to the API. This can be done via the following calls:
  * `/` or `/id`
  * The names & types of the variables will then be stored in the variables field of the returned JSON object

### Status LED (BETA)

To know the activity of the library while the sketch is running, there is the possibility to connect a LED to a pin to show this activity in real-time. Simply connect a 220 Ohm resistor in series with a 5mm LED to the pin of your choice, and enter this line in the setup() function of your Arduino sketch:

```c
rest.set_status_led(led_pin);
```
