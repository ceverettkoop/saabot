//SAA1099P midi controller
//

//channel 0 = square
//channel 1 = controls 2s envelope
//channel 2 = custom tone
//channel 3 = square
//channel 4 = square
//channel 5 = percussion

#include <MIDI.h>

/*
struct MySettings : public midi::DefaultSettings
{
    static const unsigned SysExMaxSize = 1024; // Accept SysEx messages up to 1024 bytes long.
    static const bool UseRunningStatus = true; // My devices seem to be ok with it.
};
*/

// read through midi jack
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

//read through usb serial
struct CustomBaud : public midi::DefaultSettings{
    static const long BaudRate = 38400; // Baud rate for hairless
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, CustomBaud);


static unsigned long lastUpdate = 0;
static unsigned long lastUpdateClock = 0;

//Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 8;
//Pin connected to latch pin (ST_CP) of 74HC595
const int latchPin = 9;
////Pin connected to Data in (DS) of 74HC595
const int dataPin = 10;

//pin connected to WR pin of SAA1099 - active low indicates writing data
const int pinWR = 2 ; //2 for pcb 11 for proto

//AO pin of SAA1099 -indicates address or control register target
const int pinAO = 3; //3 for pcb 12 for proto


const int releaseRate = 32;
const int attackRate = 8;
const int decayRate = 4;

struct status{
  boolean channelActive;
  boolean keyOn;
  boolean isPerc;
  int sinceOn;
  int sinceOff;
  byte prevOctaves;
  byte currentPitch;
  byte inputChannel;
  int lastVolume;
  short int attackCount;
};

struct status outputStatus[] = {
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 0
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 1
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 2
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 3
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 4
  {false, false, false, 0, 0, 0, 0, 0, 0, 0},  //channel 5
};

void setup(){

  //8mhz on A5???
    // LUT0 is the D input, which is connected to the flipflop output
  CCL.LUT0CTRLB = CCL_INSEL0_FEEDBACK_gc; // Input from sequencer
  CCL.TRUTH0 = 1; // Invert the input
  CCL.SEQCTRL0 = CCL_SEQSEL0_DFF_gc; // D flipflop
  // The following line configures using the system clock, OUTEN, and ENABLE
  CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;
  // LUT1 is the D flipflop G input, which is always high
  CCL.TRUTH1 = 0xff;
  CCL.LUT1CTRLA = CCL_ENABLE_bm; // Turn on LUT1
  CCL.CTRLA = 1; // Enable the CCL

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

  //Disable envelopes on Channels 2
  write_data(0x18, 0x00);
  write_data(0x19, B10101110);   //set channel 5 envelope

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
  Serial.begin(38400); //for midi over USB

  //startup noise
  startNote(0, 24, 32);
  delay(32);
  startNote(3, 48, 32);
  delay(32);
  startNote(4, 52, 32);
  delay(32);
  startNote(0, 55, 32);
  delay(32);
  startNote(3, 60, 32);
  delay(32);
  startNote(4, 64, 32);
  startNote(5, 52, 32);
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


//1ms emptydata to clock envelope at 500 hz?
if ( (now - lastUpdateClock) > 1 ) {
  lastUpdate += 1;
  write_data (0x06, 0); //should be nothing
 }


//10ms ADSR check
 if ( (now - lastUpdate) > 10 ) {
   lastUpdate += 10;

   for (int i = 0; i < 6; i++){ //reduce to 5 for drums

     processAttack(i);
     processDecay(i);
     processRelease(i);

   }

   //processPerc(5);
  }


}


