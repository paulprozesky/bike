#include "Arduino.h"
#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(byte pin, byte debounce_ms) {
  button_pin = pin;
  debounce_ms = debounce_ms;
  pinMode(pin, INPUT_PULLUP);
}

bool DebouncedButton::button_state(void) {
  /* Get the state without updating it
  */
  return last_read_state;
}

bool DebouncedButton::is_pressed(void) {
  /* Is the button currently pressed?
  */
  return 0 == last_read_state;
}

bool DebouncedButton::read(unsigned long time_now = 0) {
  /* Read the button with a software debounce
  */
  if (0 == time_now)
    time_now = millis();
  byte button_reading_now = digitalRead(button_pin);
  if (button_reading_now != last_button_reading) {
    debounce_time = time_now + debounce_ms;
    last_button_reading = button_reading_now;
    #ifdef DEBUG_LOGGING
    Serial.println("debounce_start");
    #endif
  }
  else if ((debounce_time != 0) && (time_now > debounce_time)) {
    debounce_time = 0;
    last_read_state = button_reading_now;
    #ifdef DEBUG_LOGGING
    sprintf(debug_string, "debounce_done(%1u)", last_read_state);
    Serial.println(debug_string);
    #endif
  }
  return last_read_state;
}
