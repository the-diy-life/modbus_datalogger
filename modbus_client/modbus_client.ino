#include "UbidotsESPMQTT.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <EEPROM.h>
//WiFi Manager libraries
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
//NTP libraries
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266httpUpdate.h>
//modbus library
#include <ModbusIP_ESP8266.h>

char macAddr[18];
#define TOKEN  "BBFF-p51mIO3hV0xz5M8mufeAHatCyIWZDl"  // Put here your Ubidots TOKEN
//BBFF-p51mIO3hV0xz5M8mufeAHatCyIWZDl

// Set the host to the esp8266 file system
const char* host = "esp2866fs";

//the time difference with UTC standard in seconds
const long utcOffsetInSeconds = 0; //2*3600;

//#define WIFISSID "Focus" // Put here your Wi-Fi SSID
//#define PASSWORD "Focus@Pro" // Put here your Wi-Fi password

//  Wifi managerr access point name and password
char* APName = "AutoConnectAP";
char* APPassword = "password";
char* SSID;

/*
#include "DHTesp.h"

#define DHTpin 14    //D5 of NodeMCU is GPIO14

DHTesp dht;
*/
ESP8266WebServer espServer(80);

Ubidots mqtt(TOKEN);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


long mytime;
int upload = 0;
uint addr = 0;
struct {
  char val[40] = "";
  int interval = 0;
  unsigned int enable;
} data;
uint16_t temperature = 0;
uint16_t humidity = 0;

const int REG = 100;               // Modbus Hreg Offset
IPAddress remote(10, 2, 101, 103);  // Address of Modbus Slave device

const uint16_t COIL_REG = 200;
bool coil[4] = {0};
bool Switch = 1;
int ledPin = D0;
char ubidotsMAC[30];


ModbusIP mb;  //ModbusIP object

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//format bytes size to known bytes units.
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

// check for the file extension to get the file type.
String getContentType(String filename) {
  if (espServer.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

// Read the file
bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  // If the path is the root add the index.htm to it.
  if (path.endsWith("/")) {
    Serial.println("path ends With / " + path);
    path += "settings.htm";
  }
  // call the getContentType method and set the result to string varible.
  String contentType = getContentType(path);
  // Compress the file
  String pathWithGz = path + ".gz";

  //Check if the file exist on the flash file system zip or unzip.
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    // Check again if the ziped file exist. I don't understand why and why zip it again!?
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    // Open the file in read mode
    File file = SPIFFS.open(path, "r");
    espServer.streamFile(file, contentType);
    // close the file.
    file.close();
    return true;
  }
  return false;
}

void WiFiManagerSetup() {
  /*
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    ReadConfig();
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  */

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(APName, APPassword)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...successfully :)");

  /*
    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["http_server"] = http_server;
      json["http_port"] = http_port;
      json["username"] = username;
      json["userpassword"] = userpassword;

      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }

      json.printTo(Serial);
      json.printTo(configFile);
      configFile.close();
      //end save
      ReadConfig();
    }
  */
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  if ((*payload-48)==0 || (*payload-48)==1)
  {
    char tempBuff[100];
    int i = 0;
    digitalWrite(ledPin, (*payload-48));
    Serial.println();
    for(i = 0 ; i <3 ; i++){
      memset(tempBuff,0,100);
      snprintf(tempBuff,100,"/v1.6/devices/%s/switch%d/lv", macAddr, i); 
      if (!strcmp(topic, tempBuff)){
        Serial.println(strcmp(topic, tempBuff));
        if (!mb.isConnected(remote)){
          mb.connect(remote);
        }
        mb.task();
        mb.writeCoil(remote, COIL_REG + i, (*payload-48));
      }
    }  
//    if (!strcmp(topic, "/v1.6/devices/modbus/switch0/lv")){
//      Serial.println(strcmp(topic, "/v1.6/devices/modbus/switch0/lv"));
//      mb.writeCoil(remote, COIL_REG, (*payload-48));
//    }
//    else if (!strcmp(topic, "/v1.6/devices/modbus/switch1/lv")){
//      Serial.println(strcmp(topic, "/v1.6/devices/modbus/switch1/lv"));
//      mb.writeCoil(remote, COIL_REG+1, (*payload-48));
//    }
//    else if (!strcmp(topic, "/v1.6/devices/modbus/switch2/lv")){
//      Serial.println(strcmp(topic, "/v1.6/devices/modbus/switch2/lv"));
//      mb.writeCoil(remote, COIL_REG+2, (*payload-48));
//    }
  }
  mb.task();
}


