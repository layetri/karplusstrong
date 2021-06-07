#include "Header/Global.h"

#if !defined(PLATFORM_DARWIN_X86)
  #include <Arduino.h>
  #include <MCP4922.h>
  #include <SPI.h>

  #include "Header/Knob.h"
  #include "Header/KarplusStrong.h"
  #include "Header/Preset.h"

  #define SAMPLERATE 44100

  // Init various classes
  MCP4922 DAC(11,13,10,9);
  KarplusStrong synth(0.9, SAMPLERATE, 1);
  PresetEngine presetEngine;

  // Initialize hardware interfaces
  Knob k_feedback(A9);
  Knob k_preset(A8);

  // Runtime variables
  int saved_note = 0;
  int throttle = 0;
  int in_val = 511;
  int note = 60;

  // Timing variables
  unsigned long old_time = micros();
  unsigned long new_time;
  float interval = 1000000.0f / SAMPLERATE;


  // ============================================
  //  Setup
  // ============================================
  void setup() {
    // Initialize SPI class
    SPI.begin();
    Serial.begin(9600);
    #ifdef DEVMODE

      verbose("KPS starting up!");
      synth.log();
    #endif

    // Setup hardware for IO operations
    pinMode(A7, INPUT);

    // Load presets
    presetEngine.import(0.9995, 6000, 1);
    presetEngine.import(0.8, 2000, 2);
    presetEngine.import(0.999, 1000, 1);
    presetEngine.import(0.850, 7000, 1);
    presetEngine.import(0.999, 10000, 1);
  }


  // ============================================
  //  Main loop
  // ============================================
  void loop() {
    // Set timestamp
    new_time = micros();

    // Wrapper to handle micros() overflow
    if(new_time < old_time) {
      old_time = 0;
    }

    // Check scheduling
    if(new_time - old_time >= interval) {
      // TODO: Implement trigger input
      //  -> for now: use only CV input for both pitch and triggering
      // Read note CV input and check if new value
      // Apply averaging filter to note input for stability
      int input = analogRead(A7);
      if(input - in_val > 4 || input - in_val < -4) {
        in_val = input;
        note = (in_val / 26) + 36;
      }

      if (note != saved_note) {
        Serial.print(in_val);
        Serial.print(": ");
        Serial.println(note);

        // If new value, trigger new note
        synth.pluck(note);
        saved_note = note;
      }

      #ifdef DEVMODE
        if(Serial.available() && throttle == 0) {
          synth.pluck(60);
          verbose("plucked!");
        }
      #endif

      // Run KS-synth master process and shift 16 bit output to 12 bits for DAC
      auto out_val = ((synth.process() + 32768) / 65536.0) * 4096;

      // Write to DAC
      DAC.Set(out_val, out_val);

      // Set new target timestamp
      old_time = new_time;
//    } else {
      // Read IO status on off-cycles
//      synth.setFeedback(k_feedback.getValue());
      presetEngine.set(100 * k_preset.getValue());

      Preset pr = presetEngine.getPreset();
      synth.setFeedback(pr.feedback);
      synth.setDampening(pr.dampening);
      synth.setExciter(pr.exciter);
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

    // Do Jack setup
    JackModule jack;
    jack.init("synth");
    int samplerate = jack.getSamplerate();
    if(samplerate == 0) {
      samplerate = 44100;
    }

    // Setup preset engine and load presets
    PresetEngine presetEngine;
    presetEngine.import(0.9995, 6000, 1);
    presetEngine.import(0.8, 4000, 2);
    presetEngine.import(0.999, 1000, 1);
    presetEngine.import(0.850, 7000, 1);
    presetEngine.import(0.995, 9000, 1);

    // Initialize the synth voices
    int voice_count = 10;
    KarplusStrong* voices[voice_count];
    for(auto& voice : voices) {
      voice = new KarplusStrong(0.9995, samplerate, 1);
    }

    // Assign the Jack callback function
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

    // Start the UI and print a welcome message
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

    // Loop for UI tasks
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

