//
// Created by Daniël Kamp on 30/05/2021.
//

#ifndef KARPLUSSTRONG_KARPLUSSTRONG_H
#define KARPLUSSTRONG_KARPLUSSTRONG_H

#include "Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
#elif defined(PLATFORM_DARWIN_X86)
  #include <cmath>
  #include <random>
  #include <cstdint>
#endif

#include "Buffer.h"
#include "DelayLine.h"

class KarplusStrong {
  public:
    KarplusStrong(float feedback, int samplerate);
    ~KarplusStrong();

    int16_t process();

    void pluck(int note);

    void setFeedback(float feedback);
    void log();
    bool available();
  private:
    Buffer* buffer;
    DelayLine* delayLine;

    void setDelayTime(float frequency);
    static float mtof(int note);

    int samplerate;
    int delayTime;
    float feedback;
    bool busy;
};


#endif //KARPLUSSTRONG_KARPLUSSTRONG_H
