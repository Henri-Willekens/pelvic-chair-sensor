#include <ArduinoJson.h> //json version 5.13.5
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#define EXTERNAL_SWITCH_PIN 16
#include <AutoConnect.h>
#include <EEPROM.h>
 

const char* mqttServer = "Insert IP address or hostname of webserver"; //”192.168.1.3”
const int mqttPort = 1883;
const char* mqttUser = "pelvic";
const char* mqttPassword = "chair";
float sensor1, sensor2, sensor3, sensor4;



static const char AUX_CONFIGIP[] PROGMEM = R"(
{
  "title": "Config IP",
  "uri": "/configip",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<b>Module IP configuration</b>",
      "style": "color:steelblue"
    },
    {
      "name": "mac",
      "type": "ACText",
      "format": "MAC: %s",
      "posterior": "br"
    },
    {
      "name": "staip",
      "type": "ACInput",
      "label": "IP",
      "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
      "global": true
    },
    {
      "name": "gateway",
      "type": "ACInput",
      "label": "Gateway",
      "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
      "global": true
    },
    {
      "name": "netmask",
      "type": "ACInput",
      "label": "Netmask",
      "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
      "global": true
    },
    {
      "name": "dns1",
      "type": "ACInput",
      "label": "DNS",
      "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
      "global": true
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/restart"
    },
    {
      "name": "cancel",
      "type": "ACSubmit",
      "value": "Cancel",
      "uri": "/_ac"
    }
  ]
}
)";

static const char AUX_RESTART[] PROGMEM = R"(
{
  "title": "Config IP",
  "uri": "/restart",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "Settings",
      "style": "font-family:Arial;font-weight:bold;text-align:center;margin-bottom:10px;color:steelblue"
    },
    {
      "name": "staip",
      "type": "ACText",
      "format": "IP: %s",
      "posterior": "br",
      "global": true
    },
    {
      "name": "gateway",
      "type": "ACText",
      "format": "Gateway: %s",
      "posterior": "br",
      "global": true
    },
    {
      "name": "netmask",
      "type": "ACText",
      "format": "Netmask: %s",
      "posterior": "br",
      "global": true
    },
    {
      "name": "dns1",
      "type": "ACText",
      "format": "DNS1: %s",
      "posterior": "br",
      "global": true
    },
    {
      "name": "result",
      "type": "ACText",
      "posterior": "par"
    }
  ]
}
)";

WiFiClient espClient;
PubSubClient client(espClient);
AutoConnect portal;
AutoConnectConfig config;
AutoConnectAux    auxIPConfig;
AutoConnectAux    auxRestart;

// Pin assignment for an external configuration switch
uint8_t ConfigPin = EXTERNAL_SWITCH_PIN;
uint8_t ActiveLevel = LOW;

// EEPROM saving structure
typedef union {
  struct {
    uint32_t  ip;
    uint32_t  gateway;
    uint32_t  netmask;
    uint32_t  dns1;
  } ipconfig;
  uint8_t  ipraw[sizeof(uint32_t) * 4];
} IPCONFIG;

// Load IP configuration from EEPROM
void loadConfig(IPCONFIG* ipconfig) {
  EEPROM.begin(sizeof(IPCONFIG));
  int dp = 0;
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t c = 0; c < sizeof(uint32_t); c++)
      ipconfig->ipraw[c + i * sizeof(uint32_t)] = EEPROM.read(dp++);
  }
  EEPROM.end();

  // Unset value screening
  if (ipconfig->ipconfig.ip == 0xffffffffL)
    ipconfig->ipconfig.ip = 0U;
  if (ipconfig->ipconfig.gateway == 0xffffffffL)
    ipconfig->ipconfig.gateway = 0U;
  if (ipconfig->ipconfig.netmask == 0xffffffffL)
    ipconfig->ipconfig.netmask = 0U;
  if (ipconfig->ipconfig.dns1 == 0xffffffffL)
    ipconfig->ipconfig.dns1 = 0U;

  Serial.println("IP configuration loaded");
  Serial.printf("Sta IP :0x%08lx\n", ipconfig->ipconfig.ip);
  Serial.printf("Gateway:0x%08lx\n", ipconfig->ipconfig.gateway);
  Serial.printf("Netmask:0x%08lx\n", ipconfig->ipconfig.netmask);
  Serial.printf("DNS1   :0x%08lx\n", ipconfig->ipconfig.dns1);
}

// Save current IP configuration to EEPROM
void saveConfig(const IPCONFIG* ipconfig) {
  // EEPROM.begin will truncate the area to the size given by the argument.
  // The part overflowing from the specified size is filled with 0xff,
  // so if the argument value is too small, the credentials may be lost.
  EEPROM.begin(128);

  int dp = 0;
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t d = 0; d < sizeof(uint32_t); d++)
      EEPROM.write(dp++, ipconfig->ipraw[d + i * sizeof(uint32_t)]);
  }
  EEPROM.end();
  delay(100);
}

