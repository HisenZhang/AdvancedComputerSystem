# Project 2

Matrix-Matrix Multiplication with SIMD Instructions & Cache Miss Minimization

## Compile

```bash
g++ mixed.cpp -o build/mixed -mavx
```

## Run

```bash
./build/mixed 
```

## References

SIDM Brief Intro (1): https://www.it.uu.se/edu/course/homepage/hpb/vt12/lab4.pdf
SIDM Brief Intro (2): https://blog.qiqitori.com/2018/05/matrix-multiplication-using-simd-instructions/
High Performance Matrix Multiplication: https://gist.github.com/nadavrot/5b35d44e8ba3dd718e595e40184d03f0
Tutorial: https://www.intel.com/content/www/us/en/developer/articles/technical/using-intel-streaming-simd-extensions-and-intel-integrated-performance-primitives-to-accelerate-algorithms.html?wapkw=simd