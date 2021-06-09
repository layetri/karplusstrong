#include "Header/Global.h"

#define FEEDBACK_LONG 0.9995

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
      int input = analogRead(A7);
      if(input - in_val > 4 || input - in_val < -4) {
        in_val = input;
        note = (in_val / 26) + 48;
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
  #include "Header/RtMidi.h"

  #include "Header/KarplusStrong.h"
  #include "Header/Preset.h"

  #include <unistd.h>
  #include <string>
  #include <termios.h>
  #include <cstdio>
  #include <map>
  #include <csignal>

  using namespace std;

  bool done;
  static void finish(int ignore){ done = true; }

  // Using a getch solution from https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
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

  // Abstraction to easily provide a text coloring utility
  string color(string text, int color) {
    return "\033[" + to_string(color) + "m" + text + "\033[0m";
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
    cout << "Welcome to " << color("Tensions", 36) << endl;
    bool running = true;

    // Setup key lookup
    map<char,int> keys;
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


    // Setup RTMidi
    // RTMidi implementation mostly copied from http://www.music.mcgill.ca/~gary/rtmidi/
    RtMidiIn  *midi_in = 0;
    vector<unsigned char> message;
    int nBytes, i;
    double stamp;

    // RtMidiIn constructor
    try {
      midi_in = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
      error.printMessage();
      exit( EXIT_FAILURE );
    }
    // Check inputs.
    unsigned int nPorts = midi_in->getPortCount();
    cout << "\nThere are " << color(to_string(nPorts), 35) << " MIDI input sources available.\n";
    // List inputs
    string portName;
    for ( unsigned int i=0; i<nPorts; i++ ) {
      try {
        portName = midi_in->getPortName(i);
      }
      catch ( RtMidiError &error ) {
        error.printMessage();
        goto cleanup;
      }
      cout << "  Input Port #" << i << ": " << portName << '\n';
    }
    // Prompt user selection
    if(nPorts > 0) {
      cout << "\nChoose a MIDI port to use: ";

      int port;
      cin >> port;
      midi_in->openPort(port);

      cout << "\n\n";

      // Don't ignore sysex, timing, or active sensing messages.
      midi_in->ignoreTypes(false, false, false);

      // Install an interrupt handler function.
      done = false;
      (void) signal(SIGINT, finish);
      // Periodically check input queue.
      cout << "Reading MIDI from port " << port << "... quit with Ctrl-C.\n";
      while (!done) {
        stamp = midi_in->getMessage(&message);
        nBytes = message.size();

        if (nBytes > 0) {
          // Handle note ON messages
          if (message[0] == 144) {
            voiceAllocator(message[1], voice_count, voices);
          }
          if (message[0] == 176) {
            // Handle sustain pedal
            if(message[1] == 64) {
              if (message[2] == 127) {
                for (auto &voice : voices) {
                  voice->setFeedback(FEEDBACK_LONG);
                }
              } else {
                for (auto &voice : voices) {
                  voice->setFeedback(presetEngine.getPreset().feedback);
                }
              }
            }
            // Handle knobs
            if(message[1] == 112) {
              // Preset knob
              presetEngine.set(message[2] / 1.27);
              Preset pr = presetEngine.getPreset();

              for (auto &voice : voices) {
                voice->setFeedback(pr.feedback);
                voice->setDampening(pr.dampening);
                voice->setExciter(pr.exciter);
              }
            }
            // Feedback
            if(message[1] == 74) {
              for (auto &voice : voices) {
                voice->setFeedback(0.9 + ((message[2] / 127.0) * 0.09995));
              }
            }
            // Dampening
            if(message[1] == 71) {
              for (auto &voice : voices) {
                voice->setDampening(1000.0 + (message[2] / 0.0127));
              }
            }
          }
        }

        #if defined(DEVMODE)
          for (i = 0; i < nBytes; i++)
            cout << "Byte " << i << " = " << (int) message[i] << ", ";
          if (nBytes > 0)
            cout << "stamp = " << stamp << endl;
        #endif

        // Sleep for 10 milliseconds.
        usleep(10000);
      }
    } else {
      // Start the UI and print a welcome message
      cout << "No MIDI inputs found, using keyboard input instead.\n" << endl;
      cout << "====================================" << endl;
      cout << "Keymap:" << endl;
      cout << "====================================" << endl;
      cout << "Q: quit" << endl;
      cout << "W/E: increase/decrease feedback" << endl;
      cout << "R/T: increase/decrease dampening" << endl;
      cout << "Y/U: rotate presets left/right" << endl;
      cout << "I: switch excitation modes" << endl;
      cout << "Any other key plays notes" << endl;
      cout << "====================================" << endl;
      cout << "Have fun!" << endl;
      cout << "====================================" << endl;

      // Loop for UI tasks
      while (running) {
        char cmd = getch();

        if (cmd == 'q') {
          running = false;
        } else if (cmd == 'w') {
          // Decrease feedback
          for (auto &voice : voices) {
            voice->decreaseFeedback();
          }
        } else if (cmd == 'e') {
          // Increase feedback
          for (auto &voice : voices) {
            voice->increaseFeedback();
          }
        } else if (cmd == 'r') {
          // Decrease dampening
          for (auto &voice : voices) {
            voice->decreaseDampening();
          }
        } else if (cmd == 't') {
          // Increase dampening
          for (auto &voice : voices) {
            voice->increaseDampening();
          }
        } else if (cmd == 'y') {
          // Decrease preset
          presetEngine.rotate(-1);
          Preset pr = presetEngine.getPreset();

          for (auto &voice : voices) {
            voice->setFeedback(pr.feedback);
            voice->setDampening(pr.dampening);
            voice->setExciter(pr.exciter);
          }
        } else if (cmd == 'u') {
          // Increase preset
          presetEngine.rotate(1);
          Preset pr = presetEngine.getPreset();

          for (auto &voice : voices) {
            voice->setFeedback(pr.feedback);
            voice->setDampening(pr.dampening);
            voice->setExciter(pr.exciter);
          }
        } else if (cmd == 'i') {
          // Cycle exciters
          for (auto &voice : voices) {
            voice->nextMode();
          }
        } else if (cmd != 0) {
          voiceAllocator(keys.find(cmd)->second, voice_count, voices);
        }
      }
    }

    cleanup:
      delete midi_in;
  }
#endif

