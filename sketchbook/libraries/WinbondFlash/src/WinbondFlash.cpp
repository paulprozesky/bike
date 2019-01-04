#include "Arduino.h"
#include "WinbondFlash.h"
#include <SPI.h>

#define DEBUG_LOGGING

#define CS_LOW digitalWrite(cs_pin, LOW)
#define CS_HIGH digitalWrite(cs_pin, HIGH)

WinbondFlash::WinbondFlash(byte chip_select, byte size_in_mb) {
  cs_pin = chip_select;
  size_in_mb = size_in_mb;
  size_in_bytes = size_in_mb;
  size_in_bytes <<= 20;
  write_address = 0;
  pinMode(cs_pin, OUTPUT);
  SPI.begin();
  CS_LOW;
}

void print_data_array_256(byte *data_array) {
  /* Print the contents of a data array that is 256 bytes long
  */
  for (byte row = 0; row < 16; row++) {
    byte start_addr = row << 3;
    Serial.print(start_addr);
    Serial.print(": ");
    for (byte col = 0; col < 8; col++)
      Serial.print(data_array[start_addr + col]);
      Serial.print(" ");
    Serial.println("");
  }
}

void WinbondFlash::test(void) {
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
  
  // block writing
  byte data_array[256];
  for (unsigned int ctr = 0; ctr < 256; ctr++)
    data_array[ctr] = ctr;
  write_data(data_array, 256);
  
  // byte-wise reading
  for (unsigned int ctr=0; ctr<256; ctr++)
    data_array[ctr] = read_byte(ctr);
  print_data_array_256(data_array);
  
  // block reading
  read_data(3, data_array, 256);
  print_data_array_256(data_array);
  
  unsigned long func_exit = millis();
  sprintf(debug_string, "test_flash_exit(%lu, %lu)", func_exit, func_exit - func_enter);
  Serial.println(debug_string);
}

void WinbondFlash::init(void) {
  /* Set up the flash chip and find the next free location to write
  */
  write_address = find_next_write_address();
}

unsigned long WinbondFlash::find_next_write_address(void) {
  /* Scan through the flash chip and find the next location to write 
  */
  unsigned long next_write_address = 0;
  for (; next_write_address <= size_in_bytes; next_write_address += 256) {
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

void WinbondFlash::set_write_address(unsigned long address) {
  /* Set the write address - NB NB NB - writes WILL fail if trying to write to
    memory that has not been erased.
  */
  write_address = address;
}

void WinbondFlash::read_data(unsigned long address, byte *return_array, unsigned int length_to_read) {
  /* Read the given length of data from the given address
  */
  CS_LOW;
  SPI.transfer(0x03);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer((address >> 0) & 0xff);
  for (unsigned int ctr = 0; ctr < length_to_read; ctr++)
    return_array[ctr] = SPI.transfer(0x00);    
  CS_HIGH;
}

byte WinbondFlash::read_byte(unsigned long address) {
  /* Read a byte from the given address
  */
  byte rv;
  read_data(address, &rv, 1);
  return rv;
}

void WinbondFlash::write_enable(void) {
  /* Set the write-enable latch
  */
  CS_LOW;
  SPI.transfer(0x06);
  CS_HIGH;
}

void WinbondFlash::write_data(byte *data, unsigned int length_to_write) {
  /* Write data of the given length to the current address
  */
  if (size_in_bytes <= write_address) {
    #ifdef DEBUG_LOGGING
    sprintf(debug_string, "cannot write to address %lu, larger than flash size %lu", write_address, size_in_bytes);
    Serial.println(debug_string);
    #endif
    return;
  }
  write_enable();
  CS_LOW;
  SPI.transfer(0x02);
  SPI.transfer((write_address >> 16) & 0xff);
  SPI.transfer((write_address >> 8) & 0xff);
  SPI.transfer((write_address >> 0) & 0xff);
  for (unsigned int ctr = 0; ctr < length_to_write; ctr++) {
    SPI.transfer(data[ctr]);
    write_address++;
  }
  CS_HIGH;
}

void WinbondFlash::write_byte(byte data) {
  /* Write a byte to the current address
  */
  write_enable();
  write_data(&data, 1);
}

void WinbondFlash::chip_query(void) {
  /* Query the flash chip and print information about it to the serial line
  */
  byte flash_info[] = {0, 0, 0, 0, 0, 0};
  read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

void WinbondFlash::read_flash_info(byte *return_array) {
  /* Read chip-specific info from the SPI flash chip
  */
  CS_LOW;
  return_array[0] = SPI.transfer(0x90);
  return_array[1] = SPI.transfer(0x00);
  return_array[2] = SPI.transfer(0x00);
  return_array[3] = SPI.transfer(0x00);
  return_array[4] = SPI.transfer(0x00);
  return_array[5] = SPI.transfer(0x00);
  CS_HIGH;
}

void WinbondFlash::erase_chip(void) {
  /* Erase the flash chip
  */
  Serial.println("erasing entire flash chip...");
  write_enable();
  CS_LOW;
  SPI.transfer(0xc7);
  CS_HIGH;
  unsigned int ctr = 0;
  while(busy()) {
    Serial.print("busy_");
    Serial.println(ctr);
    ctr++;
  }
  Serial.println("done erasing.");
}

void WinbondFlash::erase_sector(unsigned int sector) {
  /* Erase the given 4k sector
  */
  Serial.print("erasing flash sector ");
  Serial.print(sector);
  Serial.println("...");
  unsigned long sector_address = sector << 12;
  write_enable();
  CS_LOW;
  SPI.transfer(0x20);
  SPI.transfer((sector_address >> 16) & 0xff);
  SPI.transfer((sector_address >> 8) & 0xff);
  SPI.transfer((sector_address >> 0) & 0xff);
  CS_HIGH;
  unsigned int ctr = 0;
  while(busy()) {
    Serial.print("busy_");
    Serial.println(ctr);
    ctr++;
  }
  Serial.println("done erasing sector.");
}

byte WinbondFlash::read_status_reg1(void) {
  /* Read status register 1 from the SPI flash chip
  */
  CS_LOW;
  SPI.transfer(0x05);
  byte result = SPI.transfer(0x00);
  CS_HIGH;
  return result;
}

byte WinbondFlash::read_status_reg2(void) {
  /* Read status register 1 from the SPI flash chip
  */
  CS_LOW;
  SPI.transfer(0x35);
  byte result = SPI.transfer(0x00);
  CS_HIGH;
  return result;
}

unsigned short WinbondFlash::read_status_reg_write(void) {
  /* Read the write status register from the SPI flash chip
  */
//  write_enable();
//  CS_LOW; 
//  SPI.transfer(0x01);
//  byte result_lsb = SPI.transfer(0x00);
//  byte result_msb = SPI.transfer(0x00);
//  CS_HIGH;
//  return result_lsb + (result_msb << 8);
  return 0;
}

bool WinbondFlash::busy(void) {
  /* Is the SPI chip busy?
  */
  return read_status_reg1() & 0x01;
}

void WinbondFlash::wait_busy(void) {
  /* Spin while the SPI busy line is high
  */
  while(busy());
}
