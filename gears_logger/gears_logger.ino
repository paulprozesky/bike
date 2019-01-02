#include <SPI.h>

#define DEBUG_LOGGING

// constants for this hardware flash logger
const byte TACH_INTERRUPT_PIN = 2;
const byte SPEEDO_INTERRUPT_PIN = 3;
const int ADC_NEUTRAL_PIN = A0;
const short UPDATE_RATE = 100;  // milliseconds - 10 Hz
const short LED_RATE_LOGGING = UPDATE_RATE;
const short LED_RATE_ALIVE = 500;
const short LED_RATE_ERASING = 50;
const byte BUTTON_PIN = 7;
const byte LED_PIN = 6;  // need a 150ohm current limiting resistor
const byte SPI_CLK = 13;
const byte SPI_MOSI = 11;
const byte SPI_MISO = 12;
const byte SPI_CS = 10;
const byte DEBOUNCE_TIME = 20;
const short SHORT_PRESS = 1000;
const short LONG_PRESS = 5000;

// globals - cos arduinos seem to work like this
volatile byte logging_enabled = 0;
volatile byte erase_flag = 0;
unsigned int ctr_tacho = 0;
unsigned int ctr_speedo = 0;
volatile int last_tacho;
volatile int last_speedo;
unsigned long update_data_time = 0;
unsigned long led_time = 0;
unsigned int adc_neutral = 0;
// button things
volatile byte last_button_reading = 1;
byte button_state = 1;
unsigned long debounce_time = 0;
unsigned long button_falling_edge_time = 0;
#define BUTTON_PRESSED (0 == button_state)
#define BUTTON_RELEASED (1 == button_state)

char debug_string[100];

void setup() {
  /* This is run once at startup
  */
  noInterrupts();
  Serial.begin(115200);
  analogReference(DEFAULT);
  attachInterrupt(digitalPinToInterrupt(TACH_INTERRUPT_PIN), isr_tacho, RISING);
  attachInterrupt(digitalPinToInterrupt(SPEEDO_INTERRUPT_PIN), isr_speedo, RISING);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SPI_CS, OUTPUT);
  SPI.begin();
  digitalWrite(SPI_CS, LOW);
  interrupts();
}

void loop() {
  /* The main loop that the arduino uses for running
  */
  unsigned long now = millis();
  check_push_button(now);
  erase_flash();
  update_data(now);
  update_status_led(now);
  dump_data_to_serial();
}

void check_push_button(unsigned long time_now) {
  /* Check the button to see it it's transitioned from high to low
  */
  byte last_button_state = button_state;
  read_button_with_debounce(time_now);
  if (1 == last_button_state) {  // button was not pressed
    button_falling_edge_time = BUTTON_PRESSED ? time_now : 0;
  }
  if ((0 != button_falling_edge_time) && BUTTON_PRESSED) {
    // the button is held down and we're counting
    if (time_now > button_falling_edge_time + LONG_PRESS) {
      erase_flag = 1;
      logging_enabled = 0;
      #ifdef DEBUG_LOGGING
      Serial.println("long_press");
      #endif
    }
    else if (time_now > button_falling_edge_time + SHORT_PRESS) {
      logging_enabled = 1;
      #ifdef DEBUG_LOGGING
      Serial.println("short_press");
      #endif
    }
  }
}

void erase_flash(void) {
  /* Erase the flash chip.
  */
  if (!erase_flag) return;
  erase_flag = 0;
  Serial.println("erasing entire flash chip...");
  digitalWrite(SPI_CS, LOW);
  unsigned int result = SPI.transfer(0xc7);
  unsigned int ctr = 0;
  while(1 == spi_read_status_reg1() & 0x01) {
    Serial.print("busy_");
    Serial.println(ctr);
    ctr++;
  }
  digitalWrite(SPI_CS, HIGH);
  Serial.println("done erasing.");
}

byte spi_read_status_reg1(void) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x05);
  byte result = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
  return result;
}

