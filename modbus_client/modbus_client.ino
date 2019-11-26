/****************************************************************************************
  FileName    : modbus_client.ino
  Author      : The-DIY-life co. - peter.magdy@the-diy-life.co
  Description : This application makes an ESP8266 module as a modbus client that gives
                instructions to a server (slave) modbus device.
                This device is also connected to ubidots broker through MQTT connection.
                It reads values of temperature and humidity from a modbus server, then
                publishes these values to ubidots. And it can control 4 coils on a 
                modbus server by subscribing to 4 switches from ubidots, then sending
                this values to the server.
*****************************************************************************************/

/*************************************** libraries **************************************/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>                   /* library to use filesystem on ESP8266 */
#include <EEPROM.h>
#include <UbidotsESPMQTT.h>       /* MQTT library for ubidots broker */
#include <ModbusIP_ESP8266.h>     /* modbus library */
#include <ESP8266httpUpdate.h>    /* library to upload code Over The Air */

/* WiFi Manager libraries */
#include <DNSServer.h>
#include <WiFiManager.h>          /* https://github.com/tzapu/WiFiManager */
#include <ArduinoJson.h>          /* https://github.com/bblanchon/ArduinoJson */

/******************************** preprocessor directives *******************************/
#define TOKEN  ""  /* your initial Ubidots TOKEN */

/*********************************** global variables ***********************************/
char macAddr[18];
/* Set the host to the esp8266 file system */
const char* host = "esp2866fs";
/* Wifi managerr access point name and password */
char* APName = "AutoConnectAP";
char* APPassword = "password";
char* SSID;
long myTime;              /* this variable is used for millis function */
int upload = 0;           /* a flag used to upload a code from the server Over The Air */
uint address = 0;         /* the offset address of the EEPROM to save data in it */
/* this structure is used to save the user input configuration values */
struct {
  char token[40] = "";    /* for ubidots user TOKEN*/
  int interval = 0;       /* period between sending data to ubidots */
  unsigned int enable;    /* let apply these changes */
} configuration;
uint16_t temperature = 0; /* the temperature value received from modebus server (slave) */
uint16_t humidity = 0;    /* the humidity value received from modebus server (slave) */
/* the modbus offset register for temperature and humidity of the DHT11 sensor */
const int DHT_HREG = 100;
/* the modbus offset register for the 4 coils */
const int COIL_REG = 200;
bool shouldSaveConfig = false;    /* flag for saving data */
long second = 0;          /* used to delay 1 second */

/************************************* class objects ************************************/
ESP8266WebServer espServer(80);
Ubidots mqtt(TOKEN);
IPAddress remote(10, 2, 101, 108);  /* put here the IP Address of Modbus Slave device */
ModbusIP modbus;

