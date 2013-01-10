#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <stdint.h>
#include <stdio.h>
enum
{
    A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
} ;

class controller
{
private:
    bool canRead ;
    int keyStates[8] ;
    int buf[8];
    int currentState ;
public: 
    controller()
    {
        for(int i =0; i<8;i++)
        {
            keyStates[i] = 0;
            buf[i] = 0;
        }
        canRead = false;
        currentState = 0;
        plugged_in = true;
        state = 0;
        saved_state = 0;
    } 

    unsigned int  state; //state of each key true if pressed
    unsigned int saved_state;
    bool plugged_in;

    uint8_t readState();
    void setState(int key, bool);
    void saveState(bool);

};
#endif 
