#include "note.h"
#include "io.h"

struct Status output_status[6];
const int releaseRate = 12;
const int attackRate = 4;
const int decayRate = 4;

void start_note (byte chan, byte note, byte volume) {

    byte noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243}; // The 12 note-within-an-octave values for the SAA1099, starting at B
    byte octaveAdr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
    byte channelAdr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies
    byte addressOut;
    byte dataOut;

    //Shift the note down by 1, since MIDI octaves start at C, but octaves on the SAA1099 start at B
    note += 1;

    byte octave = (note / 12) - 1; //Some fancy math to get the correct octave
    byte noteVal = note - ((octave + 1) * 12); //More fancy math to get the correct note

    //skip octave setting if same as before
    if(output_status[chan].prevOctaves != octave){
        //if new octave set it here
        output_status[chan].prevOctaves = octave; //Set this variable so we can remember /next/ time what octave was /last/ played on this channel
        //set octave address
        addressOut = octaveAdr[chan / 2];
        //set octave data
        if (chan == 0 || chan == 2 || chan == 4) {
            dataOut = octave | (output_status[chan + 1].prevOctaves << 4); //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
        }
        if (chan == 1 || chan == 3 || chan == 5) {
            dataOut = (octave << 4) | output_status[chan - 1].prevOctaves ; //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
        }
        write_data(addressOut, dataOut); //write octave
    }
    
    //Note addressing and playing code
    //Set address to the channel's address
    addressOut = channelAdr[chan];

    //set note data and write address + data
    dataOut = noteAdr[noteVal];
    write_data(addressOut, dataOut);

    //Setting volume to match velocity - decay and release handled elsewhere
    //Set the Address to the volume channel
    addressOut = chan;

    //code to treat velocity as volume, reduce upper bound so that attack doesn't take us past max
    byte vol = (volume / 8);
    if (vol <= 0){
        vol = 1;
    }
    if (vol >= 11){
        vol = 11;
    }

    output_status[chan].lastVolume = vol;

	dataOut = (vol << 4) | vol;

    write_data(addressOut, dataOut);

    output_status[chan].sinceOff = 0;
    output_status[chan].sinceOn = 0;
}

void stop_note (byte chan) {
    byte addressOut = chan;
    write_data(addressOut, B00000000);
    output_status[chan].channelActive = false;
    output_status[chan].sinceOff = 0;
    output_status[chan].attackCount = 0;
}

void handle_note_on(byte channel, byte pitch, byte velocity) {
    
    //startNote(1, 64, 32); //set a note for some reason?
    short int channelOut = get_channel_out();

    output_status[channelOut].channelActive = true;
    output_status[channelOut].keyOn = true;
    output_status[channelOut].currentPitch = pitch;

    start_note(channelOut, pitch, velocity);

    //check if another channel is free, if so play octave
    if (is_channel_free()){
        channelOut = get_channel_out();
        output_status[channelOut].channelActive = true;
        output_status[channelOut].keyOn = true;
        output_status[channelOut].currentPitch = (pitch+12);

        start_note(channelOut, (pitch + 12), velocity);
    }

}

void handle_note_off(byte channel, byte pitch, byte velocity) {

    for (int i = 0; i < 6; i++){
        if ((output_status[i].currentPitch == pitch) && (output_status[i].keyOn == true)){
            output_status[i].keyOn = false;
        }
        if ((output_status[i].currentPitch == pitch + 12) && (output_status[i].keyOn == true)){
            output_status[i].keyOn = false;
        }
    }
}

short int get_channel_out(){
/*this code is for testing the two channels with built-in envelope control /*

    if (output_status[2].channelActive == false){
        return 2;
    }
        else{
            return 5;
        }
*/
    for (int i = 0; i < 6; i++){
        if (output_status[i].channelActive == false){
            return i;
        }
    }
    return 0;
}

bool is_channel_free(){
    for (int i = 0; i < 6; i++){
        if (output_status[i].channelActive == false){
            return 1;
        }
    }
    return 0;
}

void proc_attack(short int i){
    //ATTACK PROCESSING
        if (output_status[i].channelActive == true && output_status[i].attackCount < 4){

            output_status[i].sinceOn++;

            if (output_status[i].sinceOn >= attackRate){
                output_status[i].lastVolume++;
                output_status[i].attackCount++;
                byte dataOut = (output_status[i].lastVolume << 4) | output_status[i].lastVolume; //write new volume
                write_data(i, dataOut);
                output_status[i].sinceOn = 0;
            }
        }
}

void proc_decay(short int i){
    //decay PROCESSING - sustain is set by upper bound
        if (output_status[i].channelActive == true && output_status[i].attackCount >= 4 && output_status[i].attackCount < 8 ){ //last number sets sustain

            output_status[i].sinceOn++;

            if (output_status[i].sinceOn >= decayRate){
                output_status[i].lastVolume--;
                output_status[i].attackCount++;
                byte dataOut = (output_status[i].lastVolume << 4) | output_status[i].lastVolume; //write new volume
                write_data(i, dataOut);
                output_status[i].sinceOn = 0;
            }
        }
}

void proc_release(short int i){
    //release PROCESSING
    //if key is off but sound is playing we are still in release
    if (output_status[i].keyOn == false && output_status[i].channelActive == true){
     output_status[i].sinceOff++;

     if (output_status[i].sinceOff >= releaseRate){
         output_status[i].lastVolume--;

         if (output_status[i].lastVolume <= 0){ //if release takes us to 0 stop note
             stop_note(i);
            }
            else{
                byte dataOut = (output_status[i].lastVolume << 4) | output_status[i].lastVolume; //write new volume
                write_data(i, dataOut);
                output_status[i].sinceOff = 0;
            }
        }
    }
}
