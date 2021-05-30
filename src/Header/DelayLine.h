//
// Created by Daniël Kamp on 25/02/2021.
//

#ifndef _SNOWSTORM_DELAYLINE_H
#define _SNOWSTORM_DELAYLINE_H

#include "Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
#elif defined(PLATFORM_DARWIN_X86)
  #include <cmath>
  #include <cstdint>
#endif

#include "Buffer.h"
#include "LowPassFilter.h"

class DelayLine {
  public:
    DelayLine(int delayTime, float feedback, Buffer *input);
    ~DelayLine();

    int16_t process();
    void tick();

    void setDelayTime(int delayTime);
    void setFeedback(float feedback);
    void setdf(int delayTime, float feedback);

  protected:
    Buffer *x;
    Buffer *y;

    float feedback;

    int delayTime;
    int position;
};


#endif //_SNOWSTORM_DELAYLINE_H
