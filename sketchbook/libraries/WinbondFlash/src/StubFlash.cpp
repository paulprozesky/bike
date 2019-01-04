#include "StubFlash.h"

StubFlash::StubFlash(){
  len_bytes = STUB_MEMORY_LEN_BYTES;
  for (unsigned int ctr = 0; ctr < STUB_MEMORY_LEN_BYTES; ctr++)
    memory[ctr] = 0xff;
}

void StubFlash::read_data(unsigned long address, byte *return_array, unsigned int length_to_read) {
  for (unsigned int ctr = 0; ctr < length_to_read; ctr++)
    return_array[ctr] = memory[address + ctr];
}

void StubFlash::write_data(byte *data, unsigned int length_to_write) {
  for (unsigned int ctr = 0; ctr < length_to_write; ctr++) {
    memory[write_address] = data[ctr];
    write_address++;
  }
}

void StubFlash::read_flash_info(byte *return_array) {
  /* Read chip-specific info from the SPI flash chip
  */
  return_array[0] = 0x99;
  return_array[1] = 0x88;
  return_array[2] = 0x77;
  return_array[3] = 0x66;
  return_array[4] = 0x55;
  return_array[5] = 0x44;
}

void StubFlash::erase_chip(void) {
  /* Erase the flash chip
  */
  Serial.println("erasing entire flash chip...");
  for (unsigned int ctr = 0; ctr < STUB_MEMORY_LEN_BYTES; ctr++)
    memory[ctr] = 0xff;
  Serial.println("done erasing.");
}

void StubFlash::erase_sector(unsigned int sector) {
  /* Erase the given 4k sector
  */
  Serial.print("erasing flash sector ");
  Serial.print(sector);
  Serial.println("...");
  Serial.println("done erasing sector.");
}

byte StubFlash::read_status_reg1(void) {
  /* Read status register 1 from the SPI flash chip
  */
  return 0x11;
}

byte StubFlash::read_status_reg2(void) {
  /* Read status register 1 from the SPI flash chip
  */
  return 0x22;
}

unsigned short StubFlash::read_status_reg_write(void) {
  /* Read the write status register from the SPI flash chip
  */
  return 0x33;
}

bool StubFlash::busy(void) {
  /* Is the SPI chip busy?
  */
  return 0;
}
