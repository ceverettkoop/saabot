#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>

struct status{
    bool channelActive = false;
    bool keyOn = false;
    int sinceOn = 0;
    int sinceOff = 0;
    byte prevOctaves = -1; //so initial octave is set
    byte currentPitch = 0;
    int lastVolume = 0;
    short int attackCount = 0;
};

extern struct status outputStatus[6];

void startNote (byte chan, byte note, byte volume);
void stopNote (byte chan);
void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
short int getChannelOut();
bool isChannelFree();
void processAttack(short int i);
void processDecay(short int i);
void processRelease(short int i);

#endif /* NOTE_H */
