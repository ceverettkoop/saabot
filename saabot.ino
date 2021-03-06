//SAA1099P midi controller
//

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

static unsigned long lastUpdate = 0;

//Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 8;
//Pin connected to latch pin (ST_CP) of 74HC595
const int latchPin = 9;
////Pin connected to Data in (DS) of 74HC595
const int dataPin = 10;

//pin connected to WR pin of SAA1099 - active low indicates writing data
const int pinWR =  11;

//AO pin of SAA1099 -indicates address or control register target
const int pinAO =  12;

const int releaseRate = 12;
const int attackRate = 4;
const int decayRate = 4;

struct status{
  boolean channelActive;
  boolean keyOn;
  int sinceOn;
  int sinceOff;
  byte prevOctaves;
  byte currentPitch;
  int lastVolume;
  short int attackCount;
};

struct status outputStatus[] = {
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 0
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 1
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 2
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 3
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 4
  {false, false, 0, 0, 0, 0, 0, 0},  //channel 5
};

void setup(){

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
//  write_data(0x18, 0x8A);
//  write_data(0x19, 0x8A);


  // Connect the handleNoteOn function to the library,
  // so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

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


void startNote (byte chan, byte note, byte volume) {

  byte noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243}; // The 12 note-within-an-octave values for the SAA1099, starting at B
  byte octaveAdr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
  byte channelAdr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies
  byte addressOut;
  byte dataOut;

  //Shift the note down by 1, since MIDI octaves start at C, but octaves on the SAA1099 start at B
  note += 1;

  byte octave = (note / 12) - 1; //Some fancy math to get the correct octave
  byte noteVal = note - ((octave + 1) * 12); //More fancy math to get the correct note

  outputStatus[chan].prevOctaves = octave; //Set this variable so we can remember /next/ time what octave was /last/ played on this channel

  //set octave address
  addressOut = octaveAdr[chan / 2];

  //set octave data
  if (chan == 0 || chan == 2 || chan == 4) {
    dataOut = octave | (outputStatus[chan + 1].prevOctaves << 4); //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }

  if (chan == 1 || chan == 3 || chan == 5) {
    dataOut = (octave << 4) | outputStatus[chan - 1].prevOctaves ; //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }

  write_data(addressOut, dataOut); //write octave

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

  outputStatus[chan].lastVolume = vol;

	dataOut = (vol << 4) | vol;

  write_data(addressOut, dataOut);

  outputStatus[chan].sinceOff = 0;
  outputStatus[chan].sinceOn = 0;

}



void stopNote (byte chan) {

  byte addressOut = chan;
  write_data(addressOut, B00000000);
  outputStatus[chan].channelActive = false;
  outputStatus[chan].sinceOff = 0;
  outputStatus[chan].attackCount = 0;

}

//data writing through 74HC595

//AO is high to indicate address buffer input
void mode_latch(){
    digitalWrite(pinAO, HIGH);
    digitalWrite(pinWR, LOW);
    delayMicroseconds(5);
    digitalWrite(pinWR, HIGH);
}

//AO is high to indicate control register input
void mode_write(){
    digitalWrite(pinAO, LOW);
    digitalWrite(pinWR, LOW);
    delayMicroseconds(5);
    digitalWrite(pinWR, HIGH);
}

void mode_inactive(){
    digitalWrite(pinWR, HIGH);
}

void write_data(unsigned char address, unsigned char data)
{
  mode_inactive();

  //write address
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, address);
  //take the latch pin high
  digitalWrite(latchPin, HIGH);

  mode_latch(); //now inputting to address buffer
  mode_inactive(); //now off

  //write data to control register

  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, data);

  //take the latch pin high
  digitalWrite(latchPin, HIGH);

  mode_write();
  mode_inactive();
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  short int channelOut = getChannelOut();

  outputStatus[channelOut].channelActive = true;
  outputStatus[channelOut].keyOn = true;
  outputStatus[channelOut].currentPitch = pitch;

  startNote(channelOut, pitch, velocity);

  //check if another channel is free, if so play octave
  if (isChannelFree()){
    channelOut = getChannelOut();
    outputStatus[channelOut].channelActive = true;
    outputStatus[channelOut].keyOn = true;
    outputStatus[channelOut].currentPitch = (pitch+12);

    startNote(channelOut, (pitch + 12), velocity);
  }

}

void handleNoteOff(byte channel, byte pitch, byte velocity) {

  for (int i = 0; i < 6; i++){
    if ((outputStatus[i].currentPitch == pitch) && (outputStatus[i].keyOn == true)){
      outputStatus[i].keyOn = false;
    }
    if ((outputStatus[i].currentPitch == pitch + 12) && (outputStatus[i].keyOn == true)){
      outputStatus[i].keyOn = false;
    }

  }

}

short int getChannelOut(){

/*this code is for testing the two channels with built-in envelope control /*

  if (outputStatus[2].channelActive == false){
    return 2;
  }
    else{
      return 5;
    }
*/

  for (int i = 0; i < 6; i++){
    if (outputStatus[i].channelActive == false){
      return i;
    }
  }
  return 0;
}

boolean isChannelFree(){
  for (int i = 0; i < 6; i++){
    if (outputStatus[i].channelActive == false){
      return 1;
    }
  }
  return 0;
}

void processAttack(short int i){
  //ATTACK PROCESSING
    if (outputStatus[i].channelActive == true && outputStatus[i].attackCount < 4){

      outputStatus[i].sinceOn++;

      if (outputStatus[i].sinceOn >= attackRate){
        outputStatus[i].lastVolume++;
        outputStatus[i].attackCount++;
        byte dataOut = (outputStatus[i].lastVolume << 4) | outputStatus[i].lastVolume; //write new volume
        write_data(i, dataOut);
        outputStatus[i].sinceOn = 0;
      }
    }
}

void processDecay(short int i){
  //decay PROCESSING - sustain is set by upper bound
    if (outputStatus[i].channelActive == true && outputStatus[i].attackCount >= 4 && outputStatus[i].attackCount < 8 ){ //last number sets sustain

      outputStatus[i].sinceOn++;

      if (outputStatus[i].sinceOn >= decayRate){
        outputStatus[i].lastVolume--;
        outputStatus[i].attackCount++;
        byte dataOut = (outputStatus[i].lastVolume << 4) | outputStatus[i].lastVolume; //write new volume
        write_data(i, dataOut);
        outputStatus[i].sinceOn = 0;
      }
    }
}

void processRelease(short int i){
  //release PROCESSING
     //if key is off but sound is playing we are still in release
  if (outputStatus[i].keyOn == false && outputStatus[i].channelActive == true){
   outputStatus[i].sinceOff++;

   if (outputStatus[i].sinceOff >= releaseRate){
     outputStatus[i].lastVolume--;

     if (outputStatus[i].lastVolume <= 0){ //if release takes us to 0 stop note
       stopNote(i);
      }
        else{
          byte dataOut = (outputStatus[i].lastVolume << 4) | outputStatus[i].lastVolume; //write new volume
          write_data(i, dataOut);

          outputStatus[i].sinceOff = 0;
        }

    }
  }
}
