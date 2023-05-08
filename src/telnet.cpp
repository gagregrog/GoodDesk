#include "telnet.h"

void noop() {};

void Telnet::begin() {
  Serial.println("Telnet server started!");
  stream = &TelnetStream;
  stream->begin();
}

void Telnet::loop() {
  if (stream->available()) {
    String message = stream->readStringUntil('\n');
    message.trim();

    for (telnet_hook &hook : hooks) {
      if (message == hook.trigger) {
        if (hook.message) {
          stream->println(hook.message);
        } else {
          hook.cb();
        }
        break;
      }
    }
  }
}

void Telnet::registerCallback(char const *trigger, callback cb) {
  telnet_hook hook = {
    trigger,
    .message = NULL,
    cb,
  };

  hooks.push_back(hook);
}

void Telnet::registerCallback(char const *trigger, char const *message) {
  telnet_hook hook = {
    trigger,
    message,
    .cb = NULL,
  };

  hooks.push_back(hook);
}
