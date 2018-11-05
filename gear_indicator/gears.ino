#include <EEPROM.h>

#define NO_EEPROM

const byte tacho_interrupt_pin = 1;
const byte speedo_interrupt_pin = 2;
const short UPDATE_RATE = 500;  // milliseconds
const float RATIO_TOLERANCE = 0.05;  // percent

unsigned int ctr_tacho = 0;
unsigned int ctr_speedo = 0;
unsigned int check_time = 0;
unsigned short ratios[6] = {65535, 65535, 65535, 65535, 65535, 65535};  // store the ratios multiplied by 1000 so we don't need floats
byte num_ratios = 0;
byte current_gear = 0;
char debug_string[100];

void setup() {
  noInterrupts();
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(tacho_interrupt_pin), isr_tacho, RISING);
  attachInterrupt(digitalPinToInterrupt(speedo_interrupt_pin), isr_speedo, RISING);
  load_ratios_from_eeprom();
  interrupts();
}

void loop() {
  /*The loop.
  */
  int now = millis();
  if (now == check_time){
    check_time = now + UPDATE_RATE;
    main_func();
  }
//  delay(1000);
}

int main_func(void){
  /*The function that does all the work.
  */
  volatile int now_tacho = ctr_tacho;
  volatile int now_speedo = ctr_speedo;
  ctr_tacho = 0;
  ctr_speedo = 0;
  float ratio = calculate_ratio(now_tacho, now_speedo);
  byte gear = calculate_gear(ratio);
  sprintf(debug_string, "tacho(%10u) speed(%10u) ratio(%10.5f) gear(%1u)", now_tacho, now_speedo, ratio, gear);
  Serial.println(debug_string);
}

byte calculate_gear(float ratio) {
  /*Calculate the current gear based on the ratio given
  */
  // loop through the currently known ratios and find where this one fits in
  byte matched_gear = 255;
  for (byte ctr = 0; ctr < 6; ctr++) {
    float this_ratio = ratios[ctr] / 1000.0;
    float this_ratio_upper_limit = this_ratio * (1 + RATIO_TOLERANCE);
    float this_ratio_lower_limit = this_ratio * (1 - RATIO_TOLERANCE);
    bool match = (ratio > this_ratio_lower_limit) && (ratio <= this_ratio_upper_limit);
    if (match) {
      matched_gear = ctr;
      break;
    }
  }
  if (matched_gear == 255) {
    matched_gear = update_gear_table(ratio);
  }
  return matched_gear;
}

byte update_gear_table(float new_ratio) {
  /*Given a ratio that could not be matched to a currently known one, put it in the list.
  */
  byte new_gear = 255;
  if (num_ratios == 0) {
    // it's the first one
    ratios[0] = new_ratio;
    new_gear = 0;
    num_ratios = 1;
  }
  else if (new_ratio > (ratios[num_ratios] * (1 + RATIO_TOLERANCE))) {  // bigger than anything in the list
    ratios[num_ratios] = new_ratio;
    new_gear = num_ratios;
    num_ratios++;
  }
  else {
    // find the first time it's smaller than a ratio's upper limit
    unsigned short new_ratios[6];
    memcpy(new_ratios, ratios, 6*2);
    byte lower_match = 255;
    for (byte ctr = 0; ctr < num_ratios; ctr++) {
      float this_ratio = ratios[ctr] / 1000.0;
      float this_ratio_upper_limit = this_ratio * (1 + RATIO_TOLERANCE);
      if (new_ratio < this_ratio_upper_limit) {
        lower_match = ctr;
        break;
      }
    }
    if (lower_match == 255) {
      // this should never happen
      Serial.println("But somehow it did?");
      return 255;
    }
    // lower chunk
    byte dest_offset = 0;
    byte src_offset = 0;
    byte num_bytes_to_copy = lower_match * 2;
    memcpy(ratios + dest_offset, new_ratios + src_offset, num_bytes_to_copy);
    sprintf(debug_string, "lower_memcpy(%i, %i, %i)", dest_offset, src_offset, num_bytes_to_copy);
    Serial.println(debug_string);
    // upper chunk
    dest_offset = num_bytes_to_copy + 2;
    src_offset = num_bytes_to_copy;
    num_bytes_to_copy = 12 - dest_offset;
    memcpy(ratios + dest_offset, new_ratios + src_offset, num_bytes_to_copy);
    sprintf(debug_string, "upper_memcpy(%i, %i, %i)", dest_offset, src_offset, num_bytes_to_copy);
    Serial.println(debug_string);
    // new value
    ratios[lower_match] = new_ratio;
    new_gear = lower_match;
    num_ratios++;
  }
  if (new_gear < 255) {
    Serial.print("Found a new gear at ");
    Serial.print(new_gear);
    Serial.println(".");
    save_to_eeprom();
  }
  return new_gear;
}

float calculate_ratio(int tacho, int speedo) {
  /*Given the last tacho and speedo counters, calculate the current gear ratio
  */
  if (speedo == 0 || tacho == 0) {
    return -1;
  }
  return (1.0 * tacho) / speedo;
}

void write_seven_seg(int number) {
  // given an int 0 - 6, write a number to the seven seg display
  switch(number) {
    case 0:
      Serial.println("gear(N)");
      break;
    case 1:
      Serial.println("gear(1)");
      break;
    case 2:
      Serial.println("gear(2)");
      break;
    case 3:
      Serial.println("gear(3)");
      break;
    case 4:
      Serial.println("gear(4)");
      break;
    case 5:
      Serial.println("gear(5)");
      break;
    case 6:
      Serial.println("gear(6)");
      break;
    default:
      Serial.println("gear(eRR)");
      break;
  }
}

void load_ratios_from_eeprom() {
  /*Load the table of known ratios from the EEPROM.
  */
  #ifdef NO_EEPROM
  return;
  #endif
  byte version_major = EEPROM.read(0);
  byte version_minor = EEPROM.read(1);
  num_ratios = EEPROM.read(2);
  if (num_ratios == 255) {  // the default case
    num_ratios = 0;
  }
  byte address_ctr = 3;
  for (byte ctr = 0; ctr < num_ratios; ctr++) {
    ratios[ctr] = (EEPROM.read(address_ctr + 1) << 8) || EEPROM.read(address_ctr);
    address_ctr += 2;
  }
  sprintf(debug_string, "software_version(%u.%u) known_ratios(%i)", version_major, version_minor, num_ratios);
  Serial.println(debug_string);
}

void save_to_eeprom() {
  /*Save the current table to EEPROM.
  */
  #ifdef NO_EEPROM
  return;
  #endif
  byte address_ctr = 3;
  for (byte ctr = 0; ctr < num_ratios; ctr++) {
    EEPROM.write(address_ctr, ratios[ctr] & 255);
    EEPROM.write(address_ctr + 1, ratios[ctr] >> 8);
    address_ctr += 2;
  }
  sprintf(debug_string, "saved known_ratios(%i) to EEPROM", num_ratios);
  Serial.println(debug_string);
}

void isr_tacho() {
  // just increment the tacho counter
  ctr_tacho++;
}

void isr_speedo() {
  // just increment the speedo counter
  ctr_speedo++;
}

// end
