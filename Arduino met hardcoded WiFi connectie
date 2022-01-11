#include <ArduinoJson.h> //json version 5.13.5
#include <WiFi.h>
#include <PubSubClient.h>
 
const char* ssid = "insert SSID"; //”androidAP”
const char* password =  "Insert SSID password"; //”HGjsi1”
const char* mqttServer = "Insert IP address of webserver or hostname"; //”192.168.1.3”
const int mqttPort = 1883;
const char* mqttUser = "Insert username that has to match in the mqtthandler.js code of NodeJS. In github code mqttUser is pelvic."; //”pelvic”
const char* mqttPassword = " Insert password that has to match in the mqtthandler.js code of NodeJS. In github code mqttpassword is chair "; //”chair”
float sensor1, sensor2, sensor3, sensor4;

WiFiClient espClient;
PubSubClient client(espClient);
 void setup() {
   Serial.begin(115200);
  Serial.println();
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
    //if (client.connect("ESP32Client" )) {
 
      Serial.println("connected");
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
}
 
void loop() {
  sensor1 = touchRead(T0); //pin 2
  sensor2 = touchRead(T2); //pin 4
  sensor3 = touchRead(T4); //pin 13
  sensor4 = touchRead(T7); //pin 27
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
 
  JSONencoder["device"] = "ESP32";
  JSONencoder["sensorType"] = "sensors";
  JsonArray& values = JSONencoder.createNestedArray("values");

  values.add(sensor1);
  values.add(sensor2);
  values.add(sensor3);
  values.add(sensor4);

  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
 
  if (client.publish("esp/test", JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
      }
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
    //if (client.connect("ESP32Client" )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
    }
  }
  client.loop();
  Serial.println("-------------");
  delay(10000);
}
