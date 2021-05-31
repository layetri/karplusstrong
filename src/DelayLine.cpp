//
// Created by Daniël Kamp on 25/02/2021.
//

#include "Header/DelayLine.h"
#include <iostream>

DelayLine::DelayLine(int delayTime, float feedback, int samplerate, Buffer *input) {
  x = input;
  y = new Buffer(input->getSize());
  z = new Buffer(input->getSize());
  lpf = new LowPassFilter(3000, samplerate, y, z);

  setDelayTime(delayTime);
  setFeedback(feedback);
  this->samplerate = samplerate;
  position = 0;

}

DelayLine::~DelayLine() {
  delete y;
  delete z;
  delete lpf;
}

// Increment the buffer position
void DelayLine::tick() {
  if(position < x->getSize()) {
    position++;
  } else {
    position -= x->getSize();
  }
}

int16_t DelayLine::process() {
  int16_t sample;

  // Run the delay line
//  sample = (int16_t) ((x->getSample(position - delayTime) + (z->getSample(position - delayTime) * feedback)) * 0.5);
  y->write(x->getSample(position - delayTime) + ((z->getSample(position - delayTime - 1) + z->getSample(position - delayTime)) * 0.5 * feedback));

  // Process the LPF
  sample = (y->getSample(position - delayTime) + lpf->process()) * 0.5;

  // Store the sample in the output buffer
//  y->write(sample);
  y->tick();
  z->tick();
  lpf->tick();

  // Return the sample
  return sample;
}

void DelayLine::setDelayTime(int delayTime) {
  this->delayTime = delayTime;
  lpf->setDelayTime(delayTime);
}

void DelayLine::setFeedback(float feedback) {
  this->feedback = feedback;
}

void DelayLine::setFilterFrequency(float ffreq) {
  lpf->setFrequency(ffreq);
}
