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

// Modbus Registers Offsets
const int TEST_HREG = 100;


//ModbusIP object
ModbusIP mb;
  
void setup() {
 #ifdef ESP8266
  Serial.begin(74880);
 #else
  Serial.begin(115200);
 #endif
 
  WiFi.begin("Focus", "Focus@Pro");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  int value1 = 0xABCD;
  int value2 = 0x1234;

  mb.slave();
  mb.addHreg(TEST_HREG, value1);
  mb.addHreg(TEST_HREG+1, value2);
}
 
void loop() {
   //Call once inside loop() - all magic here
   mb.task();
   delay(10);
}
