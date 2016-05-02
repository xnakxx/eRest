/*
  eREST Library for Arduino
  totally copied from Marco Schwartz's aRest lib
  See the README file for more details.

  Written in 2014 by Marco Schwartz. modified by xnakxx 2016

  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License:
  http://creativecommons.org/licenses/by-sa/4.0/
  Version 0.1
  
  aRest Version 2.2.0
   Original Changelog:

  Version 2.2.0: Added compatibility with the Arduino MKR1000 board
  Version 2.1.2: Added data about hardware type in JSON answer
  Version 2.1.1: Fixed analogWrite() for ESP8266 chips
  Version 2.1.0: Added publish() function
  Version 2.0.2: Able to change MQTT remote server
  Version 2.0.2: Added cloud access support for the Ethernet library
  Version 2.0.1: Added beta support for cloud access via cloud.arest.io
  Version 2.0.0: Added beta support for MQTT communications
  Version 1.9.10: Added support for floats & Strings for Uno (without the CC3000 chip)
  Version 1.9.8: Added support for ESP8266 chip
  Version 1.9.7: Added support for Arduino 1.6.2
  Version 1.9.6: Added support for float variables for Arduino Mega
  Version 1.9.5: Added compatibility with Arduino IDE 1.5.8
  Version 1.9.4: Bug fixes & added support for configuring analog pints as digital outputs
  Version 1.9.3: Added description of available variables for the /id and / routes
  Version 1.9.2: Added compatibility with the Arduino WiFi library
  Version 1.9.1: Added compatibility with CORS
  Version 1.9: New speedup of the library (answers 2x faster in HTTP compared to version 1.8)
  Version 1.8: Speedup of the library (answers 2.5x faster with the CC3000 WiFi chip)
  Version 1.7.5: Reduced memory footprint of the library
  Version 1.7.4: Added a function to read all analog & digital inputs at once
  Version 1.7.3: Added LIGHTWEIGHT mode to only send limited data back
  Version 1.7.2: Added possibility to assign a status pin connected to a LED
  Version 1.7.1: Added possibility to change number of exposed variables & functions
  Version 1.7: Added compatibility with the Arduino Due & Teensy 3.x
  Version 1.6: Added compatibility with the Arduino Yun

  Version 1.5: Size reduction, and added compatibility with Adafruit BLE

  Version 1.4: Added authentification with API key

  Version 1.3: Added support for the Ethernet shield

  Version 1.2: Added support of Serial communications

  Version 1.1: Added variables & functions support

  Version 1.0: First working version of the library
*/

#ifndef eRest_h
#define eRest_h

// Include Arduino header
#include "Arduino.h"

// Using ESP8266 ?
#if defined(ESP8266)
#include "stdlib_noniso.h"
#endif

// Which board?
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
#define NUMBER_ANALOG_PINS 16
#define NUMBER_DIGITAL_PINS 54
#define OUTPUT_BUFFER_SIZE 2000
#else
#define NUMBER_ANALOG_PINS 6
#define NUMBER_DIGITAL_PINS 14
#define OUTPUT_BUFFER_SIZE 350
#endif

// Hardware data
#if defined(ESP8266)
#define HARDWARE "esp8266"
#else
#define HARDWARE "arduino"
#endif

// Size of name & ID
#define NAME_SIZE 20
#define ID_SIZE 10

// Debug mode
#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

// Default number of max. exposed variables
#ifndef NUMBER_VARIABLES
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
  #define NUMBER_VARIABLES 10
  #else
  #define NUMBER_VARIABLES 5
  #endif
#endif

// Default number of max. exposed functions
#ifndef NUMBER_FUNCTIONS
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
  #define NUMBER_FUNCTIONS 10
  #else
  #define NUMBER_FUNCTIONS 5
  #endif
#endif

class eREST {

public:

eREST() {

  command = 'u';
  pin_selected = false;

  status_led_pin = 255;
  state = 'u';

}

eREST(char* rest_remote_server, int rest_port) {

  command = 'u';
  pin_selected = false;

  status_led_pin = 255;
  state = 'u';

  remote_server = rest_remote_server;
  port = rest_port;

}

// Set status LED
void set_status_led(uint8_t pin){

  // Set variables
  status_led_pin = pin;

  // Set pin as output
  pinMode(status_led_pin,OUTPUT);
}

// Glow status LED
void glow_led() {

  if(status_led_pin != 255){
    unsigned long i = millis();
    int j = i % 4096;
    if (j > 2048) { j = 4096 - j;}
      analogWrite(status_led_pin,j/8);
    }
}

// Send HTTP headers for Ethernet & WiFi
void send_http_headers(){

  addToBuffer(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, GET, PUT, OPTIONS\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));

}

