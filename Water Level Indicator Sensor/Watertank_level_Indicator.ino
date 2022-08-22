/* 
 * Smart Home Water Lavel Indicator by "VCCGROUND"
   
   Thanks to Ben from bruh automation. The following code is the edited version of the multisensor code provided by him. I have written this code to be used with HomeAssistant.
   Features: - Updates real time water level status to HA over the MQTT.
             - Provides Visual Indication with a common cathode LED.
             - Notifies about damaged sensor cable.
             - Can update the code Over the air (OTA). 
   
   You are absolutely free to use and distribute this Arduino code. To use this code you will need the following dependancies: 

  - Support for the ESP8266 boards. 
        - You can add it to the board manager by going to File -> Preference and pasting http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Managers URL field.
        - Next, download the ESP8266 dependancies by going to Tools -> Board -> Board Manager and searching for ESP8266 and installing it.
  
  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - Adafruit unified sensor
      - PubSubClient
      - ArduinoJSON
    
  Code last UPDATED 14 JAN 2021 by VCCGROUND. https://www.youtube.com/channel/UC1zmYQH0FRrppcR0GhUm_GQ
             
*/

/********************* ADD FOLLOWING LIBRARIES TO ARDUINO IDE ****************************/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

/******************* ADD YOUR WIFI and MQTT INFORMATION HERE ****************************/
#define wifi_ssid "your wifi ssid"
#define wifi_password "your wifi password"
#define mqtt_server "your mqtt server IP"
#define mqtt_user "your mqtt user name" 
#define mqtt_password "your mqtt password"
#define mqtt_port 1883
#define state_topic "watertank/level"

/**************************** FOR OVER THE AIR(OTA) UPDATES *****************************/
#define SENSORNAME "Watertank NodeMCU"
int OTAport = 8266;

/***************************** PIN DEFINATIONS *****************************************/
const int greenLED = D0;
const int redLED = D1;
const int quarter = D7;
const int half = D6;
const int threeFourth = D5;
const int full = D2;

int flag0=0;
int flag1=0; 
int flag2=0; 
int flag3=0; 
int flag4=0; 
int flag5=0;

String levelStatus;

char message_buff[100];
int calibrationTime = 0;
const int BUFFER_SIZE = 300;
#define MQTT_MAX_PACKET_SIZE 512

WiFiClient espClient;
PubSubClient client(espClient);

/********************************** START SETUP*****************************************/
void setup() {
  //Serial.begin(115200);               //Start serial monitor for debugging...
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(quarter, INPUT_PULLUP);
  pinMode(half, INPUT_PULLUP);
  pinMode(threeFourth, INPUT_PULLUP);
  pinMode(full, INPUT_PULLUP);

  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  delay(10);
  Serial.println("Starting Node named " + String(SENSORNAME));

  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  ArduinoOTA.onStart([]() {
  Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
  Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
  Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}

/********************************** SETUP WIFI *****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, LOW);
  delay(500);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
  delay(500);
  Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
}

/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["waterlevel"] = (String)levelStatus;
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(state_topic, buffer, true);
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we reconnect
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
  flag0=0; flag1=0; flag2=0; flag3=0; flag4=0; flag5=0;
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  reconnect();
  }
  client.loop();

  if ((digitalRead(quarter)==HIGH) && (digitalRead(half)==HIGH) && (digitalRead(threeFourth)==HIGH) && (digitalRead(full)==HIGH)){      //if water level is below 25%
      if (flag0 == 0) {
      Serial.println("Tank is empty.....");
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      levelStatus = "Empty";
      sendState();
      flag0=1; flag1=0; flag2=0; flag3=0; flag4=0; flag5=0;
  }}
  else if ((digitalRead(quarter)==LOW) && (digitalRead(half)==HIGH) && (digitalRead(threeFourth)==HIGH) && (digitalRead(full)==HIGH)){      //if water level is above 25%
      if (flag1 == 0) {
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      Serial.println("Tank is quarter.....");
      levelStatus = "25% Filled";
      sendState();
      flag0=0; flag1=1; flag2=0; flag3=0; flag4=0; flag5=0;
  }}
  else if ((digitalRead(quarter)==LOW) && (digitalRead(half)==LOW) && (digitalRead(threeFourth)==HIGH) && (digitalRead(full)==HIGH)){     //if water level is above 50%
      if (flag2 == 0) {
      digitalWrite(redLED, HIGH);
      delay(100);
      digitalWrite(greenLED, HIGH);
      Serial.println("Tank is 50%.....");
      levelStatus = "Half Filled";
      sendState();
      flag0=0; flag1=0; flag2=1; flag3=0; flag4=0; flag5=0;
  }}
  else if ((digitalRead(quarter)==LOW) && (digitalRead(half)==LOW) && (digitalRead(threeFourth)==LOW) && (digitalRead(full)==HIGH)){     //if water level is above 75%
      if (flag3 == 0) {
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);
      Serial.println("Tank is 75%.....");
      levelStatus = "75% Filled";
      sendState();
      flag0=0; flag1=0; flag2=0; flag3=1; flag4=0; flag5=0;
  }}
  else if ((digitalRead(quarter)==LOW) && (digitalRead(half)==LOW) && (digitalRead(threeFourth)==LOW) && (digitalRead(full)==LOW)){     //if water tank is Full
      if (flag4 == 0) {
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, LOW);
      Serial.println("Tank is full.....");
      levelStatus = "Full";
      sendState();
      //client.publish(tankState, "Full");  //publish the current state to MQTT
      flag0=0; flag1=0; flag2=0; flag3=0; flag4=1; flag5=0;
  }}
  else { Serial.println("Error.....");
      if (flag5 == 0) {
      levelStatus = "Sensor Cable Error...";
      sendState();
      flag0=0; flag1=0; flag2=0; flag3=0; flag4=0; flag5=1; } 
      else {
      digitalWrite(greenLED, HIGH);
      digitalWrite(redLED, LOW);
      delay(1000);
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, HIGH);
      delay(1000);
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);
  }}  delay(2000);
}
/**************************************** END OF PROGRAM ************************/  


