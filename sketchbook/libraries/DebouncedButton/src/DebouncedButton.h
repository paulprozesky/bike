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
    unsigned long get_press_time(void);
    char *debug_string;
  private:
    byte button_pin;
    byte debounce_ms;
    volatile bool last_button_reading;
    bool last_read_state;
    unsigned long debounce_time;
    unsigned long press_time;
};

#endif
