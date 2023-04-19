# SIMD-accelerated DSP Library

Hisen & Wyatt

This project contains several DSP functions that take advantage of SIMD to speed up computing. 

Features:
- Load wav file
- Waveform visualization
- Generate simple wave forms
- Apply filters (AVX enabled, on its own thread)
- Audio playback
- Write wav file

Filters Available:
- Delay
- Derivative
- ...

## Setup

```sudo apt update```

```sudo apt install gtk2.0 libglfw3 libglfw3-dev```

## Usage

```mkdir build && cd build```

```cmake ..``

Now, run make or open the generated VisualStudio solution to build the ```DSP``` target. 