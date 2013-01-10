#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "ram.h"


bool ram::preRamStore(uint8_t value, uint16_t memloc)
{
    switch (memloc)
    {
        case 0x4016://joystick
            player1.saveState((bool)value&0x1);
            return true;
            
        case 0x4014 :
            PPU->writeDMA(value);
            return true;
        case 0x2000:
            PPU->writeCtrl(value);
            return true;
        case 0x2001:
            PPU->writeMask(value);
            return true;
        case 0x2003:
            PPU->writeOAMAddress(value);
            return true;
        case 0x2004:
            PPU->writeOAMData(value);
            return true;
        case 0x2006:
            PPU->writeAddr(value);
            return true;
        case 0x2007:
            PPU->writeData(value);
            return true;
        case 0x2005:
            PPU->writeScroll(value);
            return true;
    }
    if ( (memloc & 0xf000) == 0x4000)
    {
        //TODO implement APU
       return true; 
    }
    return false;
}


bool ram::preRamRead(uint16_t memloc, uint8_t * return_value)
{
    switch (memloc)
    {
        case 0x4016: //joystick 1

            *return_value = player1.readState();
            //printf("key = %d\n", *return_value);
            return true;
        case 0x4017: //joystick 2
            *return_value = 0;
            return true;
        case 0x2005:
            *return_value = PPU->SCROLL; //read scroll
            return true;
        case 0x2002:
            *return_value = PPU->readStatus();
            return true;
        case 0x2004:
            *return_value = PPU->readOAMData();
            return true;
        case 0x2007:
            *return_value = PPU->readData();
            return true;
    }
    if ( (memloc & 0xf000) == 0x4000)
    {
        //TODO implement APU
       *return_value = 0; 
       return true; 
    }

    return false;
}
uint8_t ram::read(uint16_t memloc)
{
    return readHelp(memloc);
}
uint8_t ram::readHelp(uint16_t memloc)
{
    uint8_t ret;
    if( preRamRead(memloc, &ret))
    {
        return ret;
    }
    else if (memloc >= 0xc000)
    {
        return cart_rom[1][memloc - 0xC000];
    }
    else if ( memloc >= 0x8000)
    {
        return cart_rom[0][memloc - 0x8000];
    }
    else if (memloc >= 0x6000 && memloc < 0x8000)
    {
        return cart_ram[memloc - 0x6000];
    }
    else if ( memloc <= 0x07ff)
    {
        return ram[memloc];
    }
    else if (memloc <=0x0fff)
    {
        memloc -= 0x0800;
        return ram[memloc];
    }
    else if ( memloc <=0x17ff)
    {
        memloc -= 0x1000;
        return ram[memloc];
    }
    else if (memloc <= 0x1fff)
    {
        memloc -= 0x1800;
        return ram[memloc];
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




void ram::store(uint8_t value, uint16_t memloc)
{
    storeHelp(value, memloc);
}
void ram::storeHelp(uint8_t value, uint16_t memloc)
{
    if(preRamStore(value,memloc))
    {
        return;
    }
    else if (memloc >= 0x6000 && memloc < 0x8000)
    {
        cart_ram[memloc - 0x6000] = value;
    }
    else if ( memloc <= 0x07ff)
    {
        ram[memloc] = value;
    }
    else if (memloc <=0x0fff)
    {
        memloc -= 0x0800;
        ram[memloc] = value;
    }
    else if ( memloc <=0x17ff)
    {
        memloc -= 0x1000;
        ram[memloc] = value;
    }
    else if (memloc <= 0x1fff)
    {
        memloc -= 0x1800;
        ram[memloc] = value;
    }
    else 
    {
        printf("STORE problem memloc =  0x%x\n", memloc);
//        exit(1);
    }
}
