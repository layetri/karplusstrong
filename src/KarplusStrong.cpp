//
// Created by Daniël Kamp on 30/05/2021.
//

#include "Header/KarplusStrong.h"

KarplusStrong::KarplusStrong(float init_feedback, int init_samplerate, int exciter) {
#if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
  verbose("Start init KPS");
#endif
  feedback = init_feedback;
  samplerate = init_samplerate;

  buffer = new Buffer(samplerate);
  delayLine = new DelayLine(10, feedback, samplerate, buffer);
  delayLine->setFilterFrequency(6000.0);

  busy = false;
  remaining_trigger_time = 0;

  this->exciter = exciter;

  srand(678438625);

  setDelayTime(440);

#if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
  verbose("Done init KPS");
#endif
}

KarplusStrong::~KarplusStrong() {
  delete buffer;
  delete delayLine;
}

void KarplusStrong::pluck(int note) {
  // Send impulse on delay line
  // TODO: look at different types of exciters
  //  -> Sine/Square/Saw burst, with envelope
  //  -> enveloped input signal
  //  -> ...

  // TODO: Add audio input as exciter
  //  -> v/oct cv for pitch
  //  -> trigger for pluck
  //  -> audio input buffer as exciter

  // TODO: Add audio input trigger
  //  -> Audio input triggers pluck and serves as exciter at the same time

  setDelayTime(mtof(note));

  // Excite
  // Possible exciters:
  //  0: noise
  //  1: sine
  //  2: pulse

  if(exciter == 0) {
    for (int i = 0; i < 5; i++) {
      #ifdef PLATFORM_DARWIN_X86
        static std::default_random_engine e;
        std::uniform_real_distribution<> dist(-32768, 32768);
        auto smp = (int16_t) dist(e);
      #elif defined(PLATFORM_TEENSY40)
        auto smp = (int16_t) random(-32768, 32768);
      #endif

      buffer->writeAhead(smp, i);
    }
    remaining_trigger_time = 5;
  } else if(exciter == 1) {
    // Do sine excitation
    float phase_step = 440.0 / samplerate;
    float phase = 0.0;

    for(int i = 0; i < 10; i++) {
      int16_t smp = M_PI * 2 * phase * 32768;
      smp = (buffer->readAhead(i - 1) + smp) * 0.5;
      buffer->writeAhead(smp, i);

      phase += phase_step;
      if(phase > 1.0) phase -= 1.0;
    }
    remaining_trigger_time = 10;
  } else if(exciter == 2) {
    for(int i = 0; i < 10; i++) {
      int16_t smp;
      if(i % 2) {
        smp = 32767;
      } else {
        smp = -32767;
      }
      buffer->writeAhead(smp, i);
    }
  }
}

int16_t KarplusStrong::process() {
  // Do the actual delay line processing
  int16_t smp = delayLine->process();

  // Push the DL and buffer forward
  delayLine->tick();
  buffer->tick();

  // Write a 0 for the next sample (if a burst isn't in progress)
  if(remaining_trigger_time > 0) {
    remaining_trigger_time--;
  } else {
    buffer->write(0);
  }

  busy = smp != 0;

  // Return the value
  return smp;
}

float KarplusStrong::mtof(int note) {
  // fm  =  2(m−69)/12(440 Hz)
  return (float) (pow(2, (note-69) / 12.0) * 440.0);
}

void KarplusStrong::setDelayTime(float frequency) {
  #if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
    verbose("set delay time");
  #endif
  delayLine->setDelayTime((int) (samplerate / frequency));
}

void KarplusStrong::setFeedback(float n_feedback) {
  feedback = n_feedback;
  delayLine->setFeedback(feedback);
}

void KarplusStrong::log() {
  #if !defined(PLATFORM_DARWIN_X86) && defined(DEVMODE)
    Serial.print("feedback: ");
    Serial.println(feedback);

    Serial.print("delaytime: ");
    Serial.println(delayTime);

    Serial.print("buffer: ");
    Serial.println(reinterpret_cast<int>(&buffer));
  #endif
}

bool KarplusStrong::available() {
  return !busy;
}

void KarplusStrong::setDampening(float dampening) {
  if(dampening > 0) {
    delayLine->setFilterFrequency(dampening);
  }
}

void KarplusStrong::setExciter(int exciter) {
  this->exciter = exciter;
}

#ifdef PLATFORM_DARWIN_X86
  void KarplusStrong::increaseFeedback() {
    if(feedback < 0.995) {
      setFeedback(feedback + 0.005);
    }
  }
  void KarplusStrong::decreaseFeedback() {
    if(feedback > 0.005) {
      setFeedback(feedback - 0.005);
    }
  }

  void KarplusStrong::increaseDampening() {
    if(delayLine->getFilterFrequency() > 10) {
      delayLine->setFilterFrequency(delayLine->getFilterFrequency() - 100);
    }
  }
  void KarplusStrong::decreaseDampening() {
    if(delayLine->getFilterFrequency() < 10000) {
      delayLine->setFilterFrequency(delayLine->getFilterFrequency() + 100);
    }
  }

  void KarplusStrong::nextMode() {
    if(exciter < 2) {
      exciter++;
    } else {
      exciter = 0;
    }
  }
#endif