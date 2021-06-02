//
// Created by Daniël Kamp on 02/06/2021.
//

#ifndef KARPLUSSTRONG_PRESET_H
#define KARPLUSSTRONG_PRESET_H

#include "Global.h"

#define NUM_PRESETS 8
#if !defined(PLATFORM_DARWIN_X86)

#elif defined(PLATFORM_DARWIN_X86)
  #include <cmath>
#endif

struct Preset {
    float feedback;
    float dampening;
    int exciter;

    // Provide a one-stop setup function
    void setup(float fb, float damp, int exc) {
      feedback = fb;
      dampening = damp;
      exciter = exc;
    }
};

class PresetEngine {
  public:
    PresetEngine() {iterator=0; dial = 0;};
    ~PresetEngine() {delete[] presets;};

    void import(float n_feedback, float n_dampening, int n_exciter) {
      if(iterator < NUM_PRESETS) {
        presets[iterator] = new Preset;
        presets[iterator]->setup(n_feedback, n_dampening, n_exciter);

        iterator++;
      }
    };

    void turn(int amount) {
      if(dial <= 100 - amount) {
        // Find step per preset
        int step = (100 / NUM_PRESETS);
        dial += amount;

        int pr = ceil(dial / step);
        float factor = (dial - ((pr - 1) * step)) / step;

        feedback = factor * presets[pr]->feedback + (1 - factor) * presets[pr - 1]->feedback;
        dampening = factor * presets[pr]->dampening + (1 - factor) * presets[pr - 1]->dampening;

        if (factor > 0.5) {
          exciter = presets[pr]->exciter;
        } else {
          exciter = presets[pr - 1]->exciter;
        }
      }
    }
  private:
    Preset* presets[NUM_PRESETS];
    int iterator;
    // 0 - 100
    int dial;

    float feedback;
    float dampening;
    int exciter;
};

#endif //KARPLUSSTRONG_PRESET_H
