#include "Arduino.h"
#include "FlashBase.h"

#define DEBUG_LOGGING

void print_data_array_256(byte *data_array) {
  /* Print the contents of a data array that is 256 bytes long
  */
  for (byte row = 0; row < 32; row++) {
    byte start_addr = row << 3;
    Serial.print(start_addr);
    Serial.print(":_");
    for (byte col = 0; col < 8; col++) {
      Serial.print(data_array[start_addr + col]);
      Serial.print("_");
    }
    Serial.println("");
  }
}

FlashBase::FlashBase() {
    len_bytes = 0;
    write_address = 0;
}

void FlashBase::test_flash(void) {
  /* Test reading, writing and navigating in flash
  */
  unsigned long func_enter = millis();
  sprintf(debug_string, "test_flash_enter(%lu)", func_enter);
  Serial.println(debug_string);
  init();

  // byte-wise writing
  write_byte(0xde);
  write_byte(0xad);
  write_byte(0xbe);
  write_byte(0xef);
  
  sprintf(debug_string, "write_address_now(%lu)", write_address);
  Serial.println(debug_string);

  // block writing
  byte data_array[256];
  for (unsigned int ctr = 0; ctr < 256; ctr++)
    data_array[ctr] = ctr;
  write_data(data_array, 256);
  
  sprintf(debug_string, "write_address_now(%lu)", write_address);
  Serial.println(debug_string);

  // byte-wise reading
  Serial.println("byte-wise reading ----");
  for (unsigned int ctr=0; ctr<256; ctr++)
    data_array[ctr] = read_byte(ctr);
  print_data_array_256(data_array);

  // block reading
  Serial.println("block reading ----");
  read_data(3, data_array, 256);
  print_data_array_256(data_array);
  
  unsigned long func_exit = millis();
  sprintf(debug_string, "test_flash_exit(%lu, %lu)", func_exit, func_exit - func_enter);
  Serial.println(debug_string);
}

void FlashBase::init(void) {
  /* Set up the flash chip and find the next free location to write
  */
  set_write_address(find_next_write_address());
}

unsigned long FlashBase::find_next_write_address(void) {
  /* Scan through the flash chip and find the next location to write
  */
  unsigned long next_write_address = 0;
  for (; next_write_address <= len_bytes; next_write_address += 256) {
    if (0xff == read_byte(next_write_address))
      break;
  }
  if (0 != next_write_address) {
    while (0xff == read_byte(next_write_address))
      next_write_address--;
  }
  #ifdef DEBUG_LOGGING
  Serial.print("find_next_write_address: ");
  Serial.println(next_write_address);
  #endif
  return next_write_address;
}

void FlashBase::set_write_address(unsigned long address) {
  /* Set the write address - NB NB NB - writes WILL fail if trying to write to
    memory that has not been erased.
  */
  write_address = address;
}

byte FlashBase::read_byte(unsigned long address) {
  /* Read a byte from the given address
  */
  byte rv;
  read_data(address, &rv, 1);
  return rv;
}

void FlashBase::write_enable(void) {
  /* Set the write-enable latch
  */
  // pass
}

void FlashBase::write_byte(byte data) {
  /* Write a byte to the current address
  */
  write_enable();
  write_data(&data, 1);
}

void FlashBase::chip_query(void) {
  /* Query the flash chip and print information about it to the serial line
  */
  byte flash_info[] = {0, 0, 0, 0, 0, 0};
  read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

void FlashBase::wait_busy(void) {
  /* Spin while the SPI busy line is high
  */
  while(busy());
}
