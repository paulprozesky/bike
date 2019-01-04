#include "Arduino.h"
#include "WinbondFlash.h"
#include <SPI.h>

#define DEBUG_LOGGING

#define CS_LOW digitalWrite(cs_pin, LOW)
#define CS_HIGH digitalWrite(cs_pin, HIGH)

WinbondFlash::WinbondFlash(byte chip_select, byte len_mb){
  cs_pin = chip_select;
  len_mb = len_mb;
  len_bytes = len_mb;
  len_bytes <<= 20;
  write_address = 0;
  pinMode(cs_pin, OUTPUT);
  SPI.begin();
  CS_LOW;
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
  if (len_bytes <= write_address) {
    #ifdef DEBUG_LOGGING
    sprintf(debug_string, "cannot write to address %lu, larger than flash size %lu", write_address, len_bytes);
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

