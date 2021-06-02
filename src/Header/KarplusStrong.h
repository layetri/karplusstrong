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
  #include <ctime>
#endif

#include "Buffer.h"
#include "DelayLine.h"

class KarplusStrong {
  public:
    KarplusStrong(float feedback, int samplerate, int exciter);
    ~KarplusStrong();

    int16_t process();

    void pluck(int note);

    void setFeedback(float feedback);
    void log();
    bool available();

    #ifdef PLATFORM_DARWIN_X86
      void increaseFeedback();
      void decreaseFeedback();

      void increaseDampening();
      void decreaseDampening();

      void nextMode();
    #endif
  private:
    Buffer* buffer;
    DelayLine* delayLine;

    void setDelayTime(float frequency);
    static float mtof(int note);

    int samplerate;
    int delayTime;
    int remaining_trigger_time;

    int exciter;

    float feedback;
    bool busy;
};


#endif //KARPLUSSTRONG_KARPLUSSTRONG_H
