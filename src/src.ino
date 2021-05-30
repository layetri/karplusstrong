// KarplusStrong (dev name)
//
// Simple KarplusStrong implementation to run on Teensy audio dev board
// Copyright © 2021 Daniël Kamp
//
// This file is only an entry point for the Arduino IDE. The code is in ./main.cpp


#include <Arduino.h>
#include <MCP4922.h>
#include "Header/Knob.h"
#include "Header/KarplusStrong.h"

#define SAMPLERATE 22000
#define DEVMODE

// Init various classes
MCP4922 DAC(11,13,10,9);
KarplusStrong synth(0.9, SAMPLERATE);

Knob k_feedback(A9);

// Runtime variables
int saved_note = 0;

// Timing variables
unsigned long old_time = micros();
unsigned long new_time;
long interval = 1000000 / SAMPLERATE;

void setup() {
  // Setup hardware for IO operations
  pinMode(A0, INPUT);
  pinMode(A9, INPUT);

#ifdef DEVMODE
  Serial.begin(9600);
  synth.log();
#endif
}

void loop() {
  // Set timestamp
  new_time = micros();

  // Check scheduling
  if(new_time > old_time + interval) {
    // Read note CV input and check if new value
    int note = (analogRead(A0) / 26) + 24;
    if (note != saved_note) {
      // If new value, trigger new note
      synth.pluck(note);
    }

    // Run KS-synth master process
    auto val = synth.process();
    // Shift 16 bit output to 12 bits for DAC
    int out_val = val << 4;

    // Write to DAC
    DAC.Set(out_val, out_val);

    // Set new target timestamp
    old_time = new_time;

#ifdef DEVMODE
    Serial.println("loop done");
#endif
  } else {
    // Read IO status on off-cycles
    synth.setFeedback(k_feedback.getValue());
  }
}