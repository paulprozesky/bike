#define DEBUG_LOGGING

#include <WinbondFlash.h>
#include <StubFlash.h>
#include <DebouncedButton.h>

// constants for this hardware flash logger
const byte TACH_INTERRUPT_PIN = 2;
const byte SPEEDO_INTERRUPT_PIN = 3;
const int ADC_NEUTRAL_PIN = A0;
//const short UPDATE_RATE = 100;  // milliseconds - 10 Hz
const int UPDATE_RATE = 2000;
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
const byte RECORD_BYTES = 10;
const byte FILE_HEADER_MAGIC_BYTES[] = {0xbe, 0xeb, 0xee, 0x1a, 0x2b, 0x3c, 0xee, 0x77};
const byte FILE_FORMAT_VERSION = 1;

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

//WinbondFlash flash(SPI_CS, 64);
StubFlash flash;
unsigned long flash_counter = 0;

DebouncedButton button(BUTTON_PIN, DEBOUNCE_TIME);

char debug_string[200];

struct DataRecord {
  unsigned long ctr_record;
  unsigned int ctr_tacho;
  unsigned int ctr_speedo;
  unsigned int adc_neutral;
  byte check_byte;
};

struct FileHeader {  // 32 bytes total
  // FILE_HEADER_MAGIC_BYTES - 8 bytes
  byte record_version;
  byte record_len;
  byte future_use[21];
  byte check_byte;
};
#define FILE_HEADER_MAGIC_LEN 8
#define FILE_HEADER_LEN 24
#define FILE_HEADER_TOTAL_LEN 32

class Foo {
  public:
    Foo();
};

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
  flash_init();
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

void flash_init(void) {
  /* Set up the flash filesystem
  */
  flash.init();

  // check file header functionality
  write_file_header();
  FileHeader header;
  bool header_ok = read_file_header(0, &header);
  sprintf(debug_string, "header: ok(%1u) ver(%u) reclen(%u) x(0x%02x)", header_ok, header.record_version, header.record_len, header.check_byte);
  Serial.println(debug_string);
  
//  DataRecord record_one;
//  record_one.ctr_record = 11;
//  record_one.ctr_tacho = 22;
//  record_one.ctr_speedo = 33;
//  record_one.adc_neutral = 44;
//
//  write_record(&record_one);
//
//  byte some_data[256];
//  flash.read_data(0, some_data, 256);
//  print_data_array_256(some_data);
//
//  DataRecord record_from_flash;
//
//  bool res = read_record(0, &record_from_flash);
//  print_record(&record_from_flash);
//  Serial.println(res);
}