void setup(){
    mqtt.ubidotsSetBroker("business.api.ubidots.com"); // Sets the broker properly for the business account
    mqtt.setDebug(true); // Pass a true or false bool value to activate debug messages
    Serial.begin(115200);
    //dht.setup(DHTpin, DHTesp::DHT11); //for DHT11 Connect DHT sensor to GPIO 17
    //mqtt.wifiConnection(WIFISSID, PASSWORD);
    //mqtt.setDebug(true); // Uncomment this line to set DEBUG on

    // commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
    // this step actually loads the content (512 bytes) of flash into
    // a 512-byte-array cache in RAM
    EEPROM.begin(512);
    // read bytes (i.e. sizeof(data) from "EEPROM"),
    // in reality, reads from byte-array cache
    // cast bytes into structure called data
    EEPROM.get(addr, data);
    Serial.println("Values are: " + String(data.val) + "," + String(data.interval));
    
    //Start flash file system
    SPIFFS.begin();
    {
      // Open the direction
      Dir dir = SPIFFS.openDir("/");

      // Get the files names and sizes
      while (dir.next()) {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("FS File: %s, Size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      }
    }
    
    WiFiManagerSetup();
    mytime = millis();
    
    // Start mdns for the file system
    MDNS.begin(host);
    Serial.print("Open http://");
    Serial.print(host);
    Serial.println(".local to see the home page");

    timeClient.begin();

    mb.master();
    
    espServer.onNotFound([]() {
      if (!handleFileRead(espServer.uri())) {
        espServer.send(404, "text/plain", "FileNotFound");
      }
    });
    //Configuring the web server
    espServer.on("/settings", HTTP_POST, response);
    espServer.on("/getSettings", HTTP_GET, bindValues);
    espServer.on("/OTA", HTTP_GET, uploadCode);

    //mqtt.wifiConnection(WIFISSID, PASSWORD);
    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(macAddr, "%2x%2x%2x%2x%2x%2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println("macAddr");
    Serial.println(macAddr);
    //mqtt.wifiConnection("", "");
    //mqtt.initialize(data.val,macAddr);
    mqtt.changeToken(data.val);
    mqtt.changeClientID(macAddr);
    mqtt.begin(callback);
    
    // start the server
    espServer.begin();
    Serial.println("HTTP2 server started");
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    mqtt.ubidotsSubscribe(macAddr, "+");
    //mqtt.ubidotsSubscribe("modbus", "+"); //Insert the dataSource and Variable's Labels
    //mqtt.ubidotsSubscribe("modbus", "switch0"); //Insert the dataSource and Variable's Labels
    //mqtt.ubidotsSubscribe("modbus", "switch1"); //Insert the dataSource and Variable's Labels
    //mqtt.ubidotsSubscribe("modbus", "switch2"); //Insert the dataSource and Variable's Labels
}

void loop(){
//    espServer.handleClient();
////    MDNS.update();
//
////    timeClient.update();

    // make the request if the interval is valid
    if ((millis() - mytime) > (data.interval * 1000) && data.interval >= 30) {
      mytime = millis();
      Serial.println("timeInterval is: " + String(data.interval));
      Serial.println("mytime is: " + String(millis() - mytime));
   
      if (data.enable == 1) {
        Serial.println("enable is: true" );
        //delay(dht.getMinimumSamplingPeriod());
        //float value1 = dht.getTemperature();
        //float value2 = dht.getHumidity();
        //mqtt.add("temperature", value1);
        //mqtt.add("humidity", value2);

//        unsigned long timestamp = timeClient.getEpochTime();
//        Serial.printf("\ntimestamp = %ld\n", timestamp);
        
        //mqtt.add("5da32863e694aa4ba82dcd35", value1, timestamp);
        //mqtt.add("5da33830bbddbd73a86702f1", value2, timestamp);
        //mqtt.add("humidity2", value2, timestamp);
        //mqtt.sendAll(true);
        //delay(5000);
        //upload++;

        if (mb.isConnected(remote)) {   // Check if connection to Modbus Slave is established
          mb.readHreg(remote, REG, &temperature);  // Initiate Read Coil from Modbus Slave
          mb.readHreg(remote, REG+1, &humidity);  // Initiate Read Coil from Modbus Slave
//          mb.writeCoil(remote, COIL_REG, coil[0]);
//          mb.writeCoil(remote, COIL_REG+1, coil[1]);
//          mb.writeCoil(remote, COIL_REG+2, coil[2]);
//          //mb.writeCoil(remote, COIL_REG+3, coil[3]);
//          coil[0] ^= 1;
//          coil[1] ^= 1;
//          coil[2] ^= 1;
//          //coil[4] ^= 1;
        } else {
          mb.connect(remote);           // Try to connect if no connection
        }
        mb.task();                      // Common local Modbus task        
        
        if(!mqtt.connected()){
          mqtt.reconnect();
        }
        if(mqtt.connected()){
          mqtt.add("temperature", temperature);
          mqtt.add("humidity", humidity);          
          //mqtt.ubidotsPublish("modbus");
          mqtt.ubidotsPublish(macAddr);
        }
      }
    }

    mb.task();                      // Common local Modbus task
    
    if(!mqtt.connected()){
      mqtt.reconnect();
      mqtt.ubidotsSubscribe(macAddr, "+");
      //mqtt.ubidotsSubscribe("modbus", "+");
      //mqtt.ubidotsSubscribe("modbus", "switch0");
      //mqtt.ubidotsSubscribe("modbus", "switch1");
      //mqtt.ubidotsSubscribe("modbus", "switch2");
    }
    mqtt.loop();

    if (upload == 2)
    {
      //ESPhttpUpdate.update("www.the-diy-life.co", 80, "/blinky.bin");
      /*
      t_httpUpdate_return ret = ESPhttpUpdate.update("www.the-diy-life.co", 80, "/blinky.bin");
      switch(ret) {
          case HTTP_UPDATE_FAILED:
              Serial.println("[update] Update failed.");
              break;
          case HTTP_UPDATE_NO_UPDATES:
              Serial.println("[update] Update no Update.");
              break;
          case HTTP_UPDATE_OK:
              Serial.println("[update] Update ok."); // may not called we reboot the ESP
              break;
      }
      */
      WiFiClient client;
      t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(client, "http://www.the-diy-life.co/spiffs.img");
      if (ret == HTTP_UPDATE_OK) {
        Serial.println("Update sketch...");
        ret = ESPhttpUpdate.update(client, "http://www.the-diy-life.co/ThingSpeakDataLogger.bin");
  
        switch (ret) {
          case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;
  
          case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
  
          case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
        }
      }
      upload = 0;
    }
}


void response() {
    Serial.println("In response arg is: " );
    if (espServer.hasArg("submit")) {
      Serial.print("submit arg:\t");
      Serial.println(espServer.arg("submit"));
    }
    
    // Check if the token not null and not more than 35 char.
    if (espServer.hasArg("token") && (espServer.arg("token").length() > 0)) {
      if ((espServer.arg("token").length() > 36)) {
        return espServer.send(500, "text/plain", "BAD ARGS");
      }
      Serial.print("User entered:\t");
      Serial.println(espServer.arg("token"));
      espServer.arg("token").toCharArray(data.val, 40);
      mqtt.changeToken(data.val);
    }
    else {
      return espServer.send(500, "text/plain", "BAD ARGS");
    }
    
    // Check for interval
    if (espServer.hasArg("interval") && (espServer.arg("interval").length() > 0)) {
      Serial.print("User entered:\t");
      data.interval =  espServer.arg("interval").toInt();
      Serial.println(data.interval);
    }
    else {
      return espServer.send(500, "text/plain", "BAD ARGS");
    }
    
    // Check for enable status, no check for length here because if it > 0 it will be true all the time.
    if (espServer.hasArg("checky")) {
      Serial.print("User cecked:\t");
      Serial.println(espServer.arg("checky"));
      if (espServer.arg("checky") == "0") {
        data.enable = 0;
        Serial.print("User cecked false: " + data.enable);
      }
      else if (espServer.arg("checky") == "1") {
        Serial.print("User cecked true: " + data.enable);
        data.enable = 1;
        Serial.print("User cecked:\t");
      }
    }
    else {
      data.enable = 0;
      Serial.print("User cecked false: " + data.enable);
    }
    
    // commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
    // this step actually loads the content (512 bytes) of flash into
    // a 512-byte-array cache in RAM
    EEPROM.begin(512);
  
    // replace values in byte-array cache with modified data
    // no changes made to flash, all in local byte-array cache
    EEPROM.put(addr, data);
  
    // actually write the content of byte-array cache to
    // hardware flash.  flash write occurs if and only if one or more byte
    // in byte-array cache has been changed, but if so, ALL 512 bytes are
    // written to flash
    EEPROM.commit();
    
    Serial.println("In Response Values are: " + String(data.val) + "," + String(data.interval) + "," + String(data.enable));
    //delay(500);
    handleFileRead("/success.htm");

    mqtt.disconnect();
    mqtt.reconnect();
}

void bindValues() {
  Serial.println("\nin bindValues");

  String json = "{";
  json += "\"token\": \"" + String(data.val) + "\" ,\"interval\":" + data.interval + ",\"enableD\":" + data.enable;
  json += "}";
  // send the json
  espServer.send(200, "text/json", json);

  Serial.println("\nending bindValues");
}

void uploadCode()
{
  upload = 2;
  espServer.send(200, "text/plain", "ok");
}