// Reset variables after a request
void reset_status() {

  #if defined(ESP8266)
  Serial.print("Memory loss before reset:");
  Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
  freeMemory = ESP.getFreeHeap();
  #endif

  answer = "";
  command = 'u';
  pin_selected = false;
  state = 'u';
  arguments = "";

  index = 0;
  //memset(&buffer[0], 0, sizeof(buffer));

  #if defined(ESP8266)
  Serial.print("Memory loss after reset:");
  Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
  freeMemory = ESP.getFreeHeap();
  Serial.print("Memory free:");
  Serial.println(freeMemory, DEC);
  #endif

}

// Handle request for the ESP8266 chip
#ifdef ESP8266
void handle(WiFiClient& client){

  if (DEBUG_MODE) {
    Serial.print("Memory loss before available:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
  }

  if (client.available()) {

    if (DEBUG_MODE) {
      Serial.print("Memory loss before handling:");
      Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
      freeMemory = ESP.getFreeHeap();
    }

    // Handle request
    handle_proto(client,true,0);

    if (DEBUG_MODE) {
      Serial.print("Memory loss after handling:");
      Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
      freeMemory = ESP.getFreeHeap();
    }

    // Answer
    sendBuffer(client,0,0);
    client.stop();

    // Reset variables for the next command
    reset_status();
  }
}

#endif

void handle(char * string) {

  // Process String
  handle_proto(string);

  // Reset variables for the next command
  reset_status();
}

void handle_proto(char * string) {
  // Check if there is data available to read
  for (int i = 0; i < strlen(string); i++){

    char c = string[i];
    answer = answer + c;

    // Process data
    process(c);

  }

  // Send command
  send_command(false);
}

template <typename T, typename V>
void publish_proto(T& client, String eventName, V value) {

  // Format data
  String data = "name=" + eventName + "&data=" + String(value);

  Serial.println("POST /" + String(id) + "/events HTTP/1.1");
  Serial.println("Host: " + String(remote_server) + ":" + String(port));
  Serial.println(F("Content-Type: application/x-www-form-urlencoded"));
  Serial.print(F("Content-Length: "));
  Serial.println(data.length());
  Serial.println();
  Serial.print(data);

  // Send request
  client.println(F("POST /1/events HTTP/1.1"));
  client.println("Host: " + String(remote_server) + ":" + String(port));
  client.println(F("Content-Type: application/x-www-form-urlencoded"));
  client.print(F("Content-Length: "));
  client.println(data.length());
  client.println();
  client.print(data);

}


void process(char c){

  // Check if we are receveing useful data and process it
  if ((c == '/' || c == '\r') && state == 'u') {

      if (DEBUG_MODE) {
        #if defined(ESP8266)
        Serial.print("Memory loss:");
        Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
        freeMemory = ESP.getFreeHeap();
        #endif
        Serial.println(answer);
      }

      // If the command is mode, and the pin is already selected
      if (command == 'm' && pin_selected && state == 'u') {

        // Get state
        state = answer[0];

     }

     // If a digital command has been received, process the data accordingly
     if (command == 'd' && pin_selected && state == 'u') {

       // If it's a read command, read from the pin and send data back
       if (answer[0] == 'r') {state = 'r';}

       // If not, get value we want to apply to the pin
       else {value = answer.toInt(); state = 'w';}
     }

     // If analog command has been selected, process the data accordingly
     if (command == 'a' && pin_selected && state == 'u') {

       // If it's a read, read from the correct pin
       if (answer[0] == 'r') {state = 'r';}

       // Else, write analog value
       else {value = answer.toInt(); state = 'w';}
     }

     // If the command is already selected, get the pin
     if (command != 'u' && pin_selected == false) {

       // Get pin
       if (answer[0] == 'A') {
         pin = 14 + answer[1] - '0';
       }
       else {
         pin = answer.toInt();
       }
       if (DEBUG_MODE) {
        Serial.print("Selected pin: ");
        Serial.println(pin);
       }
       pin_selected = true;

       // Nothing more ?
       if ((answer[1] != '/' && answer[2] != '/')
        || (answer[1] == ' ' && answer[2] == '/')
        || (answer[2] == ' ' && answer[3] == '/')) {

        // Nothing more & digital ?
        if (command == 'd') {

          // Read all digital ?
          if (answer[0] == 'a') {state = 'a';}

          // Save state & end there
          else {state = 'r';}
        }

       // Nothing more & analog ?
       if (command == 'a') {

         // Read all analog ?
         if (answer[0] == 'a') {state = 'a';}

         // Save state & end there
         else {state = 'r';}
       }
     }

   }

     // Digital command received ?
     if (answer.startsWith("digital")) {command = 'd';}

     // Mode command received ?
     if (answer.startsWith("mode")) {command = 'm';}

     // Analog command received ?
     if (answer.startsWith("analog")) {command = 'a';}

     // Variable or function request received ?
     if (command == 'u') {

       // Check if variable name is in int array
       for (uint8_t i = 0; i < variables_index; i++){
         if(answer.startsWith(int_variables_names[i])) {

           // End here
           pin_selected = true;
           state = 'x';

           // Set state
           command = 'v';
           value = i;
         }
       }

       // Check if variable name is in float array (Mega & ESP8266 only)
       #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
       for (uint8_t i = 0; i < float_variables_index; i++){
         if(answer.startsWith(float_variables_names[i])) {

           // End here
           pin_selected = true;
           state = 'x';

           // Set state
           command = 'l';
           value = i;
         }
       }
       #endif

       // Check if variable name is in float array (Mega & ESP8266 only)
       #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
       for (uint8_t i = 0; i < string_variables_index; i++){
         if(answer.startsWith(string_variables_names[i])) {

           // End here
           pin_selected = true;
           state = 'x';

           // Set state
           command = 's';
           value = i;
         }
       }
       #endif

       // Check if function name is in array
       for (uint8_t i = 0; i < functions_index; i++){
         if(answer.startsWith(functions_names[i])) {

           // End here
           pin_selected = true;
           state = 'x';

           // Set state
           command = 'f';
           value = i;

           // Get command
           arguments = "";
           uint8_t header_length = strlen(functions_names[i]);
           if (answer.substring(header_length, header_length + 1) == "?") {
             uint8_t footer_start = answer.length();
             if (answer.endsWith(" HTTP/"))
               footer_start -= 6; // length of " HTTP/"
             arguments = answer.substring(header_length + 8, footer_start);
           }
         }
       }

       // If the command is "id", return device id, name and status
       if ( (answer[0] == 'i' && answer[1] == 'd') ){

           // Set state
           command = 'i';

           // End here
           pin_selected = true;
           state = 'x';
       }

       if (answer[0] == ' '){

           // Set state
           command = 'r';

           // End here
           pin_selected = true;
           state = 'x';
       }

       // Check the type of HTTP request
       // if (answer.startsWith("GET")) {method = "GET";}
       // if (answer.startsWith("POST")) {method = "POST";}
       // if (answer.startsWith("PUT")) {method = "PUT";}
       // if (answer.startsWith("DELETE")) {method = "DELETE";}

       // if (DEBUG_MODE && method != "") {
       //  Serial.print("Selected method: ");
       //  Serial.println(method);
       // }

     }

     answer = "";
    }
}

bool send_command(bool headers) {

   if (DEBUG_MODE) {

     #if defined(ESP8266)
     Serial.print("Memory loss:");
     Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
     freeMemory = ESP.getFreeHeap();
     #endif

     Serial.println(F("Sending command"));
     Serial.print(F("Command: "));
     Serial.println(command);
     Serial.print(F("State: "));
     Serial.println(state);
     Serial.print(F("State of buffer at the start: "));
     Serial.println(buffer);
   }

   // Start of message
   if (headers && command != 'r') {send_http_headers();}

   // Mode selected
   if (command == 'm'){

     // Send feedback to client
       addToBuffer(F("{\"message\": \"Pin D"));
       addToBuffer(pin);

     // Input
     if (state == 'i'){

      // Set pin to Input
      pinMode(pin,INPUT);

      // Send feedback to client
      addToBuffer(F(" set to input\", "));}
     }

     // Output
     if (state == 'o'){

       // Set to Output
       pinMode(pin,OUTPUT);

       // Send feedback to client
       addToBuffer(F(" set to output\", "));
     }

   }

   // Digital selected
   if (command == 'd') {
     if (state == 'r'){

       // Read from pin
       value = digitalRead(pin);

       // Send answer
       if (LIGHTWEIGHT){addToBuffer(value);}
       else {
        addToBuffer(F("{\"return_value\": "));
        addToBuffer(value);
        addToBuffer(F(", "));
      }
     }

     #if !defined(__AVR_ATmega32U4__) || !defined(ADAFRUIT_CC3000_H)
     if (state == 'a') {
       addToBuffer(F("{"));

       for (uint8_t i = 0; i < NUMBER_DIGITAL_PINS; i++) {

         // Read analog value
         value = digitalRead(i);

         // Send feedback to client
           addToBuffer(F("\"D"));
           addToBuffer(i);
           addToBuffer(F("\": "));
           addToBuffer(value);
           addToBuffer(F(", "));
     }
    }
    #endif

     if (state == 'w') {

       // Disable analogWrite if ESP8266
       #if defined(ESP8266)
       analogWrite(pin, 0);
       #endif

       // Apply on the pin
       digitalWrite(pin,value);

       // Send feedback to client
      
        addToBuffer(F("{\"message\": \"Pin D"));
        addToBuffer(pin);
        addToBuffer(F(" set to "));
        addToBuffer(value);
        addToBuffer(F("\", "));

     }
   }

   // Analog selected
   if (command == 'a') {
     if (state == 'r'){

       // Read analog value
       value = analogRead(pin);

       // Send feedback to client
       
        addToBuffer(F("{\"return_value\": "));
        addToBuffer(value);
        addToBuffer(F(", "));
     }
     #if !defined(__AVR_ATmega32U4__)
     if (state == 'a') {
       addToBuffer(F("{"));

       for (uint8_t i = 0; i < NUMBER_ANALOG_PINS; i++) {

         // Read analog value
         value = analogRead(i);

         // Send feedback to client
        
           addToBuffer(F("\"A"));
           addToBuffer(i);
           addToBuffer(F("\": "));
           addToBuffer(value);
           addToBuffer(F(", "));

     }
   }
   #endif
   if (state == 'w') {

     // Write output value
     analogWrite(pin,value);

     // Send feedback to client
     addToBuffer(F("{\"message\": \"Pin D"));
     addToBuffer(pin);
     addToBuffer(F(" set to "));
     addToBuffer(value);
     addToBuffer(F("\", "));

   }
  }

  // Variable selected
  if (command == 'v') {

       // Send feedback to client
  
        addToBuffer(F("{\""));
        addToBuffer(int_variables_names[value]);
        addToBuffer(F("\": "));
        addToBuffer(*int_variables[value]);
        addToBuffer(F(", "));

  }

  // Float ariable selected (Mega only)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
  if (command == 'l') {

       // Send feedback to client

        addToBuffer(F("{\""));
        addToBuffer(float_variables_names[value]);
        addToBuffer(F("\": "));
        addToBuffer(*float_variables[value]);
        addToBuffer(F(", "));

  }
  #endif

  // String variable selected (Mega & ESP8266 only)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
  if (command == 's') {

       // Send feedback to client

        addToBuffer(F("{\""));
        addToBuffer(string_variables_names[value]);
        addToBuffer(F("\": \""));
        addToBuffer(*string_variables[value]);
        addToBuffer(F("\", "));

  }
  #endif

  // Function selected
  if (command == 'f') {

    // Execute function
    uint8_t result = functions[value](arguments);

    // Send feedback to client

     addToBuffer(F("{\"return_value\": "));
     addToBuffer(result);
     addToBuffer(F(", "));
     //addToBuffer(F(", \"message\": \""));
     //addToBuffer(functions_names[value]);
     //addToBuffer(F(" executed\", "));

  }

  if (command == 'r' || command == 'u') {
    root_answer();
  }

  if (command == 'i') {

      addToBuffer(F("{"));

  }

   // End of message
  

     if (command != 'r' && command != 'u') {
       addToBuffer(F("\"id\": \""));
       addToBuffer(id);
       addToBuffer(F("\", \"name\": \""));
       addToBuffer(name);
       #if !defined(PubSubClient_h)
       addToBuffer(F("\", \"hardware\": \""));
       addToBuffer(HARDWARE);
       #endif
       addToBuffer(F("\", \"connected\": true}\r\n"));
     }


   if (DEBUG_MODE) {
     #if defined(ESP8266)
     Serial.print("Memory loss:");
     Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
     freeMemory = ESP.getFreeHeap();
     #endif
     Serial.print(F("State of buffer at the end: "));
     Serial.println(buffer);
   }

   // End here
   return true;
}

