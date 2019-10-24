/*
  Modbus-Arduino Example - Test Holding Register (Modbus IP ESP8266)
  Configure Holding Register (offset 100) with initial value 0xABCD
  You can get or set this holding register
  Original library
  Copyright by André Sarmento Barbosa
  http://github.com/andresarmento/modbus-arduino

  Current version
  (c)2017 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*/

#ifdef ESP8266
 #include <ESP8266WiFi.h>
#else //ESP32
 #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

#include "DHTesp.h"

#define DHTpin 14    //D5 of NodeMCU is GPIO14

DHTesp dht;

// Modbus Registers Offsets
const int TEST_HREG = 100;
const int COIL_REG = 200;
const int coilPin0 = D0; //GPIO0
const int coilPin1 = D1; //GPIO1
const int coilPin2 = D2; //GPIO2
//const int coilPin3 = D6; //GPIO3


//ModbusIP object
ModbusIP mb;

uint16_t temperature = 0;
uint16_t humidity = 0;
long mytime;

//int value1 = 10;
//int value2 = 50;

void setup() {
 #ifdef ESP8266
  Serial.begin(74880);
 #else
  Serial.begin(115200);
 #endif

  dht.setup(DHTpin, DHTesp::DHT11); //for DHT11 Connect DHT sensor to GPIO 17

  WiFi.begin("Focus", "Focus@Pro");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mb.slave();
  mb.addHreg(TEST_HREG, temperature);
  mb.addHreg(TEST_HREG+1, humidity);
  //mb.addHreg(TEST_HREG, value1++);
  //mb.addHreg(TEST_HREG+1, value2++);
  pinMode(coilPin0, OUTPUT);
  pinMode(coilPin1, OUTPUT);
  pinMode(coilPin2, OUTPUT);
  //pinMode(coilPin3, OUTPUT);
  digitalWrite(coilPin0, LOW);
  digitalWrite(coilPin1, LOW);
  digitalWrite(coilPin2, LOW);
  //digitalWrite(coilPin3, LOW);
  mb.addCoil(COIL_REG);
  mb.addCoil(COIL_REG+1);
  mb.addCoil(COIL_REG+2);
  //mb.addCoil(COIL_REG+3);
  
  mytime = millis();
}
 
void loop() {
    //delay(dht.getMinimumSamplingPeriod());
    if ((millis()-mytime) > 1000)
    {
       mytime = millis();
       // Read the sensor data - temperature and humidity millis
       temperature = dht.getTemperature();
       humidity = dht.getHumidity();
       
       if (temperature != 65535 && humidity != 65535) //this is a bug in the code
       {
         // print the temperature and humidity values on the serial window
         Serial.print("Temperature is: ");
         Serial.print(temperature);
         Serial.println(" degrees C");
         Serial.print("Humidity is: ");
         Serial.println(humidity);
         Serial.println();
         
         mb.Hreg(TEST_HREG, temperature);
         mb.Hreg(TEST_HREG+1, humidity);
         //mb.Hreg(TEST_HREG, value1++);
         //mb.Hreg(TEST_HREG+1, value2++);
//         digitalWrite(coilPin0, mb.Coil(COIL_REG));
//         digitalWrite(coilPin1, mb.Coil(COIL_REG+1));
//         digitalWrite(coilPin2, mb.Coil(COIL_REG+2));
         //digitalWrite(coilPin3, mb.Coil(COIL_REG+3));
       }
    }

    digitalWrite(coilPin0, mb.Coil(COIL_REG));
    Serial.println(mb.Coil(COIL_REG));
    digitalWrite(coilPin1, mb.Coil(COIL_REG+1));
    Serial.println(mb.Coil(COIL_REG+1));
    digitalWrite(coilPin2, mb.Coil(COIL_REG+2));
    Serial.println(mb.Coil(COIL_REG+2));
    Serial.println();
    Serial.println();
    Serial.println();
    
    //Call once inside loop() - all magic here
    mb.task();
    //delay(10);
}
