#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mm0.h"
#include "main.h"
uint8_t mm0::read(uint16_t memloc)
{
    uint8_t ret;
    if( preRamRead(memloc, &ret))
    {
        return ret;
    }
    else if (memloc >= 0xc000)
    {
        if ( ROM->prgrom == 2)
        {
            return cart_rom[1][memloc - 0xC000];

        } 
        else{
        return cart_rom[0][memloc - 0xC000];
        }
   }
    else if ( memloc >= 0x8000)
    {
        return cart_rom[0][memloc - 0x8000];
    }
    else if (memloc >= 0x6000 && memloc < 0x8000)
    {
        return mm0::cart_ram[memloc];
    }
    else if ( memloc <= 0x07ff)
    {
        return mm0::ram[memloc];
    }
    else if (memloc <=0x0fff)
    {
        memloc -= 0x0800;
        return mm0::ram[memloc];
    }
    else if ( memloc <=0x17ff)
    {
        memloc -= 0x1000;
        return mm0::ram[memloc];
    }
    else if (memloc <= 0x1fff)
    {
        memloc -= 0x1800;
        return mm0::ram[memloc];
    }
    else 
    {
        printf("READ problem memloc =  0x%x\n", memloc);
    //    exit(1);
        return 0;
    }
    printf("should never happen!!!!");
    exit(1);
    return 0;

}




void mm0::store(uint8_t value, uint16_t memloc)
{
    if(preRamStore(value,memloc))
    {
        return;
    }
    else if (memloc >= 0x6000 && memloc < 0x8000)
    {
        mm0::cart_ram[memloc] = value;
    }
    else if ( memloc <= 0x07ff)
    {
        mm0::ram[memloc] = value;
    }
    else if (memloc <=0x0fff)
    {
        memloc -= 0x0800;
        mm0::ram[memloc] = value;
    }
    else if ( memloc <=0x17ff)
    {
        memloc -= 0x1000;
        mm0::ram[memloc] = value;
    }
    else if (memloc <= 0x1fff)
    {
        memloc -= 0x1800;
        mm0::ram[memloc] = value;
    }
    else 
    {
        printf("STORE problem memloc =  0x%x\n", memloc);
//        exit(1);
    }
}
