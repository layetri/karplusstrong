//
// Created by Daniël Kamp on 17/02/2021.
//

#ifndef SNOWSTORM_BUFFER_H
#define SNOWSTORM_BUFFER_H

#include "Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
#elif defined(PLATFORM_DARWIN_X86)
  #include <cmath>
  #include <cstdint>
#endif

class Buffer {
public:
    Buffer(int length);
    ~Buffer();

    void write(int16_t sample);
    int getSize();

    int getPosition();

    int16_t getSample(int sample_position);
    int16_t getCurrentSample();
    void tick();

    int16_t& operator[] (int index) {
      return data[index];
    }

private:
    int16_t *data;
    int size;
    int position;
};

#endif //SNOWSTORM_BUFFER_H
