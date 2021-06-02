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
  #include <string>
  #include <sstream>
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
    PresetEngine() {
      iterator = 0;
      dial = 0;
      factor = 0;
      preset = 0;
      step = (100.0 / NUM_PRESETS);
    };
    ~PresetEngine() {
      for(auto& pr : presets) {
        delete pr;
      }
    };

    void import(float n_feedback, float n_dampening, int n_exciter) {
      if(iterator < NUM_PRESETS) {
        presets[iterator] = new Preset;
        presets[iterator]->setup(n_feedback, n_dampening, n_exciter);

        iterator++;
      }
    };

    // For working with dynamically imported presets:
    //  check if the next preset is set.
    bool rangeTest(int amount) {
      if(dial + amount > step * preset) {
        return preset + 1 < iterator;
      }
      return true;
    }

    // Interpolate between different presets when a knob is turned.
    void rotate(int amount) {
      if(dial <= 100.0 - amount && rangeTest(amount)) {
        // Find step per preset
        dial += amount;

        // Calculate what the next preset will be
        preset = ceil(dial / step);
        factor = (dial - ((preset - 1.0) * step)) / step;

        // Interpolate from one preset to the next
        feedback = factor * presets[preset]->feedback + (1 - factor) * presets[preset - 1]->feedback;
        dampening = factor * presets[preset]->dampening + (1 - factor) * presets[preset - 1]->dampening;

        // Switch the exciter preset (no interpolation here, sadly)
        if (factor > 0.5) {
          exciter = presets[preset]->exciter;
        } else {
          exciter = presets[preset - 1]->exciter;
        }
      } else {
        // Throw a warning if the requested move is impossible
        #ifdef DEVMODE
          verbose("Preset index out of range, keeping latest setting.");
        #endif
      }

      #ifdef DEVMODE
        verbose(getPresetInfoString());
      #endif
    }

    #ifdef DEVMODE
      std::string getPresetInfoString() const {
        std::ostringstream oss;
        oss << (1 - factor) * 100 << "% preset " << preset - 1 << ", " << factor * 100 << "% preset " << preset;
        return oss.str();
      }
    #endif
  private:
    Preset* presets[NUM_PRESETS];
    int iterator;
    // 0 - 100
    float dial;

    int preset;
    float factor;
    float step;

    float feedback;
    float dampening;
    int exciter;
};

#endif //KARPLUSSTRONG_PRESET_H
