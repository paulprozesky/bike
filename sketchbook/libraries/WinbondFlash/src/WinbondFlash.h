/*
Interface to the Winbond W25Q64BV via SPI.
*/

#ifndef WINBOND_FLASH_H
#define WINBOND_FLASH_H

#include "Arduino.h"
#include "FlashBase.h"

class WinbondFlash : public FlashBase {
  public:
    WinbondFlash(byte chip_select, byte len_mb);
    void read_data(unsigned long address, byte *return_array, unsigned int length_to_read);
    void write_enable(void);
    void write_data(byte *data, unsigned int length_to_write);
    void read_flash_info(byte *return_array);
    void erase_chip(void);
    void erase_sector(unsigned int sector);
    byte read_status_reg1(void);
    byte read_status_reg2(void);
    unsigned short read_status_reg_write(void);
    bool busy(void);

  private:
    byte cs_pin;
    byte len_mb;
};

#endif
