#include "Arduino.h"
#include "WinbondFlash.h"

void WinbondFlash::spi_test_flash(void) {
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

void WinbondFlash::spi_init_flash(void) {
  /* Set up the flash chip and find the next location to write
  */
  spi_write_address = spi_find_next_write_location();
}

unsigned long WinbondFlash::spi_find_next_write_location(void) {
  /* Scan through the flash chip and find the next location to write 
  */
  unsigned long rv = 0x77;
  #ifdef DEBUG_LOGGING
  Serial.print("spi_found_data_until: ");
  Serial.println(rv);
  #endif
  return rv;
}

byte WinbondFlash::spi_read_byte(unsigned long address) {
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

void WinbondFlash::spi_write_byte(byte data) {
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

void WinbondFlash::spi_write_enable(void) {
  /* Set the write-enable latch
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x06);
  digitalWrite(SPI_CS, HIGH);
}

void WinbondFlash::spi_chip_query(void) {
  /* Query the flash chip and print information about it to the serial line
  */
  byte flash_info[] = {0,0,0,0,0,0};
  spi_read_flash_info(flash_info);
  sprintf(debug_string, "flash_info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
    flash_info[0], flash_info[1], flash_info[2], flash_info[3], flash_info[4], flash_info[5]);
  Serial.println(debug_string);
}

void WinbondFlash::spi_read_flash_info(byte *return_array) {
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

void WinbondFlash::spi_erase_flash(void) {
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

void WinbondFlash::spi_erase_sector(unsigned int sector) {
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

byte WinbondFlash::spi_read_status_reg1(void) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x05);
  byte result = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
  return result;
}

byte WinbondFlash::spi_read_status_reg2(void) {
  /* Read status register 1 from the SPI flash chip
  */
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x35);
  byte result = SPI.transfer(0x00);
  digitalWrite(SPI_CS, HIGH);
  return result;
}

unsigned short WinbondFlash::spi_read_status_reg_write(void) {
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

bool WinbondFlash::spi_busy(void) {
  /* Is the SPI chip busy?
  */
  return spi_read_status_reg1() & 0x01;
}

void WinbondFlash::spi_wait_busy(void) {
  /* Spin while the SPI busy line is high
  */
  while(spi_busy());
}
