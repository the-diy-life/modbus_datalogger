# *Modbus Datalogger*

## Description

This project uses 2 ESP8266 NODEMCU module (server and client), DHT11 temperature and humidity sensor and 4 relays with Arduino IDE to send the temperature and humidity readings to ubidots platform and control the relays from switches on ubidots.
___
## External Libraries

### Wifi Manager
We are using wifi manager library by tzapu to manage the changes in the WiFi settings (SSID and Password), So thanks to tzapu. To install wifi manager [library](https://github.com/tzapu/WiFiManager).

### DHTesp
We are using DHTesp library file by “beegee Tokyo” to read the temperature and humidity easily from DHT11 sensor, thanks to beegee Tokyo. To install DHTesp [library](https://github.com/beegee-tokyo/DHTesp).

### ArduinoJson
We are using andruino json library by Benoît Blanchon to create and read the configration file in the SPIFFS, Benoît Blanchon thank you. To install ArduinoJson [library](https://github.com/bblanchon/ArduinoJson).

### Ubidots_MQTT_Library
We are using ubidots MQTT library by ubidots to send and receive data using MQTT protocol, So thanks to ubidots. To install ubidots MQTT [library](https://github.com/ubidots/ubidots-mqtt-esp).

### PubSubClient_Library
As mentioned in the readme of ubidots MQTT library, we need to install the PubSubClient library by knolleary. To install PubSubClient library [library](https://github.com/knolleary/pubsubclient).

### Modbus_Library
We are using modbus library by emelianov to send and receive data using modbus protocol, So thanks to emelianov. To install Modbus [library](https://github.com/emelianov/modbus-esp8266).
___
## How It Works

### - server
The ESP8266 modbus server is connected to 4 relays and the DHT11 sensor, it reads the temperature and the humidity values every 1 second and save them in the modbus holding registers, then when the modbus client needs them it reads these values from the modbus server registers through modbus protocol and update its registers with the new values.

It always looping around the coils registers to see if the client has made changes in it and then update the coils with these new values.

### - client
The ESP8266 modbus client has alot of features, it is connected to the server through modbus, connected to ubidots platform through MQTT
and it is also works as a web server.

It hosts HTML files on its flash memory that you can access them through an HTTP client request to configure :
   1. Your ubidots TOKEN.
   2. The frequency of sending.
   3. Enable/Disable sending the data.

It read the temperature and humidity values from server every configured interval, then sends these values to ubidots.

It also gets the values of the switches from ubidots through MQTT subscribtion then sends them to the modbus server to update the coils.

__NOTE__: If there is any upgrade in the code you can upload it to your ESP Over The Air by requesting the route (/OTA).
___

For more details please check our [blog post](https://www.the-diy-life.co/2019/09/03/esp8266-and-adafruit-io/)

----
## Thanks