void startNote (byte chan, byte note, byte volume) {

  setFrequency(chan, note);

  //Setting volume to match velocity - decay and release handled elsewhere
  //Set the Address to the volume channel

  byte addressOut = chan;

  //code to treat velocity as volume, reduce upper bound so that attack doesn't take us past max
  byte vol = (volume / 8);
  if (vol <= 0){
    vol = 1;
  }
  if (vol >= 11){
    vol = 11;
  }

  outputStatus[chan].lastVolume = vol;

  //write volume
	byte dataOut = (vol << 4) | vol;

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

void startPerc (byte chan, byte note, byte volume){

  byte addressOut = chan;
  startNote(chan, note, volume);

  write_data(0x15, B00100000); //enable noise on channel 5 only
  //write_data(0x14, B00011111); //disable frequency on channel 5
  write_data(0x16, B00100000);  //set noise generators
  write_data(0x19, B10110100);  //set channel 5 envelope

}

void customTone(byte chan, byte note, byte volume){

  //set envelope generator
  write_data(0x18, B10011010); //triangle at frequency of channel 1?
  startNote(chan, note, volume) ;
  setFrequency(1, 40); //change channel 1 frequency to midi note 1 = ??

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

  short int channelOut;

/*percussion
//if channel 10 route to channel 5 and trigger percussion
  if (channel == 10){

    channelOut = 5;
    outputStatus[channelOut].isPerc = true;
    outputStatus[channelOut].channelActive = true;
    outputStatus[channelOut].keyOn = true;
    outputStatus[channelOut].currentPitch = pitch;
    outputStatus[channelOut].inputChannel = channel;
    startPerc(channelOut, pitch, velocity);
  }
  */

  //if channel 2 route to channel 2 for custom instrument
    if (channel == 2){

      channelOut = 2;
      outputStatus[channelOut].isPerc = false;
      outputStatus[channelOut].channelActive = true;
      outputStatus[channelOut].keyOn = true;
      outputStatus[channelOut].currentPitch = pitch;
      outputStatus[channelOut].inputChannel = channel;
      customTone(channelOut, pitch, velocity);
    }

//otherwise square wave to channel 0,3,4.5
  if ((channel != 10) && (channel != 2)){
    channelOut = getSquareChannelOut();
    outputStatus[channelOut].isPerc = false;
    outputStatus[channelOut].channelActive = true;
    outputStatus[channelOut].keyOn = true;
    outputStatus[channelOut].currentPitch = pitch;
    outputStatus[channelOut].inputChannel = channel;

    startNote(channelOut, pitch, velocity);

  }

}

void handleNoteOff(byte channel, byte pitch, byte velocity) {

  for (int i = 0; i < 6; i++){
    if ((outputStatus[i].currentPitch == pitch) && (outputStatus[i].keyOn == true) && (outputStatus[i].inputChannel == channel) ){
      outputStatus[i].keyOn = false;
    }
  }

}

short int getSquareChannelOut(){
    //square channels are 0, 3, 4
  short int squares[4] = {0,3,4,5}; //comment out 5 if adding perc

  for (int i = 0; i < 4; i++){
    short int check = squares[i];
    if (outputStatus[check].channelActive == false){
      return check;
    }
  }

  //if all are busy pick the middle note
  if ( (outputStatus[1].currentPitch < outputStatus[0].currentPitch) && (outputStatus[1].currentPitch > outputStatus[2].currentPitch)){
    return 1;
  }
  if ( (outputStatus[2].currentPitch < outputStatus[0].currentPitch) && (outputStatus[2].currentPitch > outputStatus[1].currentPitch)){
    return 2;
  }
  if ( (outputStatus[1].currentPitch < outputStatus[2].currentPitch) && (outputStatus[1].currentPitch > outputStatus[0].currentPitch)){
    return 1;
  }
  if ( (outputStatus[2].currentPitch < outputStatus[1].currentPitch) && (outputStatus[2].currentPitch > outputStatus[0].currentPitch)){
    return 2;
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

    //check if already Off
    if (outputStatus[i].lastVolume <= 0){ //if release takes us to 0 stop note
      stopNote(i);
    }

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


void processPerc(short int chan){
  outputStatus[chan].sinceOn++;

  if (outputStatus[chan].sinceOn++ >= 90){
    stopNote(chan);
    write_data(0x15, B00000000); //disable noise on channel 5
  }

}

void setFrequency(byte chan, byte note){
  byte noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243}; // The 12 note-within-an-octave values for the SAA1099, starting at B
  byte octaveAdr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
  byte channelAdr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies
  byte addressOut;
  byte dataOut;

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
}


/*
void toggleNoise(){

  byte noiseEnable = 0;

  for (int i = 0; i > 6; i++){
    noiseEnable = noiseEnable + outputStatus[i].isPerc * pow(2, i);
  }

  byte frequencyEnable = ~noiseEnable;
  write_data(0x15, noiseEnable);
  write_data(0x14, frequencyEnable);


}

void disableEnvelope(byte chan){

  //envelope for channel 2
  if (chan == 2){
    write_data(0x18, 0x00);
  }
  //envelope for channel 5
  else{
    write_data(0x19, 0x00);
  }

}
*/


/*
void enableEnvelope(byte chan){

//percussion channel 5
  if (chan == 5){
    write_data(0x19, B10101010); //repeat triangle
  }

}
*/
