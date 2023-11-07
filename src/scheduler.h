#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

struct Status{
    bool channelActive = false;
    bool keyOn = false;
    int sinceOn = 0;
    int sinceOff = 0;
    byte prevOctaves = -1; //so initial octave is set
    byte currentPitch = 0;
    int lastVolume = 0;
    short int attackCount = 0;
};

struct LastSetRegisters{


};


extern struct Status output_status[6];

#endif /* SCHEDULER_H */
