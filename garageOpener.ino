#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ssid and password to connect device to wifi
const char* ssid = "ssid";
const char* password = "password";

#define TRIGGER_PIN  D8  // Pin connected to the trigger pin on the ultrasonic sensor.
#define ECHO_PIN     D7  // Pin connected to the echo pin on the ultrasonic sensor.

#define RELAY_1 D1 // Relay 1 is connected to the push button input of the door opener
#define RELAY_2 D2 // Relay 2 is controls the power to the door opener

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

struct {
  int door;
  int secure;
} state;

void handleRoot() {
  server.send(200, "text/plain", "garageOpener ready!");
}

ulong read_distance_once() {
  unsigned long max_distance = 23310; // 4 m * 2 * 10^6 us/s / 343.20 m/s = 23310 us.
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIGGER_PIN, LOW);
  while(digitalRead(ECHO_PIN)==LOW);
  unsigned long start_time = micros();
  while(digitalRead(ECHO_PIN)==HIGH) {
    if (micros() - start_time > max_distance)
      break;
  }
  unsigned long end_time = micros();
  return micros() - start_time;  
}

uint read_distance() {
  byte i;
  unsigned long t = 0;
  // do three readings in a row to reduce noise
  byte j = 3;
  for(i=0;i<j;i++) {
    t += read_distance_once();
    delay(50);
  }
  t /= j;
  return (uint)(t*0.01716f);  // 34320 cm / 2 / 10^6 s
}

String makeSimpleJSON(String key, unsigned int value) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[key] = value;
  
  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  return String(buffer);
}

String makeSimpleJSON(String key, int value) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[key] = value;
  
  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  return String(buffer);
}

void handleGetDistance() {
  unsigned int dist = read_distance();

  String json = makeSimpleJSON("distance", dist);

  server.send(200, "application/json", json);
}

void handleGetDoorState() {
  String json = makeSimpleJSON("door", state.door);

  server.send(200, "application/json", json);
}

void handleGetSecureState() {
  String json = makeSimpleJSON("secure", state.secure);

  server.send(200, "application/json", json);
}

void updateRelayState(int pin, int state) {
  if (state) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}

void handleSetDoorState() {
  if (server.args() > 0) {
    String json = server.arg(0);
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(json);

    String newStateString = root["door"];
    int newState = newStateString.toInt();
    if (newState == 1) {
      state.door = newState; 
      digitalWrite(RELAY_1, state.door);
      delay(300);
      state.door = !state.door;
      digitalWrite(RELAY_1, state.door);
      
      String json = makeSimpleJSON("door", 1);

      server.send(200, "application/json", json);
      return;
    }    
  }

  String json = makeSimpleJSON("door", 0);

  server.send(200, "application/json", json);
}

void handleSetSecureState() {
  if (server.args() > 0) {
    String json = server.arg(0);
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(json);
    
    String newStateString = root["secure"];
    int newState = newStateString.toInt();
    if (((newState == 0) || (newState == 1)) && (newState != state.secure)) {
      state.secure = newState; 
      // Invert state to output to relay
      // 1 means secure = relay off...
      digitalWrite(RELAY_2, !state.secure);
    }    
  }

  handleGetSecureState();
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  uint32_t id = ESP.getChipId();
  Serial.print("ESP id: ");
  Serial.println(id, HEX);

  state.door = 0;
  state.secure = 1;
  
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIGGER_PIN, LOW);

  WiFiMulti.addAP(ssid, password);
  while(WiFiMulti.run() != WL_CONNECTED){
    Serial.println("WiFi not connected, waiting....");
    delay(1000);
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  ArduinoOTA.setHostname("garage");

  ArduinoOTA.setPassword((const char *)"garage");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/distance", handleGetDistance);

  // parse JSON and set door state, changing if need be
  server.on("/doorState", HTTP_POST, handleSetDoorState);

  // methods to get and set secure state of opener
  server.on("/secureState", HTTP_GET, handleGetSecureState);
  server.on("/secureState", HTTP_POST, handleSetSecureState);

  server.onNotFound(handleNotFound);

  server.begin();
}

void loop(void)
{ 
  ArduinoOTA.handle();
  server.handleClient();
}
