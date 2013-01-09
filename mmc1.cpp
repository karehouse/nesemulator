#include "mmc1.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>




uint8_t mmc1::read(uint16_t memloc)
{
    
    return readHelp(memloc);


    return 0;
}

void mmc1::writeReg(unsigned int value, uint16_t memloc)
{
    if ( memloc >= 0xE000 )
    {
        //register 3
        regs[3] = value;
        prg_bank = value & 0xF;
        printf("writing to reg 3 value = %X\n\n", prg_bank);

        wram_disable = (value & 0x10) >> 4;
        //0 - enabled 1 - disabled

        if (! prg_rom_switch_size) //size is 32k (0x8000)
        {

            cart_rom[0] = prg_rom_banks[(prg_bank & 0xE)];
            cart_rom[1] = prg_rom_banks[(prg_bank& 0xE) + 1];

        } else
        {
        //size is 16k (0x4000)
            if (! prg_rom_switch_area) //switch high bank
            {
                cart_rom[1] = prg_rom_banks[prg_bank];
                cart_rom[0] = prg_rom_banks[0];
            }
            else
            {
                cart_rom[0] = prg_rom_banks[prg_bank];
                cart_rom[1] = prg_rom_banks[ROM->prgrom-1];
            }
        }
        

    }
    else if ( memloc >=0xC000 )
    {
        //This register represents CHR register 1
        // selects memory at ppu address $1000
        //ignored in 8kb mode

        regs[2] = value;

        if ( ROM->chrrom == 0 )
        {
            ;
        }
        else
        {

            if ( chr_rom_switching)
            {
                //memcpy(PPU->vram+0x1000, chr_rom_banks[value], 0x1000); 

            }
        }
        
    }
    else if (memloc >= 0xA000 )
    {
        // This Register represents CHR register 0
        regs[1] = value;
        
        if( ROM->chrrom == 0)
        {
            ;
        } else 
        {

            if ( chr_rom_switching)
            {
            //    memcpy(PPU->vram, chr_rom_banks[value], 0x1000);
            } else
            {
                printf("chr_rom_banks[1] = %p\n", chr_rom_banks[1]); 
                exit(0);
              //  memcpy(PPU->vram, chr_rom_banks[value&0xE], 0x1000);
                //memcpy(PPU->vram+0x1000, chr_rom_banks[(value&0xE) + 1], 0x1000);
            }
        }

    }
    else if (memloc >=0x8000)
    {
        // This register set different options

        regs[0] = value;
        //value &= 0x1F;
        printf("    REG 0x8000 = %5X\n", value);

        mirroring = value & 0x3;
        //  00 - 1sca  01 - 1scb
        //  10 - vert  11 - horiz

        prg_rom_switch_area = (value & 0x4) >> 2; 
        //0 - $c000 swapable and $8000 fixed to page $00
        //1 - $8000 swapable and $c000 fixed to page $0F
        // bit ignored if prg_rom_switch_size is clear ( 32k mode)


        prg_rom_switch_size = (value & 0x8) >> 3;
        //0 - 32k 1 - 16k

        chr_rom_switching = (value & 0x10) >> 4;
        //0 - 8K   1 - 4K

    }
    else
    {
        printf("should never have happened!!!");
        exit(1);
    }
}


void mmc1::store(uint8_t value, uint16_t memloc)
{
    
   //8000-9FFFh will access register 0, A000-BFFFh register 1, C000-DFFFh register 2, and E000-FFFFh register 3 
   //five writes each write writes one bit only last memloc matters for which register it is to
   if ( memloc >=0x8000)
   {
        if ( value & 0x80 )
        {
            //reset bit is set
            buffer.write_num = 0;
            buffer.value = 0;
            prg_rom_switch_area = 1;
            prg_rom_switch_size =1;
        }
        else
        {
            buffer.write_num++;
            buffer.value =  (buffer.value | (((unsigned int)value&0x1) << (buffer.write_num-1)));
        }


        if( buffer.write_num == 5)
        {
            printf("WRITE IS OCCURINGG\n\n\n");
            writeReg(buffer.value, memloc);
            buffer.write_num = 0;
            buffer.value = 0;
        }
        return;
    }
            

    storeHelp(value, memloc);


}
