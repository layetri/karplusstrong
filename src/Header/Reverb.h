//
// Created by Daniël Kamp on 25/02/2021.
//

#ifndef _SNOWSTORM_REVERB_H
#define _SNOWSTORM_REVERB_H

#include "DelayLine.h"
#include "LowPassFilter.h"

class Reverb {
  public:
    Reverb(float tail, int samplerate, Buffer *input, Buffer *output);
    ~Reverb();

    void tick();
    void process();

    void setTail(float length);
  private:
    void calculateRatios();

    Buffer *input;
    Buffer *output;
    Buffer *y;

    LowPassFilter *lpf;

    DelayLine *dl[10];

    int dl_size;
    int samplerate;
    float tail;
};


#endif //_SNOWSTORM_REVERB_H