virtual void root_answer() {

  #if defined(ADAFRUIT_CC3000_H) || defined(ESP8266) || defined(ethernet_h) || defined(WiFi_h)
    #if !defined(PubSubClient_h)
      if (command != 'u') {
        addToBuffer(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, GET, PUT, OPTIONS\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
      }
    #endif
  #endif

  
  


    // Start
    addToBuffer(F("{\"variables\": {"));

    #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266)

    // Int variables
    if (variables_index == 0 && string_variables_index == 0 && float_variables_index == 0){
      addToBuffer(F(" }, "));
    }
    else {

      if (variables_index > 0){

        for (uint8_t i = 0; i < variables_index; i++){
          addToBuffer(F("\""));
          addToBuffer(int_variables_names[i]);
          addToBuffer(F("\": "));
          addToBuffer(*int_variables[i]);
          addToBuffer(F(", "));
        }

      }
      if (string_variables_index > 0){

        for (uint8_t i = 0; i < string_variables_index; i++){
          addToBuffer(F("\""));
          addToBuffer(string_variables_names[i]);
          addToBuffer(F("\": \""));
          addToBuffer(*string_variables[i]);
          addToBuffer(F("\", "));
        }

      }
      if (float_variables_index > 0){

        for (uint8_t i = 0; i < float_variables_index; i++){
          addToBuffer(F("\""));
          addToBuffer(float_variables_names[i]);
          addToBuffer(F("\": "));
          addToBuffer(*float_variables[i]);
          addToBuffer(F(", "));
        }

      }
      removeLastBufferChar();
      removeLastBufferChar();
      addToBuffer(F("}, "));

    }
    #else
    // Int variables
    if (variables_index > 0){

      for (uint8_t i = 0; i < variables_index-1; i++){
        addToBuffer(F("\""));
        addToBuffer(int_variables_names[i]);
        addToBuffer(F("\": "));
        addToBuffer(*int_variables[i]);
        addToBuffer(F(", "));
      }

      // End
      addToBuffer(F("\""));
      addToBuffer(int_variables_names[variables_index-1]);
      addToBuffer(F("\": "));
      addToBuffer(*int_variables[variables_index-1]);
      addToBuffer(F("}, "));
    }
    else {
      addToBuffer(F(" }, "));
    }
    #endif

  

  // End
  addToBuffer(F("\"id\": \""));
  addToBuffer(id);
  addToBuffer(F("\", \"name\": \""));
  addToBuffer(name);
  addToBuffer(F("\", \"connected\": true}\r\n"));
}

void variable(char * variable_name, int *variable){

  int_variables[variables_index] = variable;
  int_variables_names[variables_index] = variable_name;
  variables_index++;

}

// Float variables (Mega & ESP only, or without CC3000)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) ||
void variable(char * variable_name, float *variable){

  float_variables[float_variables_index] = variable;
  float_variables_names[float_variables_index] = variable_name;
  float_variables_index++;

}
#endif

// String variables (Mega & ESP only, or without CC3000)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) 
void variable(char * variable_name, String *variable){

  string_variables[string_variables_index] = variable;
  string_variables_names[string_variables_index] = variable_name;
  string_variables_index++;

}
#endif

void function(char * function_name, int (*f)(String)){

  functions_names[functions_index] = function_name;
  functions[functions_index] = f;
  functions_index++;
}

// Set device ID
void set_id(char *device_id){

  strncpy(id,device_id, ID_SIZE);

}

// Set device name
void set_name(char *device_name){

  strcpy(name, device_name);
}

// Set device name
void set_name(String device_name){

  device_name.toCharArray(name, NAME_SIZE);
}

// Set device ID
void set_id(String device_id){

  device_id.toCharArray(id, ID_SIZE);
  set_id(id);
}

// Remove last char from buffer
void removeLastBufferChar() {

  index = index - 1;

}

// Add to output buffer
void addToBuffer(char * toAdd){

  if (DEBUG_MODE) {
    #if defined(ESP8266)
    Serial.print("Memory loss:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
    #endif
    Serial.print(F("Added to buffer as char: "));
    Serial.println(toAdd);
  }

  for (int i = 0; i < strlen(toAdd); i++){
    buffer[index+i] = toAdd[i];
  }
  index = index + strlen(toAdd);
}

// Add to output buffer
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
void addToBuffer(String toAdd){

  if (DEBUG_MODE) {
    #if defined(ESP8266)
    Serial.print("Memory loss:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
    #endif
    Serial.print(F("Added to buffer as String: "));
    Serial.println(toAdd);
  }

  for (int i = 0; i < toAdd.length(); i++){
    buffer[index+i] = toAdd[i];
  }
  index = index + toAdd.length();
}
#endif

// Add to output buffer
void addToBuffer(uint16_t toAdd){

  char number[10];
  itoa(toAdd,number,10);

  addToBuffer(number);
}

// Add to output buffer
void addToBuffer(int toAdd){

  char number[10];
  itoa(toAdd,number,10);

  addToBuffer(number);
}

// Add to output buffer (Mega & ESP only)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) 
void addToBuffer(float toAdd){

  char number[10];
  dtostrf(toAdd, 5, 2, number);

  addToBuffer(number);
}
#endif

// Add to output buffer
void addToBuffer(const __FlashStringHelper *toAdd){

  if (DEBUG_MODE) {
    #if defined(ESP8266)
    Serial.print("Memory loss:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
    #endif
    Serial.print(F("Added to buffer as progmem: "));
    Serial.println(toAdd);
  }

  uint8_t idx = 0;

  PGM_P p = reinterpret_cast<PGM_P>(toAdd);

  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    buffer[index + idx] = c;
    idx++;
  }
  index = index + idx;
}

template <typename T>
void sendBuffer(T& client, uint8_t chunkSize, uint8_t wait_time) {

  if (DEBUG_MODE) {
    #if defined(ESP8266)
    Serial.print("Memory loss before sending:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
    #endif
    Serial.print(F("Buffer size: "));
    Serial.println(index);
  }

  // Send all of it
  if (chunkSize == 0) {
    client.print(buffer);
  }

  // Send chunk by chunk
  else {

    // Max iteration
    uint8_t max_iteration = (int)(index/chunkSize) + 1;

    // Send data
    for (uint8_t i = 0; i < max_iteration; i++) {
      char intermediate_buffer[chunkSize+1];
      memcpy(intermediate_buffer, buffer + i*chunkSize, chunkSize);
      intermediate_buffer[chunkSize] = '\0';

      // Send intermediate buffer
  
      client.print(intermediate_buffer);

      // Wait for client to get data
      delay(wait_time);

      if (DEBUG_MODE) {
        Serial.print(F("Sent buffer: "));
        Serial.println(intermediate_buffer);
      }
    }
  }

  if (DEBUG_MODE) {
    #if defined(ESP8266)
    Serial.print("Memory loss after sending:");
    Serial.println(freeMemory - ESP.getFreeHeap(),DEC);
    freeMemory = ESP.getFreeHeap();
    #endif
    Serial.print(F("Buffer size: "));
    Serial.println(index);
  }

    // Reset the buffer
    resetBuffer();
}

char * getBuffer() {
  return buffer;
}

void resetBuffer(){

  memset(&buffer[0], 0, sizeof(buffer));
  // free(buffer);

}

// For non AVR boards
#if defined (__arm__)
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}
#endif

// Memory debug
#if defined(ESP8266)
void initFreeMemory(){
  freeMemory = ESP.getFreeHeap();
}
#endif


private:
  String answer;
  char command;
  uint8_t pin;
  char state;
  uint16_t value;
  boolean pin_selected;

  char* remote_server;
  int port;

  char name[NAME_SIZE];
  char id[ID_SIZE+1];
  String arguments;

  // Output uffer
  char buffer[OUTPUT_BUFFER_SIZE];
  uint16_t index;

  // Status LED
  uint8_t status_led_pin;

  // Int variables arrays
  uint8_t variables_index;
  int * int_variables[NUMBER_VARIABLES];
  char * int_variables_names[NUMBER_VARIABLES];


  // Float variables arrays (Mega & ESP8266 only)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
  uint8_t float_variables_index;
  float * float_variables[NUMBER_VARIABLES];
  char * float_variables_names[NUMBER_VARIABLES];
  #endif

  // String variables arrays (Mega & ESP8266 only)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(ESP8266) || defined(CORE_WILDFIRE) || !defined(ADAFRUIT_CC3000_H)
  uint8_t string_variables_index;
  String * string_variables[NUMBER_VARIABLES];
  char * string_variables_names[NUMBER_VARIABLES];
  #endif

  // Functions array
  uint8_t functions_index;
  int (*functions[NUMBER_FUNCTIONS])(String);
  char * functions_names[NUMBER_FUNCTIONS];

  // Memory debug
  #if defined(ESP8266)
  int freeMemory;
  #endif

};

#endif
