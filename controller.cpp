#include "controller.h"


uint8_t controller::readState()
{

    uint8_t return_value = 0x0;

    return_value |= 0x2;
    return_value |= saved_state & 1;
    saved_state >> 1;
    return return_value;

}

void controller::setState( int key)
{

    state |= 1 << key;
}

void controller::saveState()
{
    saved_state = state;
    state = 0;
}

