# *modbus_datalogger*
----
## Description

> This project uses 2 ESP8266 NODEMCU module (server and client), DHT11 temperature and humidity sensor and 4 relays with Arduino IDE to send the temperature and humidity readings to ubidots platform and cotrol the relays from switches on ubidots.
___
## External Libraries

### Wifi Manager
We are using wifi manager library by tzapu to manage the changes in the WiFi settings (SSID and Password), So thanks to tzapu. To install wifi manager [library](https://github.com/tzapu/WiFiManager).

### DHTesp
We are using DHTesp library file by “beegee Tokyo” to read the temperature and humidity easily from DHT11 sensor, thanks to beegee Tokyo. To install DHTesp [library](https://github.com/beegee-tokyo/DHTesp).

### ArduinoJson
We are using andruino json library by Benoît Blanchon to create and read the configration file in the SPIFFS, Benoît Blanchon thank you. To install ArduinoJson [library](https://github.com/bblanchon/ArduinoJson).

### Adafruit_MQTT_Library﻿

 Last external library we use in this project is the adafruit MQTT library by Adafruit, big thanks to Adafruit. To Install the adafruit MQTT library for [Arduino](https://github.com/adafruit/Adafruit_MQTT_Library).
___
## How It Works

### - server

### - client

___
The ESP8266 reads the temperature and the humidity through the DHT11 sensor. It connects to the Adafruit IO website through WiFi and sends the sensor readings to the feeds through MQTT. We have setup a Webserver on the ESP8266 to configure the Adafruit IO logging parameters:

   1. Start/Stop sending the data.
   2. The frequency of sending.
   3. Adafruit IO API key.
   
All this is done through the “control settings” HTML page that’s hosted in ESP8266 web server.

## Quick Start

To run the code you need to:

 1. Install the wifi manager library from [github](https://github.com/tzapu/WiFiManager). or from the tools menu in the Arduino IDE. choose “Manage libraries”, then type wifimanager in the search bar and install the one from tzapu. I used version 0.14.0.
 2. Install the DHTesp library from [github](https://github.com/beegee-tokyo/DHTesp), or from the library Manager, type DHTesp in the search bar and install it. I used version 1.0.9.
 3.	Install the ArduinoJson  library from [github](https://github.com/bblanchon/ArduinoJson), Or in Manage libraries type ArduinoJson in the search bar and install it.
 4.	Install the adafruit MQTT library from [github](https://github.com/adafruit/Adafruit_MQTT_Library) or from the tools menu in the Arduino IDE. choose “Manage libraries” , and type adafruit MQTT library in the search bar and install it.
 5. Upload the HTML files in the data folder to the SPIFFS by clicking “ESP8266 sketch data upload” from the tools menu of the Arduino IDE,  be sure that the serial window is closed. If you do not have the “ESP8266 sketch data upload” in the tools menu please check this [link](https://github.com/esp8266/arduino-esp8266fs-plugin) to install the plugin. This will add the control settings page to the ESP8266 web server.

For more details please check our [blog post](https://www.the-diy-life.co/2019/09/03/esp8266-and-adafruit-io/)

----
## Thanks
