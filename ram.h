

#ifndef  RAM_C
#define RAM_C
#include <stdint.h>
struct ram {
    virtual uint8_t read(uint16_t memloc) ;
    virtual void store(uint8_t value, uint16_t memloc) ;
    uint8_t readHelp(uint16_t memloc) ;
    void storeHelp(uint8_t value, uint16_t memloc) ;
    uint8_t cart_ram[0x2000];
    uint8_t * cart_rom[2]; //pointers to the cart_rom each should be  0x4000
    uint8_t ram[0x800];
    int mapper;
    protected:
    
    
        bool preRamStore(uint8_t value, uint16_t memloc);//for common(between memory mappers)  mmapped address 
        bool preRamRead(uint16_t memloc, uint8_t * return_value);

};



#endif
