
#include "cpu.h"
#include <stdio.h>


bool cpu::irq_enable()
{
    return !cpu::getFlag('I');
}



bool cpu::getFlag(char Flag)
{
    uint8_t mask = 0x01;
    switch (Flag)
    {
        case 'N':
            mask  = mask << 1;
        case 'V':
            mask = mask << 2;
        case 'B':
            mask  = mask << 1;
        case 'D':
            mask  = mask << 1;
        case 'I':
            mask  = mask << 1;
        case 'Z':
            mask  = mask << 1;
        case 'C':
            return ( cpu::P & mask);
        default:
            printf("WRONG FLAG\n");
            break;
    }
    return -1;
}

void cpu::setFlag(char Flag, bool value)
{
    uint8_t mask = 0x01;
    switch (Flag)
    {
        case 'N':
            mask  = mask << 1;
        case 'V':
            mask = mask << 2;
        case 'B':
            mask  = mask << 1;
        case 'D':
            mask  = mask << 1;
        case 'I':
            mask  = mask << 1;
        case 'Z':
            mask  = mask << 1;
        case 'C':
            break;
        default:
            printf("WRONG FLAG\n");
            break;
    }
    if ( value )
    {
        cpu::P = cpu::P | mask;
    } else {
        cpu::P = cpu::P & (~mask);
    }    
}
void cpu::clearFlags()
{
    //cpu::P = 0x00;
}

