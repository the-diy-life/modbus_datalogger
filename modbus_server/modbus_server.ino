/****************************************************************************************
  FileName    : modbus_server.ino
  Author      : The-DIY-life co. - peter.magdy@the-diy-life.co
  Description : This application makes an ESP8266 module as a modbus server that takes
                instructions from a client (master) modbus device.
                It sends values of temperature and humidity read from DHT11 sensor and
                control 4 coils according to the data received.

  ----- based on the commented libraries below -----
  ----- and we modified it to be applicable with our application -----
  Original library
  Copyright by André Sarmento Barbosa
  http://github.com/andresarmento/modbus-arduino

  Current version
  (c)2017 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*****************************************************************************************/

/*************************************** libraries **************************************/
#ifdef ESP8266
  #include <ESP8266WiFi.h>     /* WiFi library for ESP8266 */
#else
  #include <WiFi.h>            /* WiFi library for ESP32 */
#endif
#include <ModbusIP_ESP8266.h>  /* modbus protocol library */
#include "DHTesp.h"            /* DHT11 sensor library */
/* WiFi Manager libraries */
#include <DNSServer.h>
#include <WiFiManager.h>       /* https://github.com/tzapu/WiFiManager */
#include <ArduinoJson.h>       /* https://github.com/bblanchon/ArduinoJson */

/******************************** preprocessor directives *******************************/
#define DHTpin 14    /* D5 of NodeMCU is GPIO14 */

/*********************************** global variables ***********************************/
/* Wifi managerr access point name and password */
char* APName = "AutoConnectAP";
char* APPassword = "password";
char* SSID;
bool shouldSaveConfig = false;  /* flag for saving data */
/* the modbus offset register for temperature and humidity of the DHT11 sensor */
const int DHT_HREG = 100;
/* the modbus offset register for the 4 coils */
const int COIL_REG = 200;
const int coilPin0 = D0;    /* connect the first coil on D0 pin */
const int coilPin1 = D1;    /* connect the first coil on D1 pin */
const int coilPin2 = D2;    /* connect the first coil on D2 pin */
const int coilPin3 = D6;    /* connect the first coil on D6 pin */
uint16_t temperature = 0;   /* the temperature value read from DHT sensor */
uint16_t humidity = 0;      /* the humidity value read from DHT sensor */
long myTime;                /* this variable is used for millis function */

/************************************* class objects ************************************/
DHTesp dht;
ModbusIP modbus;

/********************************** initialization code *********************************/
void setup() {
  #ifdef ESP8266
    Serial.begin(74880);
  #else
    Serial.begin(115200);
  #endif

  WiFiManagerSetup();

  dht.setup(DHTpin, DHTesp::DHT11);   /* for DHT11 Connect data pin to GPIO 14 */

  /* configure the 4 pins for coils as outputs and send 0 to them */
  pinMode(coilPin0, OUTPUT);
  pinMode(coilPin1, OUTPUT);
  pinMode(coilPin2, OUTPUT);
  pinMode(coilPin3, OUTPUT);
  digitalWrite(coilPin0, LOW);
  digitalWrite(coilPin1, LOW);
  digitalWrite(coilPin2, LOW);
  digitalWrite(coilPin3, LOW);

  /* set the ESP8266 as modbus server (slave) and add 2 registers for temperature
   * and humidity and 4 registers for the coil */
  modbus.slave();
  modbus.addHreg(DHT_HREG, temperature);
  modbus.addHreg(DHT_HREG+1, humidity);
  modbus.addCoil(COIL_REG);
  modbus.addCoil(COIL_REG+1);
  modbus.addCoil(COIL_REG+2);
  modbus.addCoil(COIL_REG+3);

  myTime = millis();    /* set my time now */
}
 
/*********************************** application code ***********************************/
void loop() {
  if ((millis()-myTime) > 1000)   /* loop for each 1 second */
  {
    myTime = millis();    /* update the new time */

    /* Read the sensor data - temperature and humidity */
    temperature = dht.getTemperature();
    humidity = dht.getHumidity();
    
    if (temperature != 65535 && humidity != 65535) /* @todo: this is a bug in the code */
    {
      /* print the temperature and humidity values on the serial window */
      Serial.print("Temperature is: ");
      Serial.print(temperature);
      Serial.println(" degrees C");
      Serial.print("Humidity is: ");
      Serial.println(humidity);
      Serial.println();
      
      /* save the values in the modbus registers */
      modbus.Hreg(DHT_HREG, temperature);
      modbus.Hreg(DHT_HREG+1, humidity);
    }
  }

  /* read the coils values from the client modbus registers and write them */
  digitalWrite(coilPin0, modbus.Coil(COIL_REG));
  //Serial.println(modbus.Coil(COIL_REG));
  digitalWrite(coilPin1, modbus.Coil(COIL_REG+1));
  //Serial.println(modbus.Coil(COIL_REG+1));
  digitalWrite(coilPin2, modbus.Coil(COIL_REG+2));
  //Serial.println(modbus.Coil(COIL_REG+2));
  digitalWrite(coilPin3, modbus.Coil(COIL_REG+3));
  //Serial.println(modbus.Coil(COIL_REG+3));
  //Serial.println();
  //Serial.println();
  //Serial.println();
  
  modbus.task();      /* Call once inside loop() - all magic here */
  //delay(10);
}

/************************************** functions ***************************************/
/**
* This function makes the ESP8266 as a Soft WiFi Access Point.
* And you can connect to it using a WiFi station on the IP address printed on the serial
* to configure it as a station that accesses another access point
*/
void WiFiManagerSetup() {
  /* Local intialization. Once its business is done, there is no need to keep it around */
  WiFiManager wifiManager;

  /* set config save notify callback */
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  /* fetches ssid and pass and tries to connect
   * if it does not connect it starts an access point with the specified name
   * here  "AutoConnectAP"
   * and goes into a blocking loop awaiting configuration */
  if (!wifiManager.autoConnect(APName, APPassword)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    /* reset and try again, or maybe put it to deep sleep */
    ESP.reset();
    delay(5000);
  }

  /* if you get here you have connected to the WiFi */
  Serial.println("connected...successfully :)");
}

/**
* callback notifying us of the need to save config
*/
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
