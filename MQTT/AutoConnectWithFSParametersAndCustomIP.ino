#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
char mqtt_server[40]="freemqtt.gerfed.ru";
char mqtt_port[6] = "1883";
char mqtt_login[40];
char mqtt_password[40];
// Topic's
char humidity_topic[100];
char temperature_celsius_topic[100];
char temperature_fahrenheit_topic[100];
char freemem_topic[100];
char timer_topic[100];
char carddetect_topic[100];


//default custom static IP
char static_ip[16];
char static_gw[16];
char static_sn[16];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () 
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //clean FS, for testing
 // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_login, json["mqtt_login"]);
          strcpy(mqtt_password, json["mqtt_password"]);          
          
        // Topic's
          strcpy(humidity_topic, json["humidity_topic"]);
          strcpy(temperature_celsius_topic, json["temperature_celsius_topic"]);        
          strcpy(temperature_fahrenheit_topic, json["temperature_fahrenheit_topic"]);               
          strcpy(freemem_topic, json["freemem_topic"]);               
          strcpy(timer_topic, json["timer_topic"]);               
          strcpy(carddetect_topic, json["carddetect_topic"]);


                 



          if(json["ip"]) {
            Serial.println("setting custom ip from config");
            //static_ip = json["ip"];
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            //strcat(static_ip, json["ip"]);
            //static_gw = json["gateway"];
            //static_sn = json["subnet"];
            Serial.println(static_ip);
/*            Serial.println("converting ip");
            IPAddress ip = ipFromCharArray(static_ip);
            Serial.println(ip);*/
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  Serial.println(static_ip);
  
  Serial.println(mqtt_server);
  Serial.println(mqtt_login);  
  Serial.println(mqtt_password);  

  Serial.println(humidity_topic);
  Serial.println(temperature_celsius_topic);  
  Serial.println(temperature_fahrenheit_topic);
  Serial.println(freemem_topic);
  Serial.println(timer_topic);    
  Serial.println(carddetect_topic);    

          

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_login("mqtt_login", "mqtt login", mqtt_login, 40);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "mqtt password", mqtt_password, 40);


  WiFiManagerParameter custom_humidity_topic("humidity_topic", "humidity topic", humidity_topic, 100);
  WiFiManagerParameter custom_temperature_celsius_topic("temperature_celsius_topic", "temperature celsius topic", temperature_celsius_topic, 100);
  WiFiManagerParameter custom_temperature_fahrenheit_topic("temperature_fahrenheit_topic", "temperature fahrenheit topic", temperature_fahrenheit_topic, 100);
  WiFiManagerParameter custom_freemem_topic("freemem_topic", "freemem topic", freemem_topic, 100);      
  WiFiManagerParameter custom_timer_topic("timer_topic", "timer topic", timer_topic, 100);
  WiFiManagerParameter custom_carddetect_topic("carddetect_topic", "carddetect_topic", carddetect_topic, 100);


  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_password);  


  wifiManager.addParameter(&custom_humidity_topic);  
  wifiManager.addParameter(&custom_temperature_celsius_topic);  
  wifiManager.addParameter(&custom_temperature_fahrenheit_topic);  
  wifiManager.addParameter(&custom_freemem_topic);  
  wifiManager.addParameter(&custom_timer_topic);  
  wifiManager.addParameter(&custom_carddetect_topic);  



  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("ConfigTemp", "12345678")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_login, custom_mqtt_login.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());


  strcpy(humidity_topic, custom_humidity_topic.getValue());
  strcpy(temperature_celsius_topic, custom_temperature_celsius_topic.getValue());
  strcpy(temperature_fahrenheit_topic, custom_temperature_fahrenheit_topic.getValue());
  strcpy(freemem_topic, custom_freemem_topic.getValue());
  strcpy(timer_topic, custom_timer_topic.getValue());
  strcpy(carddetect_topic, custom_carddetect_topic.getValue());          


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_login"] = mqtt_login;
    json["mqtt_password"] = mqtt_password;

    
    

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void loop() {
  // put your main code here, to run repeatedly:


}
