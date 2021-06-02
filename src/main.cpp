#include "Header/Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
  #include <MCP4922.h>
  #include "Header/Knob.h"
  #include "Header/KarplusStrong.h"

  #define SAMPLERATE 22000
  #define DEVMODE

  // Init various classes
  MCP4922 DAC(11,13,10,9);
  KarplusStrong synth(0.9, SAMPLERATE);

  Knob k_feedback(A9);

  // Runtime variables
  int saved_note = 0;

  // T iming variables
  unsigned long old_time = micros();
  unsigned long new_time;
  long interval = 1000000 / SAMPLERATE;

  void setup() {
    // Setup hardware for IO operations
    pinMode(A0, INPUT);
    pinMode(A9, INPUT);

    #ifdef DEVMODE
      Serial.begin(9600);
      Serial.println("KPS starting up!");
      synth.log();
    #endif
  }

  void loop() {
    // Set timestamp
    new_time = micros();

    // Check scheduling
    if(new_time > old_time + interval) {
      // Read note CV input and check if new value
      int note = (analogRead(A0) / 26) + 24;
      if (note != saved_note) {
        // If new value, trigger new note
        synth.pluck(note);
      }

      #ifdef DEVMODE
        if(Serial.available()) {
          synth.pluck(60);
          Serial.println("plucked!");
        }
      #endif

      // Run KS-synth master process
      auto val = synth.process();
      // Shift 16 bit output to 12 bits for DAC
      int out_val = val << 4;

      // Write to DAC
      DAC.Set(out_val, out_val);

      // Set new target timestamp
      old_time = new_time;
    } else {
      // Read IO status on off-cycles
      synth.setFeedback(k_feedback.getValue());
    }
  }
#elif defined(PLATFORM_DARWIN_X86)
  #include <iostream>
  #include <chrono>
  #include "Header/jack_module.h"

  #include "Header/KarplusStrong.h"
  #include "Header/Preset.h"

  #include <unistd.h>
  #include <termios.h>
  #include <cstdio>
  #include <map>

  char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
      perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
      perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
      perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
      perror ("tcsetattr ~ICANON");
    return (buf);
  }

  void voiceAllocator(int note, int voice_count, KarplusStrong* voices[]) {
    for(int i = 0; i < voice_count; i++) {
      if(voices[i]->available()) {
        #ifdef DEVMODE
          verbose("Found a voice!");
          verbose(i);
        #endif
        voices[i]->pluck(note);
        return;
      }
    }
    voices[0]->pluck(note);
  }

  int main() {
    // Setup key lookup
    std::map<char,int> keys;
    keys['z'] = 36;
    keys['x'] = 38;
    keys['c'] = 40;
    keys['v'] = 41;
    keys['b'] = 43;
    keys['n'] = 45;
    keys['m'] = 47;
    keys['a'] = 48;
    keys['s'] = 50;
    keys['d'] = 52;
    keys['f'] = 53;
    keys['g'] = 55;
    keys['h'] = 57;
    keys['j'] = 59;
    keys['k'] = 60;

    JackModule jack;
    jack.init("synth");
    int samplerate = jack.getSamplerate();
    if(samplerate == 0) {
      samplerate = 44100;
    }

    PresetEngine presetEngine;
    presetEngine.import(0.9995, 6000, 1);
    presetEngine.import(0.8, 2000, 2);
    presetEngine.import(0.999, 1000, 1);
    presetEngine.import(0.850, 7000, 1);
    presetEngine.import(0.999, 10000, 1);

    int voice_count = 10;
    KarplusStrong* voices[voice_count];
    for(auto& voice : voices) {
      voice = new KarplusStrong(0.995, samplerate, 1);
    }

    //assign a function to the JackModule::onProcess
    jack.onProcess = [&voices](jack_default_audio_sample_t *inBuf,
                              jack_default_audio_sample_t *outBuf, jack_nframes_t nframes) {
        for(unsigned int i = 0; i < nframes; i++) {
          float smp = 0.0;
          for(auto& voice : voices) {
            smp += (voice->process() / 32768.0);
          }

          if(smp > 1.0) {
            smp = 1.0;
          } else if (smp < -1.0) {
            smp = -1.0;
          }

          outBuf[i] = smp;
        }
        return 0;
    };

    jack.autoConnect();

    bool running = true;
    std::cout << "====================================" << std::endl;
    std::cout << "Keymap:" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Q: quit" << std::endl;
    std::cout << "W/E: increase/decrease feedback" << std::endl;
    std::cout << "R/T: increase/decrease dampening" << std::endl;
    std::cout << "Y/U: rotate presets left/right" << std::endl;
    std::cout << "I: switch excitation modes" << std::endl;
    std::cout << "Any other key plays notes" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Have fun!" << std::endl;
    std::cout << "====================================" << std::endl;

    while (running) {
      char cmd = getch();

      if(cmd == 'q') {
        running = false;
      } else if(cmd == 'w') {
        // Decrease feedback
        for(auto& voice : voices) {
          voice->decreaseFeedback();
        }
      } else if(cmd == 'e') {
        // Increase feedback
        for(auto& voice : voices) {
          voice->increaseFeedback();
        }
      } else if(cmd == 'r') {
        // Decrease dampening
        for(auto& voice : voices) {
          voice->decreaseDampening();
        }
      } else if(cmd == 't') {
        // Increase dampening
        for(auto& voice : voices) {
          voice->increaseDampening();
        }
      } else if(cmd == 'y') {
        // Decrease preset
        presetEngine.rotate(-1);
        Preset pr = presetEngine.getPreset();

        for(auto& voice : voices) {
          voice->setFeedback(pr.feedback);
          voice->setDampening(pr.dampening);
          voice->setExciter(pr.exciter);
        }
      } else if(cmd == 'u') {
        // Increase preset
        presetEngine.rotate(1);
        Preset pr = presetEngine.getPreset();

        for(auto& voice : voices) {
          voice->setFeedback(pr.feedback);
          voice->setDampening(pr.dampening);
          voice->setExciter(pr.exciter);
        }
      } else if(cmd == 'i') {
        // Cycle exciters
        for(auto& voice : voices) {
          voice->nextMode();
        }
      } else if(cmd != 0) {
        voiceAllocator(keys.find(cmd)->second, voice_count, voices);
      }
    }
  }
#endif

