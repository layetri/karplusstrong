## Adventures in String Synthesis
This repository contains my experimental work for the KO2D course, where I experiment with physical modeling through Karplus-Strong string synthesis.

The project can be built for Teensy 4.0 (using PlatformIO, sort of) and x86 (using Jack, tested for macOS Big Sur). To switch platforms, simply uncomment the platform of your choice in `src/Header/Global.h`. This will toggle platform-specific code throughout the project.

### What I did so far
#### Week 1 - 3
Experiments with Karplus Strong in MaxMSP. What can I do to make a sound more expressive/easily controllable/etc.?
#### Week 4
Break because vaccine.
#### Week 5
Started implementation in C++. Debugging on Teensy is (kind of) hell, so I'm also writing an implementation using Jack that can run on macOS natively. Yay for cross-platform compatibility ✨.