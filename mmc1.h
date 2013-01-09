#ifndef  MM1_C
#define MM1_C
#include <stdint.h>
#include "ram.h"
typedef struct {
    unsigned int value; // 5 bit wide register value
    int write_num;
} reg;



class mmc1 : public ram 

{

    private:

        unsigned int regs[4];
        reg buffer;
        void writeReg(unsigned int, uint16_t);
        

    public:

        uint8_t read(uint16_t memloc);
        unsigned int mirroring;
        unsigned int chr_rom_switch_size;
        unsigned int prg_bank;
        unsigned int prg_rom_switch_area;
        unsigned int wram_disable;
        unsigned int prg_rom_switch_size;
        uint8_t * prg_rom_banks[16]; //max number .. each is suppossed to be of size 0x4000
        uint8_t * chr_rom_banks[32]; // each is supposed to be of size 0x1000
        

        void store(uint8_t value, uint16_t memloc);

};
#endif
