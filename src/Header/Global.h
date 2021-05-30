//
// Created by Daniël Kamp on 30/05/2021.
//

#ifndef KARPLUSSTRONG_GLOBAL_H
#define KARPLUSSTRONG_GLOBAL_H

//#define DEVMODE
// Set platform to Darwin x86 (macOS)
#define PLATFORM_DARWIN_X86

// Make sure that there's only one platform at once
#if !defined(PLATFORM_DARWIN_X86)
  // Set platform to Arduino
  //#define PLATFORM_ARDUINO_ARM

  // Set platform to Teensy 4.0 (IMX RT 1062)
  //#define PLATFORM_TEENSY40
#endif

#endif //KARPLUSSTRONG_GLOBAL_H
