#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

struct timer {
  unsigned long target_interval_ms = 0;
  unsigned long start_ms = 0;
  
  void begin(unsigned long duration_ms = 1000) {
    target_interval_ms = duration_ms;   
    start_ms = millis();
  }

  bool is_active() {
    return target_interval_ms;
  }

  void clear() {
    target_interval_ms = 0;
  }

  bool overdue() {
    // subtraction ensures rollover safety
    // https://arduino.stackexchange.com/a/33577
    if (target_interval_ms && (millis() - start_ms >= target_interval_ms)) {
      clear();
      return true;
    }

    return false;
  }

  unsigned long remaining() {
    if (!is_active()) {
      return 0;
    }

    unsigned long elapsed_ms = millis() - start_ms;
    unsigned long remaining_ms = target_interval_ms - elapsed_ms;

    return remaining_ms;
  }
};

#define MIN_TO_MS(min) min * 60 * 1000


#endif