// Custom web page handler to set current configuration to the page
String getConfig(AutoConnectAux& aux, PageArgument& args) {
  IPCONFIG  ipconfig;
  //loadConfig(&ipconfig);

  // Fetch MAC address
  String  macAddress;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  for (uint8_t i = 0; i < 6; i++) {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    macAddress += buf;
    if (i < 5)
      macAddress += ':';
  }
  aux["mac"].value = macAddress;

  // Fetch each IP address configuration from EEPROM
  IPAddress staip = IPAddress(ipconfig.ipconfig.ip);
  IPAddress gateway = IPAddress(ipconfig.ipconfig.gateway);
  IPAddress netmask = IPAddress(ipconfig.ipconfig.netmask);
  IPAddress dns1 = IPAddress(ipconfig.ipconfig.dns1);

  // Echo back the IP settings
  aux["staip"].value = staip.toString();
  aux["gateway"].value = gateway.toString();
  aux["netmask"].value = netmask.toString();
  aux["dns1"].value = dns1.toString();

  return String();
}

// Convert IP address from AutoConnectInput string value
void getIPAddress(String ipString, uint32_t* ip) {
  IPAddress ipAddress;

  if (ipString.length())
    ipAddress.fromString(ipString);
  *ip = (uint32_t)ipAddress;
}

// Custom web page handler to save the configuration to AutoConnectConfig
String setConfig(AutoConnectAux& aux, PageArgument& args) {
  IPCONFIG  ipconfig;

  // Retrieve each IP address from AutoConnectInput field
  getIPAddress(aux["staip"].value, &ipconfig.ipconfig.ip);
  getIPAddress(aux["gateway"].value, &ipconfig.ipconfig.gateway);
  getIPAddress(aux["netmask"].value, &ipconfig.ipconfig.netmask);
  getIPAddress(aux["dns1"].value, &ipconfig.ipconfig.dns1);

  // Make a result message
  if (auxIPConfig.isValid()) {
    saveConfig(&ipconfig);
    aux["result"].value = "Reset by AutoConnect menu will restart with the above.";
  }
  else
    aux["result"].value = "Invalid IP address specified.";
  return String();
}

// Sense the external switch to enter the configuraton mode
bool senseSW(const uint8_t pin, const uint8_t activeLevel) {
  bool  sw = digitalRead(pin) == activeLevel;
  if (sw) {
    // Cut-off the chattering noise
    unsigned long tm = millis();
    while (digitalRead(pin) == activeLevel) {
      if (millis() - tm > 1000)
        break;
      delay(1);
    }
  }
  return sw;
}
 
void setup() {
 
  Serial.begin(115200);
  Serial.println();
  // Shift the credentials storage to reserve saving IPCONFIG
  config.boundaryOffset = sizeof(IPCONFIG);

  // Load current IP configuration
  IPCONFIG  ipconfig;
  //loadConfig(&ipconfig);

  // Configure pre-loaded static IPs
  config.staip = IPAddress(ipconfig.ipconfig.ip);
  config.staGateway = IPAddress(ipconfig.ipconfig.gateway);
  config.staNetmask = IPAddress(ipconfig.ipconfig.netmask);
  config.dns1 = IPAddress(ipconfig.ipconfig.dns1);
  portal.config(config);

  // Sense the configuration button (external switch)
  pinMode(ConfigPin, INPUT);
  if (senseSW(ConfigPin, ActiveLevel)) {
    Serial.println("IP configuration enable");
    auxIPConfig.load(AUX_CONFIGIP);
    auxIPConfig.on(getConfig);
    auxRestart.load(AUX_RESTART);
    auxRestart.on(setConfig);
    portal.join({ auxIPConfig, auxRestart });
  }

  portal.begin();
  client.setServer(mqttServer, mqttPort);
 
//  while (!client.connected()) {
//    Serial.println("Connecting to MQTT...");
//    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
//    //if (client.connect("ESP32Client" )) {
// 
//      Serial.println("connected");
// 
//    } else {
// 
//      Serial.print("failed with state ");
//      Serial.print(client.state());
//      delay(2000);
// 
//    }
//  }
 
}
 
void loop() {
  portal.handleClient();
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
  //values.add(23);
 
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
 
  if (client.publish("esp/test", JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
      //while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
    //if (client.connect("ESP32Client" )) {
      delay(2000);
      Serial.println("connected");
 
    } else {
      portal.handleClient();
      Serial.print("failed with state ");
      Serial.print(client.state());
      //delay(2000);
    }
    }

 
  //client.loop();
  Serial.println("-------------");
 
  //delay(10000);
 
}
