#ifndef TONE_H
#define TONE_H

#include <Arduino.h>

void square_on(int chan, int octave, byte tone);
void channel_off(byte chan);
void set_amp_l(byte new_amp);
void set_amp_r(byte new_amp);

#endif /* TONE_H */
