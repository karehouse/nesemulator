#ifndef ROM_H
#define ROM_H

#include <stdint.h>
struct rom {
    uint8_t prgrom ;
    //Number of 8K character ROM pages (0 indicates CHR RAM)
    uint8_t chrrom ;
    uint8_t flag0  ;
    uint8_t flag1  ;
    uint8_t ram_banks ;
    uint8_t mapper_number ;
    bool four_screen_mode ;
    bool trainer  ;
    bool battery_backed ;
    bool mirroring ;
    uint8_t *file;

    int FD;

    void setupRam();
    void read_whole_rom(int FD);
    void loadROM(int FD);
};
#endif
