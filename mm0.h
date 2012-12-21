#ifndef  MM0_C
#define MM0_C
#include <stdint.h>
#include "ram.h"
class mm0 : public ram {
public:
    uint8_t read(uint16_t memloc);
    void store(uint8_t value, uint16_t memloc);
};
#endif
