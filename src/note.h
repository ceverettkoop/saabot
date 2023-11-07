#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>

struct NoteRequest{
    bool channels[6];


};


void start_note (byte chan, byte note, byte volume);
void stop_note (byte chan);
void handle_note_on(byte channel, byte pitch, byte velocity);
void handle_note_off(byte channel, byte pitch, byte velocity);
short int get_channel_out();
bool is_channel_free();
void proc_attack(short int i);
void proc_decay(short int i);
void proc_release(short int i);

#endif /* NOTE_H */
