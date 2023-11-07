#ifndef TONE_H
#define TONE_H

#include <Arduino.h>

void square_on(int chan, int octave, byte tone, int amp_r, int amp_l);
void channel_off(byte chan);

#endif /* TONE_H */
