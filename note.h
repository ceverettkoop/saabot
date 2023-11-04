#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>

struct status{
    bool channelActive;
    bool keyOn;
    int sinceOn;
    int sinceOff;
    byte prevOctaves;
    byte currentPitch;
    int lastVolume;
    short int attackCount;
};

extern struct status outputStatus[];

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
