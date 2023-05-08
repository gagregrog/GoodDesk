#include <Arduino.h>
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
  telnet.begin();
  jarvis.begin(&telnet);
}

void loop() {
  app.loop();
  telnet.loop();
  jarvis.loop();
}
