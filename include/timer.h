#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

struct timer {
  unsigned long dueAt = 0;
  
  void begin(unsigned long duration_ms = 1000) {
    unsigned long now = millis();
    dueAt = now + duration_ms;   
  }

  bool overdue() {
    unsigned long now = millis();
    if (dueAt && now >= dueAt) {
      dueAt = 0;
      return true;
    }

    return false;
  }

  bool is_active() {
    return dueAt;
  }
};

#endif
