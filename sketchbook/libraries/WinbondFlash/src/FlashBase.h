/*
The base for flash devices.
*/

#ifndef FLASH_BASE_H
#define FLASH_BASE_H

void print_data_array_256(byte *data_array);

class FlashBase {
  public:
    FlashBase();
    
    virtual void read_data(unsigned long address, byte *return_array, unsigned int length_to_read) = 0;
    virtual void write_data(byte *data, unsigned int length_to_write) = 0;
    
    virtual void read_flash_info(byte *return_array) = 0;
    virtual void erase_chip(void) = 0;
    virtual void erase_sector(unsigned int sector) = 0;
    virtual byte read_status_reg1(void) = 0;
    virtual byte read_status_reg2(void) = 0;
    virtual unsigned short read_status_reg_write(void) = 0;
    virtual bool busy(void) = 0;
    
    void test_flash(void);
    void init(void);
    unsigned long find_next_write_address(void);
    void set_write_address(unsigned long address);
    byte read_byte(unsigned long address);
    void write_enable(void);
    void write_byte(byte data);
    void chip_query(void);
    void wait_busy(void);

    unsigned long len_bytes;
    char* debug_string;
    
  protected:
    unsigned long write_address;
};

#endif
