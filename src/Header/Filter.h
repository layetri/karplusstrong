//
// Created by Daniël Kamp on 12/02/2021.
//

#ifndef SNOWSTORM_FILTER_H
#define SNOWSTORM_FILTER_H

#include "Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
#elif defined(PLATFORM_DARWIN_X86)
  #include <cmath>
  #include <cstdint>
#endif

#include "Buffer.h"


class Filter {
public:
    Filter(float frequency, int samplerate, Buffer *input, Buffer *output);
    virtual ~Filter();

    int16_t process();
    void tick();

    void setFrequency(float frequency);
    float getFrequency();
    int getDelay();

    virtual int16_t calculateSample();
    virtual void frequencyHandler();

protected:
    Buffer *input;
    Buffer *output;
    float frequency;
    float sample;
    double ff;

    int samplerate;
    int index;
    int delayTime;
    int buffer_size;
};


#endif //SNOWSTORM_FILTER_H
