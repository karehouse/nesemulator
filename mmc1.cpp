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

        if (! prg_rom_switch_size) //size is 32k (0x8000)
        {

            prg_bank = ((prg_bank/2) & 0x7) *2;
            cart_rom[0] = prg_rom_banks[prg_bank];
            cart_rom[1] = prg_rom_banks[prg_bank + 1];

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
        regs[2] = value;
    }
    else if (memloc >= 0xA000 )
    {
        regs[1] = value;

    }
    else if (memloc >=0x8000)
    {
        regs[0] = value;
        mirroring = value & 0x3;
        prg_rom_switch_area = value & 0x4;
        prg_rom_switch_size = value & 0x8;
        chr_rom_switching = value & 0x10;

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
   if ( memloc >=0x8000)
   {
        if ( value & 0x80 != 0)
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
            buffer.value =  (buffer.value | (((unsigned int)value&0x1) << (5-buffer.write_num)));
        }


        if( buffer.write_num == 5)
        {
            buffer.write_num = 0;
            writeReg(buffer.value, memloc);
        }
        return;
    }
            

    storeHelp(value, memloc);


}