void save_to_flash(void) {
  /* Save the latest values to the flash chip
  */
  if (1 == logging_enabled) {
    #ifdef DEBUG_LOGGING
    Serial.println("logging_to_flash");
    byte data_to_write[] = {
      (byte)((flash_counter >> 24) & 0xff),
      (byte)((flash_counter >> 16) & 0xff),
      (byte)((flash_counter >> 8) & 0xff),
      (byte)((flash_counter >> 0) & 0xff),
      (byte)((last_tacho >> 8) & 0xff),
      (byte)((last_tacho >> 0) & 0xff),
      (byte)((last_speedo >> 8) & 0xff),
      (byte)((last_speedo >> 0) & 0xff),
      (byte)((adc_neutral >> 8) & 0xff),
      (byte)((adc_neutral >> 0) & 0xff)
    };
    flash_counter++;
    flash.write_data(data_to_write, RECORD_BYTES);
    #endif
  }
//  #ifdef DEBUG_LOGGING
//  else {
//    sprintf(debug_string, "not_logging_to_flash: tacho(%10u) speed(%10u) adc_neutral(%4u)", last_tacho, last_speedo, adc_neutral);
//    Serial.println(debug_string);
//  }
//  #endif
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

void flash_update_counter(void) {
  /* Find the next record counter to write
  */
  unsigned long record_counter = 0;
  for (unsigned long read_addr = 0; read_addr < flash.len_bytes - RECORD_BYTES; read_addr+=RECORD_BYTES) {
    byte counter_data[4] = {0xff, 0xff, 0xff, 0xff};
    flash.read_data(read_addr, counter_data, 4);
    record_counter += (unsigned long)(counter_data[3]) << 24;
    record_counter += (unsigned long)(counter_data[2]) << 16;
    record_counter += (unsigned long)(counter_data[1]) << 8;
    record_counter += (unsigned long)(counter_data[0]) << 0;
    sprintf(debug_string, "looked at(%lu), found(%lu)", read_addr, record_counter);
    Serial.println(debug_string);
    if (record_counter != 0xffffffff) {
      flash_counter = record_counter;
      break;
    }
  }
  #ifdef DEBUG_LOGGING
  sprintf(debug_string, "found flash_counter(%lu)", flash_counter);
  Serial.println(debug_string);
  #endif
}

/* \flash **********************************************************************/

void write_file_header(void) {
  /* Write the file header to flash
  */
  byte header_data[FILE_HEADER_TOTAL_LEN];
  for(byte ctr = 0; ctr < FILE_HEADER_TOTAL_LEN; ctr++)
    header_data[ctr] = 0xff;
  FileHeader header;
  header.record_version = FILE_FORMAT_VERSION;
  header.record_len = sizeof(DataRecord);
  memcpy(&header_data, FILE_HEADER_MAGIC_BYTES, FILE_HEADER_MAGIC_LEN);
  memcpy(&header_data[FILE_HEADER_MAGIC_LEN], &header, FILE_HEADER_LEN);
  header.check_byte = calculate_crc((byte*)&header_data, FILE_HEADER_TOTAL_LEN - 1);
  header_data[FILE_HEADER_TOTAL_LEN - 1] = header.check_byte;
  flash.write_data(header_data, sizeof(header_data));
}

bool read_file_header(unsigned long address, FileHeader *header) {
  /* Read a file header from flash, checking the magic bytes and checksum
  */
  byte header_data[FILE_HEADER_TOTAL_LEN];
  for(byte ctr = 0; ctr < FILE_HEADER_TOTAL_LEN; ctr++)
    header_data[ctr] = 0xff;
  flash.read_data(address, header_data, FILE_HEADER_TOTAL_LEN);
  int res = memcmp(header_data, FILE_HEADER_MAGIC_BYTES, FILE_HEADER_MAGIC_LEN);
  memcpy(header, header_data + FILE_HEADER_MAGIC_LEN, sizeof(FileHeader));  
  byte calculated_checkbyte = calculate_crc((byte*)&header_data, FILE_HEADER_TOTAL_LEN - 1);
  return (0 == res) && (header->check_byte == calculated_checkbyte);
}

void print_all_records(void) {
  /* Print all the known records to the serial port
  */
//  for (unsigned long read_addr = 0; read_addr < flash.len_bytes - RECORD_BYTES; read_addr+=RECORD_BYTES) {
//    DataRecord record;
//    bool res = read_record(read_addr, &record);
//    if ((0xff == record.record_version) || (0 == res))
//      break;
//    print_record(&record);
//  }
}

byte calculate_crc(byte *data, byte len) {
  /* Calculate the XOR checksum of the given data
  */
  byte checksum = 0;
  for (byte ctr = 0; ctr < len; ctr++)
    checksum ^= data[ctr];
  return checksum;
}

void write_record(DataRecord *record) {
  /* Write a record to flash
  */
  byte *record_data = (byte*)record;
  record->check_byte = calculate_crc(record_data, sizeof(DataRecord) - 1);
  flash.write_data(record_data, sizeof(DataRecord));
}

bool read_record(unsigned long address, DataRecord *record) {
  /* Read a record from the specified flash location
  */
  byte *record_data = (byte*)record;
  flash.read_data(address, record_data, sizeof(DataRecord));
  byte check_byte = calculate_crc(record_data, sizeof(DataRecord) - 1);
  return (check_byte == record->check_byte);
}

void print_record(DataRecord *record) {
  /* Print a data record to the serial port
  */
  sprintf(debug_string, "ctr(%lu) t(%u) s(%u) n(%u) x(0x%02x)",
    record->ctr_record, record->ctr_tacho, record->ctr_speedo, record->adc_neutral, record->check_byte);
  Serial.println(debug_string);
}

// end
