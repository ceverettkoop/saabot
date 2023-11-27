//adapted from https://github.com/Bobcatmodder/SAATunes
#include "tone.h"
#include "saa.h"

int octave_status[] = {-1,-1,-1,-1,-1,-1};
const byte octave_adr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
const byte channel_adr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies

//writes tone
void square_on(int chan, int octave, byte tone){
    
    byte address_out;
    byte data_out;

    //bitshift the octaves here
    //skip octave setting if same as before
    if(octave_status[chan] != octave){
        octave_status[chan] = octave; 
        //set octave address
        address_out = octave_adr[chan / 2];
        //set octave data
        if (chan == 0 || chan == 2 || chan == 4) {
            data_out = octave | (octave_status[chan + 1] << 4); //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
        }
        if (chan == 1 || chan == 3 || chan == 5) {
            data_out = (octave << 4) | octave_status[chan - 1]; //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
        }
        write_data(address_out, data_out); //write octave
    }

    //set note data and write address + data
    address_out = channel_adr[chan];
    write_data(address_out, tone);
}