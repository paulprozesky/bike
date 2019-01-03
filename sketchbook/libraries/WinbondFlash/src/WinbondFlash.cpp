#include "Arduino.h"
#include "WinbondFlash.h"
#include <SPI.h>

#define CS_LOW digitalWrite(cs_pin, LOW)
#define CS_HIGH digitalWrite(cs_pin, HIGH)

WinbondFlash::WinbondFlash(byte chip_select, byte size_in_mb) {
  cs_pin = chip_select;
  size_in_mb = size_in_mb;
  write_address = 0;
  pinMode(cs_pin, OUTPUT);
  SPI.begin();
  CS_LOW;
}

void WinbondFlash::test(void) {
  /* Test reading, writing and navigating in flash
  */
  unsigned long func_enter = millis();
  sprintf(debug_string, "test_flash_enter(%lu)", func_enter);
  Serial.println(debug_string);
  init();
  write_byte(0xde);
  write_byte(0xad);
  write_byte(0xbe);
  write_byte(0xef);
  byte data = 0;
  for (int ctr=0; ctr<100; ctr++) {
    data = read_byte(ctr);
    sprintf(debug_string, "0x%04x: 0x%02x", ctr, data);
    Serial.println(debug_string);
  }
  unsigned long func_exit = millis();
  sprintf(debug_string, "test_flash_exit(%lu, %lu)", func_exit, func_exit - func_enter);
  Serial.println(debug_string);
}

void WinbondFlash::init(void) {
  /* Set up the flash chip and find the next location to write
  */
  write_address = find_next_write_location();
}

unsigned long WinbondFlash::find_next_write_location(void) {
  /* Scan through the flash chip and find the next location to write 
  */
  unsigned long rv = 0x77;
  #ifdef DEBUG_LOGGING
  Serial.print("spi_found_data_until: ");
  Serial.println(rv);
  #endif
  return rv;
}

byte WinbondFlash::read_byte(unsigned long address) {
  /* Read a byte from the given address
  */
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x03);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer((address >> 0) & 0xff);
  byte rv = SPI.transfer(0x00); 
  digitalWrite(cs_pin, HIGH);
  return rv;
}

void WinbondFlash::write_byte(byte data) {
  /* Write a byte to the current address
  */
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x02);
  SPI.transfer((write_address >> 16) & 0xff);
  SPI.transfer((write_address >> 8) & 0xff);
  SPI.transfer((write_address >> 0) & 0xff);
  SPI.transfer(data); 
  digitalWrite(cs_pin, HIGH);
  write_address++;
}

void WinbondFlash::write_enable(void) {
  /* Set the write-enable latch
  */
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x06);
  digitalWrite(cs_pin, HIGH);
}

void WinbondFlash::chip_query(void) {
  /* Query the flash chip and print information about it to the serial line
  */
  byte flash_info[] = {0,0,0,0,0,0};
  read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

void WinbondFlash::read_flash_info(byte *return_array) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(cs_pin, LOW);
  return_array[0] = SPI.transfer(0x90);
  return_array[1] = SPI.transfer(0x00);
  return_array[2] = SPI.transfer(0x00);
  return_array[3] = SPI.transfer(0x00);
  return_array[4] = SPI.transfer(0x00);
  return_array[5] = SPI.transfer(0x00);
  digitalWrite(cs_pin, HIGH);
}

void WinbondFlash::erase_chip(void) {
  /* Erase the flash chip
  */
  Serial.println("erasing entire flash chip...");
  write_enable();
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0xc7);
  digitalWrite(cs_pin, HIGH);
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
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x20);
  SPI.transfer((sector_address >> 16) & 0xff);
  SPI.transfer((sector_address >> 8) & 0xff);
  SPI.transfer((sector_address >> 0) & 0xff);
  digitalWrite(cs_pin, HIGH);
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
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x05);
  byte result = SPI.transfer(0x00);
  digitalWrite(cs_pin, HIGH);
  return result;
}

byte WinbondFlash::read_status_reg2(void) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(cs_pin, LOW);
  SPI.transfer(0x35);
  byte result = SPI.transfer(0x00);
  digitalWrite(cs_pin, HIGH);
  return result;
}

unsigned short WinbondFlash::read_status_reg_write(void) {
  /* Read the write status register from the SPI flash chip
  */
//  write_enable();
//  digitalWrite(cs_pin, LOW); 
//  SPI.transfer(0x01);
//  byte result_lsb = SPI.transfer(0x00);
//  byte result_msb = SPI.transfer(0x00);
//  digitalWrite(cs_pin, HIGH);
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
