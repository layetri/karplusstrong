//
// Created by Daniël Kamp on 12/02/2021.
//

#include "Header/Filter.h"
#include <iostream>


Filter::Filter(float frequency, int samplerate, Buffer *input, Buffer *output) {
  // Assign buffers
  this->input = input;
  this->output = output;
  delayTime = 0;

  // Set samplerate, buffer_size and defaults
  this->samplerate = samplerate;
  buffer_size = input->getSize();
  sample = 0.0;
  index = 0;

  setFrequency(frequency);
}

Filter::~Filter() {}

// Increment the position
void Filter::tick() {
  // Wrap the index around the circular buffer
  if(index < buffer_size) {
    index++;
  } else {
    index -= buffer_size;
  }
}

// Calculate the sample on the subclass and write to output stream
int16_t Filter::process() {
  sample = calculateSample();
  output->write(sample);

  return sample;
}

void Filter::setFrequency(float set_frequency) {
  if(set_frequency != frequency) {
    // Set frequency ratio (for Butterworth filtering)
    ff = set_frequency / samplerate;
    // Store the frequency, because why not
    frequency = set_frequency;
    // Handle the frequency change on the subclass
    frequencyHandler();
  }
}

void Filter::setDelayTime(int delayTime) {
  this->delayTime = delayTime;
}

// Placeholder functions, override in subclasses
int16_t Filter::calculateSample() {return 0;}
void Filter::frequencyHandler() {}
