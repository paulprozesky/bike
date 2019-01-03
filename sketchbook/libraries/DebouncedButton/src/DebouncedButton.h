/*
A debounced pushbutton.
*/

#ifndef DEBOUNCED_BUTTON_H
#define DEBOUNCED_BUTTON_H

#include "Arduino.h"

class DebouncedButton {
  public:
    DebouncedButton(byte pin, byte debounce_ms);
    bool read(unsigned long time_now);
    bool button_state(void);
    bool is_pressed(void);
    char *debug_string;
  private:
    byte button_pin;
    byte debounce_ms;
    volatile bool last_button_reading = 1;
    bool last_read_state = 1;
    unsigned long debounce_time = 0;
};

#endif