byte spi_read_status_reg2(void) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x35);
  byte result = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
  return result;
}

unsigned short spi_read_status_reg_write(void) {
  /* Read the write status register from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x01);
  byte result_lsb = SPI.transfer(0x00);
  byte result_msb = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
  return result_lsb + (result_msb << 8);
}

void update_status_led(unsigned long time_now) {
  /* Update the status LED based on what the board is doing
  */
  if (time_now >= led_time) {
    unsigned int led_rate = (1 == logging_enabled) ? LED_RATE_LOGGING : LED_RATE_ALIVE; 
    led_time = time_now + led_rate;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    #ifndef DEBUG_LOGGING
    Serial.println("I'm alive.");
    #endif
  }
}

void update_data(unsigned long time_now) {
  /* Update the data if it is time to do so
  */
  if (time_now >= update_data_time) {
    update_data_time = time_now + UPDATE_RATE;
    update_counters();
    update_neutral();
    save_to_flash();
  }
}

void dump_data_to_serial(void) {
  /* Dump all the data in the EEPROM to the serial port
  */
  if(Serial.available() > 4) {
    Serial.setTimeout(UPDATE_RATE);
    String str = Serial.readStringUntil('\n');
    if(str.substring(0) == "dump") {
      Serial.println("Printing all flash data to serial.");
    }
    else if(str.substring(0) == "query") {
      digitalWrite(SPI_CS, LOW);
      sprintf(debug_string, "byte_read(0x%02x)", SPI.transfer(0x00));
      Serial.println(debug_string);
      // read the manufacturer ID of the flash chip
      unsigned int result0 = SPI.transfer(0x90);
      unsigned int result1 = SPI.transfer(0x00);
      unsigned int result2 = SPI.transfer(0x00);
      unsigned int result3 = SPI.transfer(0x00);
      unsigned int result4 = SPI.transfer(0x00);
      unsigned int result5 = SPI.transfer(0x00);
      sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", result0, result1, result2, result3, result4, result5);
      Serial.println(debug_string);
      digitalWrite(SPI_CS, HIGH);
    }
  }
}

void save_to_flash(void) {
  /* Save the latest values to the flash chip
  */
  if (1 == logging_enabled) {
    #ifdef DEBUG_LOGGING
    Serial.println("logging_to_flash");
    #endif
  }
  #ifdef DEBUG_LOGGING
  else {
    sprintf(debug_string, "not_logging_to_flash: tacho(%10u) speed(%10u) adc_neutral(%4u)", last_tacho, last_speedo, adc_neutral);
    Serial.println(debug_string);
  }
  #endif
}

void update_neutral(void) {
  /* Read the ADC value for the neutral pin 
  */
  adc_neutral = analogRead(ADC_NEUTRAL_PIN);
}

int update_counters(void) {
  /* Update the counters and write them to EEPROM
  */
  last_tacho = ctr_tacho;
  last_speedo = ctr_speedo;
  ctr_tacho = 0;
  ctr_speedo = 0;
}

void isr_tacho() {
  /* Just increment the tacho counter
  */
  ctr_tacho++;
}

void isr_speedo() {
  /* Just increment the speedo counter
  */
  ctr_speedo++;
}

byte read_button_with_debounce(unsigned long time_now) {
  /* Read the button with a software debounce
  */
  byte button_reading_now = digitalRead(BUTTON_PIN);
  if (button_reading_now != last_button_reading) {
    debounce_time = time_now + DEBOUNCE_TIME;
    last_button_reading = button_reading_now;
    #ifdef DEBUG_LOGGING
    Serial.println("debounce_start");
    #endif
  }
  else if ((debounce_time != 0) && (time_now > debounce_time)) {
    debounce_time = 0;
    button_state = button_reading_now;
    #ifdef DEBUG_LOGGING
    sprintf(debug_string, "debounce_done(%1u)", button_state);
    Serial.println(debug_string);
    #endif
  }
}

// end
