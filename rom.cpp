#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rom.h"
#include "main.h"
#define ROM_DEBUG
long length;
void rom::read_whole_rom(int FD)
{
    lseek(FD, 0, SEEK_END);
    length = lseek(FD,0,SEEK_CUR);
    lseek(FD, 0, SEEK_SET);
    rom::file = (uint8_t * ) malloc(length);
    if(rom::file == NULL)
    {
        printf("INVALID MEMORY ALLOC");
    }
    read(FD, rom::file, length);
}


void rom::loadROM(int   FD)
{

    rom::read_whole_rom(FD);

//    NES[3] = 0x0;
    if (  strncmp((char *)rom::file, "NES", 3) )
    {
        printf("INVALID ROM FILE, first 3 bytes must be \"NES\" \n\n");
        exit(0);
    }
    rom::FD = FD;
    //number of 16k program rom pages
    rom::prgrom = rom::file[4] ;
    //Number of 8K character ROM pages (0 indicates CHR RAM)
    rom::chrrom = rom::file[5];
    rom::flag0  = rom::file[6];
    rom::flag1  = rom::file[7];
    rom::ram_banks = rom::file[8];//Number of 8kB RAM banks. For backwards compatibilty if 0 assume 1 8k ram page
    rom::mapper_number = ((rom::flag0 >> 4) | (rom::flag1 & 0xf0));
    rom::four_screen_mode = rom::flag0 & 0x08;
    rom::trainer  = rom::flag0 & 0x04;//0 = no trainer present, 1 = 512 byte trainer at 7000-71FFh
    rom::battery_backed = rom::flag0 & 0x02; //SRAM at 6000-7FFFh battery backed.  0= no, 1 = yes
    rom::mirroring = rom::flag0 & 0x01;// Mirroring.  0 = horizontal, 1 = vertical.
    rom::file += 16; //where the data starts
    
}

void rom::setupRam()
{
    if(rom::mapper_number == 0)
    {
       if ( rom::prgrom == 1 )
       {
            RAM->cart_rom[0] = (uint8_t*) malloc(0x4000);
            memcpy(RAM->cart_rom[0], rom::file, 0x4000);
            RAM->cart_rom[1] = RAM->cart_rom[0];
            printf("file: %x , rom: %x \n\n", rom::file[0], RAM->cart_rom[0][0]);
        } else if (rom::prgrom == 2)
        {
            RAM->cart_rom[0] = (uint8_t*) malloc(0x4000);
            RAM->cart_rom[1] = (uint8_t*) malloc(0x4000);
            memcpy(RAM->cart_rom[0], rom::file, 0x4000);
            memcpy(RAM->cart_rom[1], (rom::file+0x4000), 0x4000);
        } else {
            printf("error too much prgrom??\n\n\n");
            exit(1);
        }




        PPU->chr_rom[0] = (uint8_t*) malloc(0x1000);
        PPU->chr_rom[1] = (uint8_t*) malloc(0x1000);


        if( rom::chrrom == 1)
        {
            memcpy(PPU->chr_rom[0] , rom::file + (0x4000 * rom::prgrom) , 0x1000);
            memcpy(PPU->chr_rom[1] , rom::file + (0x4000 * rom::prgrom)+ 0x1000 , 0x1000);
        }  
        else if (rom::chrrom != 0)
        {
            printf( " INVALID CHRROM amount using no mapper!\n\n");
            exit(1);
        }

    }
    else if ( rom::mapper_number == 1)
    {
        mmc1* mmc = dynamic_cast<mmc1*>(RAM);
        if ( mmc == NULL)
        {
            printf("Dynamic cast RAM -> mmc1 Failed in rom.cpp\n\n");
            assert(false);
        }
        if ( rom::prgrom > 16)
        {
            printf("BAD ASSUMPTION OF NUMBER OF PRGROM BANKS POOSIBLE IN MMC1\n\n\n");
            //see mmc1.h
            assert(false);
        }
        
        for(int i =0; i<rom::prgrom; i++)
        {
            mmc->prg_rom_banks[i] = (uint8_t *) malloc(0x4000);
            if( length < (16 + (0x4000 * (i+1))))
            {
                goto file_not_long_enough;
            }
            memcpy(mmc->prg_rom_banks[i] , rom::file + (0x4000 * i) , 0x4000);
        }
        mmc->cart_rom[0] = mmc->prg_rom_banks[0];
        mmc->cart_rom[1] = mmc->prg_rom_banks[prgrom-1];

        if( chrrom > 0 )
        {
            for(int i = 0; i< chrrom; i++)
            {
                if( length < (16 + (0x4000 * (prgrom)) +( 0x2000 * (i+1))));
                {
                    goto file_not_long_enough;
                }
                mmc->chr_rom_banks[2*i] = (uint8_t *) malloc(0x1000);
                mmc->chr_rom_banks[(2*i) + 1] = (uint8_t *) malloc(0x1000);
                memcpy(mmc->chr_rom_banks[2*i] , rom::file + (0x4000 * prgrom) + (0x1000 * (2*i)), 0x1000);
                memcpy(mmc->chr_rom_banks[(2*i)+1] , rom::file + (0x4000 * prgrom) + ( 0x1000 * ((2*i) + 1 ) ), 0x1000);
            }
            //memcpy(PPU->vram, mmc->chr_rom_banks[0] , 0x2000); //copy first bank by default
            PPU->chr_rom[0] = mmc->chr_rom_banks[0];
            PPU->chr_rom[1] = mmc->chr_rom_banks[1];
        } else
        {
            //chrRAM ......
            PPU->chr_rom[0] = (uint8_t*) malloc(0x1000);
            PPU->chr_rom[1] = (uint8_t*) malloc(0x1000);
        }

    }


            



#ifdef ROM_DEBUG
    printf("prgrom = %d\n", rom::prgrom);
    printf("chrrom = %d\n", rom::chrrom);
    printf("mapper_number = %d\n", rom::mapper_number);
    printf("prgram = %d\n", rom::ram_banks);
    printf("mirroring = %d\n", rom::mirroring);
    printf("rom::ram_banks = %d\n", rom::ram_banks);
#endif
    rom::file -=16;
    free(rom::file);
    close(FD);
    return;

file_not_long_enough:
    printf("prgrom = %d\n", rom::prgrom);
    printf("chrrom = %d\n", rom::chrrom);
    printf("mapper_number = %d\n", rom::mapper_number);
    printf("prgram = %d\n", rom::ram_banks);
    printf("mirroring = %d\n", rom::mirroring);
    printf("rom::ram_banks = %d\n", rom::ram_banks);
    printf("ERROR : .nes header is wrong or file is corrupted\n from rom.cpp\n\n");
    printf("Header says there is more data then exists in the file: \n");
    printf(" File length is : %ld\n", length);
    free(rom::file-16);
    close(FD);
    exit(1);
}

