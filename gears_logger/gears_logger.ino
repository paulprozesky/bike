#define DEBUG_LOGGING

#include <WinbondFlash.h>
#include <DebouncedButton.h>

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

WinbondFlash flash(SPI_CS, 64);
DebouncedButton button(BUTTON_PIN, DEBOUNCE_TIME);

char debug_string[200];

void setup() {
  /* This is run once at startup
  */
  noInterrupts();
  flash.debug_string = debug_string;
  button.debug_string = debug_string;
  Serial.begin(115200);
  analogReference(DEFAULT);
  attachInterrupt(digitalPinToInterrupt(TACH_INTERRUPT_PIN), isr_tacho, RISING);
  attachInterrupt(digitalPinToInterrupt(SPEEDO_INTERRUPT_PIN), isr_speedo, RISING);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  interrupts();
}

void loop() {
  /* The main loop that the arduino uses for running
  */
  unsigned long now = millis();
  check_push_button(now);
  flash_erase();
  update_data(now);
  update_status_led(now);
  check_serial_commands();
}

void check_push_button(unsigned long time_now) {
  /* Check the button to see it it's transitioned from high to low
  */
  button.read(time_now);
  if ((0 != button.get_press_time()) && button.is_pressed()) {
    // the button is held down and we're counting
    if (time_now > button.get_press_time() + LONG_PRESS) {
      erase_flag = 1;
      logging_enabled = 0;
      #ifdef DEBUG_LOGGING
      Serial.println("long_press");
      #endif
    }
    else if (time_now > button.get_press_time() + SHORT_PRESS) {
      logging_enabled = 1;
      #ifdef DEBUG_LOGGING
      Serial.println("short_press");
      #endif
    }
  }
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

void check_serial_commands(void) {
  /* Dump all the data in the EEPROM to the serial port
  */
  if(Serial.available() > 4) {
    Serial.setTimeout(UPDATE_RATE);
    String str = Serial.readStringUntil('\n');
    if(str.substring(0) == "dump")
      dump_data_to_serial();
    else if(str.substring(0) == "query")
      flash_chip_query();
    else if (str.substring(0) == "erase_flash")
      erase_flag = 1;
    else if (str.substring(0) == "init_flash")
      flash.init();
    else if (str.substring(0) == "test_flash")
      flash.test_flash();
  }
}

void dump_data_to_serial(void) {
  /* Dump all the data in the EEPROM to the serial port
  */
  Serial.println("Printing all flash data to serial.");
}

void update_neutral(void) {
  /* Read the ADC value for the neutral pin 
  */
  adc_neutral = analogRead(ADC_NEUTRAL_PIN);
}

void update_counters(void) {
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

/* push button *********************************************************************/



/* /push button *********************************************************************/

/* flash *********************************************************************/

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

void flash_erase(void) {
  /* Erase the flash chip if the flag is set
  */
  if (!erase_flag) 
    return;
  erase_flag = 0;
  flash.erase_chip();
}

void flash_chip_query(void) {
  /* query the flash chip for info and print it to serial
  */
  byte flash_info[] = {0,0,0,0,0,0};
  flash.read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

/* \flash **********************************************************************/

// end
