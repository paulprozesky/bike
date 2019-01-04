/*
Interface to the Winbond W25Q64BV via SPI.
*/

#ifndef WINBOND_FLASH_H
#define WINBOND_FLASH_H

#include "Arduino.h"

class WinbondFlash {

  public:
    WinbondFlash(byte chip_select, byte size_in_mb);
    void test_flash(void);
    void init(void);
    
    unsigned long find_next_write_address(void);
    void set_write_address(unsigned long address);
    
    void read_data(unsigned long address, byte *return_array, unsigned int length_to_read);
    byte read_byte(unsigned long address);
    
    void write_enable(void);
    void write_data(byte *data, unsigned int length_to_write);
    void write_byte(byte data);
    
    void chip_query(void);
    void read_flash_info(byte *return_array);
    
    void erase_chip(void);
    void erase_sector(unsigned int sector);
    
    byte read_status_reg1(void);
    byte read_status_reg2(void);
    unsigned short read_status_reg_write(void);
    
    bool busy(void);
    void wait_busy(void);
    
    char* debug_string;
    
  private:
    byte cs_pin;
    unsigned long write_address;
    byte size_in_mb;
    unsigned long size_in_bytes;
};

class FlashStub : public WinbondFlash {
  // stub out the read/write/chip-specific methods
};

#endif
