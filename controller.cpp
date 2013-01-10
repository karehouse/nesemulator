#include "controller.h"
#include <stdlib.h>

uint8_t controller::readState()
{
    if(!canRead)
    {
        return 0x0;
    }

    unsigned int return_value;
    return_value = keyStates[currentState];
    currentState++;
    if(currentState == 8) {
        currentState = 0;
    }
    return return_value;

}

void controller::setState( int key, bool value)
{
    buf[key] = value;
}

void controller::saveState(bool value)
{
    if ( value)
    {
        for(int i =0; i<8;i++)
        {
            keyStates[i] = buf[i] ;
            
        } 
        canRead = false;
    }
        else {
            canRead = true;
        }
}

