//
// Created by Daniël Kamp on 25/02/2021.
//

#include "Header/DelayLine.h"
#include <iostream>

DelayLine::DelayLine(int delayTime, float feedback, Buffer *input) {
  this->delayTime = delayTime;
  this->feedback = feedback;
  position = 0;

  x = input;
  y = new Buffer(input->getSize());
}

DelayLine::~DelayLine() {
  delete y;
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
  sample = x->getSample(position - delayTime)
      + ((y->getSample(position - delayTime - 1)
        + y->getSample(position - delayTime))
        * 0.5 * feedback);

  // Store the sample in the output buffer
  y->write(sample);
  y->tick();

  // Return the sample
  return sample;
}

void DelayLine::setDelayTime(int delayTime) {
  this->delayTime = delayTime;
}

void DelayLine::setFeedback(float feedback) {
  this->feedback = feedback;
}

void DelayLine::setdf(int delayTime, float feedback) {
  this->delayTime = delayTime;
  this->feedback = feedback;
}
