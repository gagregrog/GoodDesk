#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266AutoIOT.h>

#include "telnet.h"
#include "jarvis.h"

ESP8266AutoIOT app((char*)"esp8266", (char*)"newcouch");
Telnet telnet;
Jarvis jarvis;

void setup() {
  Serial.begin(115200);

  // app.resetAllSettings();
  app.begin();
  telnet.registerCallback("help", "I am a good desk! I am happy to meet you!");
  telnet.registerCallback("test", "It was a good test. You did well.");
  telnet.begin();
  jarvis.begin(telnet.stream);
}

void loop() {
  app.loop();
  telnet.loop();
  jarvis.loop();
}
