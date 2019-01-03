/*
Interface to the Winbond W25Q64BV via SPI.
*/

#ifndef WINBOND_FLASH_H
#define WINBOND_FLASH_H

#include "Arduino.h"

class WinbondFlash {
  public:
    WinbondFlash(byte chip_select, byte size_in_mb);
    void test(void);
    void init(void);
    unsigned long find_next_write_location(void);
    byte read_byte(unsigned long address);
    void write_byte(byte data);
    void write_enable(void);
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
};

#endif
