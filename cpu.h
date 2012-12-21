#ifndef CPU_H
#define CPU_H

#include <stdint.h>
struct cpu
{



    uint16_t pc;
    uint8_t A; // accumulator regiser
    uint8_t X; // general regs
    uint8_t Y;
    uint8_t S;// Stack Pointer
    uint8_t P; //processor status
    // [7 6 5 4 3 2 1 0]
    // [N V   B D I Z C] negative , overflow , nothing, BRK, Dec, IRQ disable , Zero , carry

    bool request_nmi;
    bool request_irq;
    bool irq_enable();

    bool getFlag(char Flag);
    void setFlag(char Flag, bool value);
    void clearFlags();
};





#endif
