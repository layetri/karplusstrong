//
// Created by Daniël Kamp on 25/02/2021.
//

#include "Header/LowPassFilter.h"

LowPassFilter::LowPassFilter(float frequency, int samplerate, Buffer *input, Buffer *output) : Filter(frequency, samplerate, input, output) {

}

LowPassFilter::~LowPassFilter() {}

int16_t LowPassFilter::calculateSample() {
  return b0 * input->getSample(index) + b1 * input->getSample(index - 1) + b2 * input->getSample(index - 1) + a1 * output->getSample(index - 1) + a2 * output->getSample(index - 2);
}

void LowPassFilter::frequencyHandler() {
  const double ita = 1.0/ tan(M_PI*ff);
  const double q = sqrt(2.0);
  // Set coefficients
  b0 = 1.0 / (1.0 + q*ita + ita*ita);
  b1 = 2*b0;
  b2 = b0;
  a1 = 2.0 * (ita*ita - 1.0) * b0;
  a2 = -(1.0 - q*ita + ita*ita) * b0;
}
