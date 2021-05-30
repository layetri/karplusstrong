//
// Created by Daniël Kamp on 30/05/2021.
//

#include "Header/KarplusStrong.h"
#include <iostream>

KarplusStrong::KarplusStrong(float init_feedback, int init_samplerate) {
#if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
  Serial.println("Start init KPS");
#endif
  setDelayTime(440);
  feedback = init_feedback;
  samplerate = init_samplerate;

  buffer = new Buffer(samplerate);
  delayLine = new DelayLine(delayTime, feedback, buffer);
#if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
  Serial.println("Done init KPS");
#endif
}

KarplusStrong::~KarplusStrong() {
  delete buffer;
  delete delayLine;
}

void KarplusStrong::pluck(int note) {
  // Send impulse on delay line
  // TODO: look at different types of exciters
  //  -> Sine/Square/Saw burst, with envelope
  //  -> enveloped input signal
  //  -> ...

  std::cout << mtof(note) << std::endl;
  setDelayTime(mtof(note));
  buffer->write(1);
}

int16_t KarplusStrong::process() {
  // Do the actual delay line processing
  int16_t smp = delayLine->process();

  // Push the DL and buffer forward
  delayLine->tick();
  buffer->tick();
  // Write a 0 for the next sample
  buffer->write(0);

  // Return the value
  return smp;
}

float KarplusStrong::mtof(int note) {
  // fm  =  2(m−69)/12(440 Hz)
  return (float) (pow(2, (note-69) / 12.0) * 440.0);
}

void KarplusStrong::setDelayTime(float frequency) {
  #if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
    Serial.println("set delay time");
  #endif
  std::cout << samplerate / frequency << std::endl;
  delayLine->setDelayTime((int) (samplerate / frequency));
}

void KarplusStrong::setFeedback(float n_feedback) {

  feedback = n_feedback;
  delayLine->setFeedback(feedback);
}

void KarplusStrong::log() {
  #if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
    Serial.print("feedback: ");
    Serial.println(feedback);

    Serial.print("delaytime: ");
    Serial.println(delayTime);

    Serial.print("buffer: ");
    Serial.println(reinterpret_cast<int>(&buffer));
  #endif
}