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
// spi things
unsigned long spi_write_address = 0;

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
  spi_erase_flash();
  update_data(now);
  update_status_led(now);
  check_serial_commands();
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
      spi_chip_query();
    else if (str.substring(0) == "erase_flash") {
      erase_flag = 1;
      spi_erase_flash();
    }
    else if (str.substring(0) == "init_flash")
      spi_init_flash();
    else if (str.substring(0) == "test_flash")
      spi_test_flash();
  }
}

void dump_data_to_serial(void) {
  /* Dump all the data in the EEPROM to the serial port
  */
  Serial.println("Printing all flash data to serial.");
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

/* push button *********************************************************************/

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

/* /push button *********************************************************************/

/* SPI *********************************************************************/

void spi_test_flash(void) {
  /* Test reading, writing and navigating in flash
  */
  unsigned long func_enter = millis();
  sprintf(debug_string, "test_flash_enter(%i)", func_enter);
  Serial.println(debug_string);
  spi_init_flash();
  spi_write_byte(0xde);
  spi_write_byte(0xad);
  spi_write_byte(0xbe);
  spi_write_byte(0xef);
  byte data = 0;
  for (int ctr=0; ctr<100; ctr++) {
    data = spi_read_byte(ctr);
    sprintf(debug_string, "0x%04x: 0x%02x", ctr, data);
    Serial.println(debug_string);
  }
  unsigned long func_exit = millis();
  sprintf(debug_string, "test_flash_exit(%i, %i)", func_exit, func_exit - func_enter);
  Serial.println(debug_string);
}

void spi_init_flash(void) {
  /* Set up the flash chip and find the next location to write
  */
  spi_write_address = spi_find_next_write_location();
}

unsigned long spi_find_next_write_location(void) {
  /* Scan through the flash chip and find the next location to write 
  */
  unsigned long rv;
  #ifdef DEBUG_LOGGING
  Serial.print("spi_found_data_until: ");
  Serial.println(rv);
  #endif
  return rv;
}

byte spi_read_byte(unsigned long address) {
  /* Read a byte from the given address
  */
  digitalWrite(SPI_CS, LOW);
  byte return0 = SPI.transfer(0x03);
  byte return1 = SPI.transfer((address >> 16) & 0xff);
  byte return2 = SPI.transfer((address >> 8) & 0xff);
  byte return3 = SPI.transfer((address >> 0) & 0xff);
  byte return4 = SPI.transfer(0x00); 
  digitalWrite(SPI_CS, HIGH);
  return return4;
}

void spi_write_byte(byte data) {
  /* Write a byte to the current address
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x02);
  SPI.transfer((spi_write_address >> 16) & 0xff);
  SPI.transfer((spi_write_address >> 8) & 0xff);
  SPI.transfer((spi_write_address >> 0) & 0xff);
  SPI.transfer(data); 
  digitalWrite(SPI_CS, HIGH);
  spi_write_address++;
}

void spi_write_enable(void) {
  /* Set the write-enable latch
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x06);
  digitalWrite(SPI_CS, HIGH);
}

void spi_chip_query(void) {
  /* Query the flash chip and print information about it to the serial line
  */
  byte flash_info[] = {0,0,0,0,0,0};
  spi_read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

void spi_read_flash_info(byte *return_array) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  return_array[0] = SPI.transfer(0x90);
  return_array[1] = SPI.transfer(0x00);
  return_array[2] = SPI.transfer(0x00);
  return_array[3] = SPI.transfer(0x00);
  return_array[4] = SPI.transfer(0x00);
  return_array[5] = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
}

void spi_erase_flash(void) {
  /* Erase the flash chip
  */
  if (!erase_flag) 
    return;
  erase_flag = 0;
  Serial.println("erasing entire flash chip...");
  spi_write_enable();
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0xc7);
  digitalWrite(SPI_CS, HIGH);
  unsigned int ctr = 0;
  while(spi_busy()) {
    Serial.print("busy_");
    Serial.println(ctr);
    ctr++;
  }
  Serial.println("done erasing.");
}

void spi_erase_sector(unsigned int sector) {
  /* Erase the given 4k sector
  */
  Serial.print("erasing flash sector ");
  Serial.print(sector);
  Serial.println("...");
  unsigned long sector_address = sector << 12;
  spi_write_enable();
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x20);
  SPI.transfer((sector_address >> 16) & 0xff);
  SPI.transfer((sector_address >> 8) & 0xff);
  SPI.transfer((sector_address >> 0) & 0xff);
  digitalWrite(SPI_CS, HIGH);
  unsigned int ctr = 0;
  while(spi_busy()) {
    Serial.print("busy_");
    Serial.println(ctr);
    ctr++;
  }
  Serial.println("done erasing sector.");
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
//  spi_write_enable();
//  digitalWrite(SPI_CS, LOW); 
//  SPI.transfer(0x01);
//  byte result_lsb = SPI.transfer(0x00);
//  byte result_msb = SPI.transfer(0x00);
//  digitalWrite(SPI_CS, HIGH);
//  return result_lsb + (result_msb << 8);
  return 0;
}

bool spi_busy(void) {
  /* Is the SPI chip busy?
  */
  return spi_read_status_reg1() & 0x01;
}

void spi_wait_busy(void) {
  /* Spin while the SPI busy line is high
  */
  while(spi_busy());
}

/* \SPI **********************************************************************/

// end
