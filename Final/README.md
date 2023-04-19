# SIMD Enabled DSP Library

Hisen Zhang and Wyatt Marvil

This project contains several DSP functions that take advantage of SIMD to speed up computing.

Main Window             |  Waveform Generators
:-------------------------:|:-------------------------:
![MainWindow](https://github.com/HisenZhang/AdvancedComputerSystem/blob/main/Final/images/MainWindow.png)  |  ![GeneratorWindow](https://github.com/HisenZhang/AdvancedComputerSystem/blob/main/Final/images/GeneratorWindow.png)

### Features
- Load WAV file
- Generate simple waveforms
- Plot waveforms
- Layer audio filters (1D SIMD convolutions with SIMD on its own thread)
- Audio playback
- Write results to disk
- Graphical user interface

### Filters
- Delay
- Derivative
- ...

### Setup

Install dependencies (you may need more than these packages):

```sudo apt install gtk2.0 libglfw3 libglfw3-dev```

```mkdir build && cd build && cmake ..```

Now, run make or open the generated VisualStudio solution to select the ```DSP``` build target.
