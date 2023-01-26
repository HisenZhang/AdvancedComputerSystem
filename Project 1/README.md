# Project 1

A multi-thread streaming compression demo using zstd library.

The streamCompress function calls ZSTD_compressStream2 to handle the underlaying multithreading mechanism.

ZSTD_compressStream2 is designed to work with streaming and multithreading situation.

## Compile

```bash
g++ main.c -o build/a.out -O3 -lzstd -lpthread -fpermissive
```

## Run

```bash
build/a.out NUM_THREADS COMPRESSION_LEVEL FILENAME
```

## References

Documentation: http://facebook.github.io/zstd/zstd_manual.html

Example: https://github.com/facebook/zstd/blob/dev/examples/streaming_compression.c