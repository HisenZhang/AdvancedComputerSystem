# Project 4

This program implements a dictionary codec.

Features:

- Multi-thread encoding
- Prefix tree index lookup
- SIMD (AVX2) accelerated search

## Code Organization

The code dispatch data parsing to several worker threads in a thread pool [1] to populate the global variables (maps and tree). User may load or save the encoded data & dictionary.

Each query key is mapped to an integer code. Then the program look up this integer in the encoded vector. The lookup is accelerated by loading 8 four-byte integers into 256-bit vector register. The mask from comparison result is the retrieved. If the mask is non zero, find the index of matching. Since chance of occurrence is normally small, implying the mask is likely zero, we expect a performance boost.

The prefix lookup is accelerated by a prefix tree - Trie (modified from [2]). Each node, if is the end of a word, contains the code. By walking the subtree we retrieve codes of all words of the same prefix. Then each code undergo the same SIMD lookup process described above.

## Compile

Tested on Ubuntu 18.04 TLS

```bash
mkdir build
g++ main.cpp trie.cpp -o build/main -O2 -DSIMD -std=c++17 -mavx2 -pthread
```

## Run

```bash
cd build
./main PATH_TO_COLUMN_TXT N_WORKER
```

To use default hardware concurrency, set N_WORKER <= 0.

WARNING: CRLF will break the program. Use LF for newline.

The repo has `abc.txt`, `10k.txt` and `100k.txt` under `data/` for testing. 
Use the raw column data file at the following address for your speed performance evaluation
<https://drive.google.com/file/d/192_suEsMxGInZbob_oJJt-SlKqa4gehh>

The program runs interactively. Supported operations:

```text
===MENU===
0. Exit
1. Encode
2. Save
3. Load
4. Query
5. Prefix Query
```

## Example

For input file `abc.txt`:

```text
./main "../data/abc.txt"

> 1
Encoding completed. Input: 9 entries. Encoded: 7 entries.

> 2
Encoded data saved.
Encoded dictionary saved.

> 4abc  // find data "abc": 3 occurrence at index 0,3,6
0:abc @ 0 3 6 

> 4def  // find data "def": 1 occurrence at index 7
5:def @ 7

> 5a    // find data with prefix "a"
0:abc @ 0 3 6 
3:abcdef @ 4
1:and @ 1
2:ant @ 2
4:antenna @ 5

> 5abc  // find data with prefix "abc"
0:abc @ 0 3 6 
3:abcdef @ 4

> 5aa   // no such prefix
```

## References

- [1] Thread pool: <https://github.com/vit-vit/ctpl>
- [2] Prefix tree: <https://www.geeksforgeeks.org/trie-insert-and-search/>
- [3] SIMD: <https://medium.com/swlh/searching-gigabytes-of-data-per-second-with-simd-f8ab111a5f9c>