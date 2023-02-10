# Project 2

Matrix-Matrix Multiplication with SIMD Instructions & Cache Miss Minimization

To minimized cache miss, matrix tiling is used to sub divide the matrix into small blocks for more computation done per memory access with caching. When the block size is proper, data are aligned and stay in cache for multiple calculations. Loading these cached data into AVX vectorized registers speed up things even more (SIMD).

Block size BS is set to CACHELINE / sizeof(int16_t) for fixed-point, or CACHELINE / sizeof(float) for floating-point, where CACHELINE is typically one of 32, 64, or 128 bytes. The AVX instruction can pack 128 bits of 16-bit aligned operands at a time, so make sure block size is at least 4.

For best performance, the matrix size N is determined at compile time. To change N, modify source code and recompile.

Special note for fixed-point: we use 16-bit integer to represent 16-bit fixed-point numbers. `_mm_mulhrs_epi16` handles 16-bit fixed-point multiplication.

## Compile

For 32 bit floating-point:

```bash
g++ mixed.cpp -o build/mixed -mavx
```

For 16 bit fixed-point:

```bash
g++ mixed_fixed.cpp -o build/mixed_fixed -mavx
```

## Run

For 32 bit floating-point:

```bash
./build/mixed 
```

For 16 bit fixed-point:

```bash
./build/mixed_fixed 
```

## Output

- Naive - The reference
- Mixed - SIMD and Tiling enabled
- SIMD  - SIMD enabled using AVX
- Tiled - Tiling enabled for optimized memory access pattern

## References

SIDM Brief Intro (1): <https://www.it.uu.se/edu/course/homepage/hpb/vt12/lab4.pdf>
SIDM Brief Intro (2): <https://blog.qiqitori.com/2018/05/matrix-multiplication-using-simd-instructions/>
High Performance Matrix Multiplication: <https://gist.github.com/nadavrot/5b35d44e8ba3dd718e595e40184d03f0>
Tutorial: <https://www.intel.com/content/www/us/en/developer/articles/technical/using-intel-streaming-simd-extensions-and-intel-integrated-performance-primitives-to-accelerate-algorithms.html?wapkw=simd>
