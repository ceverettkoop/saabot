//SAA1099P midi controller
//

#include <Arduino.h>
#include <MIDI.h>
#include "io.h"
#include "note.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

static unsigned long lastUpdate = 0;

void setup(){

    //magic to get 8mhz clock on pin A5
    //credit to: https://forum.arduino.cc/t/how-to-get-8mhz-on-a-gpio-of-a-nano-every/704106/4
    // LUT0 is the D input, which is connected to the flipflop output
        CCL.LUT0CTRLB = CCL_INSEL0_FEEDBACK_gc; // Input from sequencer
        CCL.TRUTH0 = 1; // Invert the input
        CCL.SEQCTRL0 = CCL_SEQSEL0_DFF_gc; // D flipflop
        // The following line configures using the system clock, OUTEN, and ENABLE
        CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;
        // LUT1 is the D flipflop G input, which is always high
        CCL.TRUTH1 = 0xff; 
        CCL.LUT1CTRLA = CCL_ENABLE_bm; // Turn on LUT1
        CCL.CTRLA = 1; // Enable the CC
    //end magic

    //init pins
     pinMode(clockPin, OUTPUT);
     pinMode(latchPin, OUTPUT);
     pinMode(dataPin, OUTPUT);
     pinMode(pinWR, OUTPUT);
     pinMode(pinAO, OUTPUT);

 	//Reset/Enable all the sound channels ; I think this is what the original code said???
 	write_data(0x1C, 0x02);
    write_data(0x1C, 0x00);
    write_data(0x1C, 0x01);

 	//Disable the noise channels
    write_data(0x15, 0x00);

    //Disable envelopes on Channels 2 and 5
    write_data(0x18, 0x00);
    write_data(0x19, 0x00);

    //envelope control test set triangle envelope 2 and 5
    //write_data(0x18, 0x8A);
    //write_data(0x19, 0x8A);

    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);    // Put only the name of the function

    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);

    //startup noise
    startNote(3, 24, 64);
    delay(32);
    startNote(0, 48, 64);
    delay(32);
    startNote(1, 52, 64);
    delay(32);
    startNote(2, 55, 64);
    delay(32);
    startNote(3, 60, 64);
    delay(32);
    startNote(4, 64, 64);
    delay(1028);

    stopNote(0);
    stopNote(1);
    stopNote(2);
    stopNote(3);
    stopNote(4);
    stopNote(5);
}

void loop(){

 MIDI.read();
 unsigned long now = millis();

//10ms ADSR check
 if ( (now - lastUpdate) > 10 ) {
     lastUpdate += 10;

     for (int i = 0; i < 6; i++){
         processAttack(i);
         processDecay(i);
         processRelease(i);
        }
    }
}