/********************************** initialization code *********************************/
void setup(){
  /* Sets the broker properly for the business account */
  //mqtt.ubidotsSetBroker("test.mosquitto.org");
  /* you must set this broker if your account is a business account to avoid problems */
  mqtt.ubidotsSetBroker("business.api.ubidots.com");
  /* Pass a true or false bool value to activate debug messages */
  mqtt.setDebug(true);

  Serial.begin(115200);

  WiFiManagerSetup();

  /* commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
   * this step actually loads the content (512 bytes) of flash into
   * a 512-byte-array cache in RAM */
  EEPROM.begin(512);
  /* read bytes (i.e. sizeof(configuration) from "EEPROM"),
   * in reality, reads from byte-array cache
   * cast bytes into structure called configuration */
  EEPROM.get(address, configuration);
  Serial.println("Values are: " + String(configuration.token) + ", " + String(configuration.interval));
  
  /* Start flash file system */
  SPIFFS.begin();

  /* this is a local block to get rid of it after it finishes */
  {
    /* Open the direction */
    Dir dir = SPIFFS.openDir("/");
    /* Get the files names and sizes */
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, Size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
  }
    
  /* Start mdns for the file system */
  MDNS.begin(host);
  Serial.print("Open http://");
  Serial.print(host);
  Serial.println(".local to see the home page");

  /* Configuring the web server */
  /* in case the route is not found, send error 404 */
  espServer.onNotFound([]() {
    if (!handleFileRead(espServer.uri())) {
      espServer.send(404, "text/plain", "FileNotFound");
    }
  });
  /* in case the route is (/settings), call the function setConfiguration() to get new
   * configurations from the user */
  espServer.on("/settings", HTTP_POST, setConfiguration);
  /* in case the route is (/getSettings), call the function getLastConfig() to get the
   * saved configurations from the EEPROM */
  espServer.on("/getSettings", HTTP_GET, getLastConfig);
  /* in case the route is (/OTA), call the function uploadCode() to upload another
   * code saved on a server to the ESP8266 using internet (Over The Air) */
  espServer.on("/OTA", HTTP_GET, uploadCode);

  /* set the ESP8266 as modbus client (master) to read values of temperature
   * and humidity and set the new values for the coils */
  modbus.master();

  /* get the 6 parts of the MAC address and save each,
   * then convert it to be one string */
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(macAddr, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("macAddr: ");
  Serial.print(macAddr);

  /* initialize the token and client name and begin MQTT connection */
  mqtt.changeToken(configuration.token);
  mqtt.changeClientID(macAddr);
  mqtt.begin(callback);
  
  espServer.begin();          /* start the server */
  Serial.println("HTTP server started");
  
  myTime = millis();    /* set my time now */
  second = millis();

  /* check if the token is empty or not */
  if (strlen(configuration.token) > 0 && strlen(configuration.token) <= 40)
  {
    /* subscribe to all topics avaliable on the broker in this connection */
    mqtt.ubidotsSubscribe(macAddr, "+");
  }
}

/*********************************** application code ***********************************/
void loop(){
  espServer.handleClient();   /* start the requested route */

  MDNS.update();

  /* run this block of code if the interval is valid */
  if ((millis() - myTime) > (configuration.interval * 1000) && configuration.interval >= 30) {
    myTime = millis();    /* update the new time */

    Serial.println("timeInterval is: " + String(configuration.interval));
    Serial.println("myTime is: " + String(millis() - myTime));
  
    if (configuration.enable == 1) {
      Serial.println("enable is: true" );

      /* Check if connection to Modbus Slave is established */
      if (!modbus.isConnected(remote)) {
        modbus.connect(remote);    /* Try to connect if no connection */
      }
      /* Read holding registers from Modbus Slave if the connection is stablished*/
      if (modbus.isConnected(remote)) {
        modbus.readHreg(remote, DHT_HREG, &temperature);
        modbus.readHreg(remote, DHT_HREG+1, &humidity);
      }
      modbus.task();               /* Common local Modbus task */
      
      /* Check if MQTT connection is established */
      if(!mqtt.connected()){
        mqtt.reconnect();          /* try to connect */
      }
      /* send data to ubidots if the connection is stablished */
      if(mqtt.connected()){
        mqtt.add("temperature", temperature);
        mqtt.add("humidity", humidity);
        /* publish the topics to the device with the ID of MAC address of the publisher */
        mqtt.ubidotsPublish(macAddr);
      }
    }
  }

  modbus.task();               /* Common local Modbus task */

  /* check if there is a token or not */
  if (strlen(configuration.token) > 0 && strlen(configuration.token) <= 40)
  {
    if ((millis() - second) > 1000)     /* delay 1 second */
    {
      second = millis();
      /* check the MQTT connection and subscribe to all topics avaliable on 
      * the broker in this connection */
      if(!mqtt.connected()){
        mqtt.reconnect();
        mqtt.ubidotsSubscribe(macAddr, "+");
      }
      mqtt.loop();
    }
  }
  /* This block upload the up to date code over the air if the flag is raised.
   * You can upgrade your code by requesting the route (/OTA) from any browser.
   * When you request the route you update the files of the FS (spiffs.img) and 
   * the binary file of the code (modbus_client.bin) */
  if (upload == 1)
  {
    WiFiClient client;
    t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(client, "http://www.the-diy-life.co/spiffs.img");
    if (ret == HTTP_UPDATE_OK) {
      Serial.println("Update sketch...");
      ret = ESPhttpUpdate.update(client, "http://www.the-diy-life.co/modbus_client.bin");

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

/**
* format bytes size to known bytes units.
* @param bytes is the size in bytes to be converted to a string
* @return string of the size
*/
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

/**
* check for the file extension to get the file type.
* @param filename is the name of the file with the extension
* @return converted string from .exe to the route
*/
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

/**
* Read the file
* @param path is the file path to be read
* @return true if the file is read correctly and false otherwise
*/
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

/**
* This is a call back function that is called when a device publish on a topic
* that is subscribed by this device.
* @param topic that is subscribed to
* @param payload the contents of the message that is received on this topic
* @param length of the payload
*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");

  /* check if the data that is received from a publisher is 0 or 1
   * Note: the received data are strings, so we must convert it to
   *       integer or (-48) from it */
  if ((*payload-48)==0 || (*payload-48)==1)
  {
    char tempBuff[100];     /* buffer array to check all the subscribed topics */
    int i = 0;
    Serial.println();
    for(i = 0 ; i < 4 ; i++){
      memset(tempBuff,0,100);
      snprintf(tempBuff,100,"/v1.6/devices/%s/switch%d/lv", macAddr, i); 
      if (!strcmp(topic, tempBuff)){
        //Serial.println(strcmp(topic, tempBuff));
        /* check the connection of the modbus */
        if (!modbus.isConnected(remote)){
          modbus.connect(remote);
        }
        modbus.task();
        modbus.writeCoil(remote, COIL_REG + i, (*payload-48));
      }
    }  
  }

  modbus.task();
}

/**
* This function is called when the route (/settings) is requested to check the validity
* of user inputs and save them in the EEPROM
*/
void setConfiguration() {
  Serial.println("In response arg is: " );
  if (espServer.hasArg("submit")) {
    Serial.print("submit arg:\t");
    Serial.println(espServer.arg("submit"));
  }
  
  /* Check if the token not null and not more than 36 char. */
  if (espServer.hasArg("token") && (espServer.arg("token").length() > 0)) {
    if ((espServer.arg("token").length() > 36)) {
      return espServer.send(500, "text/plain", "BAD ARGS");
    }
    Serial.print("User entered:\t");
    Serial.println(espServer.arg("token"));
    espServer.arg("token").toCharArray(configuration.token, 40);
  }
  else {
    return espServer.send(500, "text/plain", "BAD ARGS");
  }
  
  /* Check for interval */
  if (espServer.hasArg("interval") && (espServer.arg("interval").length() > 0)) {
    Serial.print("User entered:\t");
    configuration.interval =  espServer.arg("interval").toInt();
    Serial.println(configuration.interval);
  }
  else {
    return espServer.send(500, "text/plain", "BAD ARGS");
  }
  
  /* Check for enable status, no check for length here because if it > 0 it will
   * be true all the time */
  if (espServer.hasArg("checky")) {
    Serial.print("User cecked:\t");
    Serial.println(espServer.arg("checky"));
    if (espServer.arg("checky") == "0") {
      configuration.enable = 0;
      Serial.print("User cecked false: " + configuration.enable);
    }
    else if (espServer.arg("checky") == "1") {
      Serial.print("User cecked true: " + configuration.enable);
      configuration.enable = 1;
      Serial.print("User cecked:\t");
    }
  }
  else {
    configuration.enable = 0;
    Serial.print("User cecked false: " + configuration.enable);
  }
  
  /* commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
   * this step actually loads the content (512 bytes) of flash into
   * a 512-byte-array cache in RAM */
  EEPROM.begin(512);
  /* replace values in byte-array cache with modified configuration
   * no changes made to flash, all in local byte-array cache */
  EEPROM.put(address, configuration);
  /* actually write the content of byte-array cache to
   * hardware flash.  flash write occurs if and only if one or more byte
   * in byte-array cache has been changed, but if so, ALL 512 bytes are
   * written to flash */
  EEPROM.commit();
  
  Serial.println("In Response Values are: " + String(configuration.token) + "," + String(configuration.interval) + "," + String(configuration.enable));
  //delay(500);
  handleFileRead("/success.htm");

  mqtt.changeToken(configuration.token);
  mqtt.disconnect();
  mqtt.reconnect();
}

/**
* This function is called when the route (/getSettings) is requested to get the last
* saved configuration values from EEPROM and show them on the web page
*/
void getLastConfig() {
  Serial.println("\nin last configuration values");

  String json = "{";
  json += "\"token\": \"" + String(configuration.token) + "\" ,\"interval\":" + configuration.interval + ",\"enableD\":" + configuration.enable;
  json += "}";
  /* send the json */
  espServer.send(200, "text/json", json);

  Serial.println("\nending configuration values");
}

/**
* This function is called when the route (/OTA) is requested to upload the code
* to the ESP8266 over the air by setting flag upload
*/
void uploadCode()
{
  upload = 1;
  espServer.send(200, "text/plain", "ok");
}
