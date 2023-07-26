#include <Arduino.h>
#include <ESP8266AutoIOT.h>

#include "telnet.h"
#include "jarvis.h"

ESP8266AutoIOT app((char*)"esp8266", (char*)"newcouch");
Telnet telnet;
Jarvis jarvis;
timer debounce;

bool is_interrupt = false;
ICACHE_RAM_ATTR void handleInterrupt() {
  is_interrupt = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SWITCH), handleInterrupt, CHANGE);

  // app.resetAllSettings();
  app.begin();
  telnet.begin();
  jarvis.begin(&telnet);
}

void loop() {
  app.loop();
  telnet.loop();
  bool canInterrupt = false;
  bool wasInterrupted = is_interrupt;
  if (wasInterrupted) {
    is_interrupt = false;

    if (!debounce.is_active() || debounce.overdue()) {
      canInterrupt = true;
      debounce.begin(1000);
    }
  }
  jarvis.loop(wasInterrupted && canInterrupt);
}
