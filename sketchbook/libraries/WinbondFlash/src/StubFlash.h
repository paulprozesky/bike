#ifndef STUB_FLASH_H
#define STUB_FLASH_H

#include "Arduino.h"
#include "FlashBase.h"

#define STUB_MEMORY_LEN_BYTES 300

class StubFlash : public FlashBase {
  public:
    StubFlash();
    void read_data(unsigned long address, byte *return_array, unsigned int length_to_read);
    void write_data(byte *data, unsigned int length_to_write);
    void read_flash_info(byte *return_array);
    void erase_chip(void);
    void erase_sector(unsigned int sector);
    byte read_status_reg1(void);
    byte read_status_reg2(void);
    unsigned short read_status_reg_write(void);
    bool busy(void);
    
    private:
      byte memory[STUB_MEMORY_LEN_BYTES];
};

#endif
