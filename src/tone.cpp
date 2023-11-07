//credit to https://github.com/Bobcatmodder/SAATunes
#include "tone.h"

const byte octave_adr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
const byte channel_adr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies


void square_on(int chan, int octave, byte tone, int amp_r, int amp_l){
    
    //bitshift the octaves here

    byte address_out = channel_adr[chan];




}