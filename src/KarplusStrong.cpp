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

  ex_interface = new ExcitationInterface(samplerate, buffer);

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

  int length = 10;
  std::string exciters[3] = {"noise", "sine", "impulse"};

  // Handle excitation via an interface struct
  ex_interface->excite(exciters[exciter], length);
  remaining_trigger_time = length;
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