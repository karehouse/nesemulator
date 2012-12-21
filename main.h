#ifndef MAIN_H
#define MAIN_H
#include "rom.h"
#include "ram.h"
#include "ppu.h"
#include "cpu.h"
#include "mmc1.h"

extern int cycles;
extern int DMA_WAIT;
extern cpu CPU;
extern ram * RAM ;
extern rom * ROM ;
extern ppu * PPU ;

#endif
