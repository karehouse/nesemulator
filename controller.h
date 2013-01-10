#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <stdint.h>
enum
{
    A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
} ;

class controller
{
    public: 
   controller()
   {
        plugged_in = true;
        shift_location = 0;
    } 

    unsigned int  state; //state of each key true if pressed
    unsigned int saved_state;
    bool plugged_in;
    int shift_location;

    uint8_t readState();
    void setState(int key);
    void saveState();

};
#endif 
