EMBEDDED COMPRESSION LIBRARY
===

**ECL** aka EMBEDDED COMPRESSION LIBRARY is NOT ONLY for embedded, it is **mostly oriented for small data** and has special optimized **low-memory** modes for restricted environments.

---
### Language: C
### Platforms: any
### Endianness: any
### Library version: 1.0.3
---
### Tested on
- Windows 7: msvc2013, msvc2015, gcc 4.8, gcc 7.2
- Mac OS 10.12: clang (Apple LLVM version 8.0.0)
- Embedded ARM Cortex-M3: armcc 5.06
---


## COMPRESSORS
Some of modes of some compressors use intermediate buffers for compression, they don't use any implicit allocation (unless otherwise specified) - user can easily choose how to allocate buffers.
Every compression method that uses temporary buffer (say, more than 10 bytes) - has it specified in documentation near method declaration.

### ECL:NanoLZ - meticulously formatted version of traditional LZ77 algorithm.
- use cases - various, same with other pure LZ algorithms;
- takes advantage of repeated sequences with length of 2 bytes;
- provides API for adjusting compressed data format (see "schemes") to gain better compression ratio for user's datasets (for advanced users);
- can be beneficial for very small amounts of data (e.g. to fit some data into single Bluetooth Low Energy packet);
- compression ratio - middle..high;
- compression ratio limit - roughly infinite;
- compressors complexity - linear..cubic;
- compressors performance - low..high, provides different modes, configurable compression/performance trade off;
- compressors performance for small data payloads - optimized;
- decompressor complexity - linear;
- decompressor performance - middle..high;
- compressors buffer memory consumption - from zero to any, has different modes using 0, 256, 512,.. bytes for temporary buffers;
- decompressor buffer memory consumption - zero;
- static const memory consumption - low;
- stack consumption - normal;
- (TBD if needed) stream modes for processing data by chunks;

### ECL:ZeroDevourer - a diff-oriented compressor that takes advantage of zero bytes (even single ones) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - roughly, linearly depends on percentage of zeroes in your data, for target use cases - high;
- compression ratio limit - roughly infinite;
- compressor complexity - linear;
- compressor performance - high (up to gigabyte per second);
- decompressor complexity - linear;
- decompressor performance - high (up to several gigabytes per second);
- compressor buffer memory consumption - zero;
- decompressor buffer memory consumption - zero;
- static const memory consumption - 10 bytes;
- stack consumption - low;
- binary code size - small;

### ECL:ZeroEater - a diff-oriented compressor that takes advantage of zero bytes (starting from two bytes in a row) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - similar to `ZeroDevourer` but worse, for target use cases - high;
- compression ratio limit - 64;
- compressor has dry-run mode;
- compressor complexity - linear;
- compressor performance - very high (up to gigabyte per second);
- decompressor complexity - linear;
- decompressor performance - very high (up to several gigabytes per second);
- compressor buffer memory consumption - zero;
- decompressor buffer memory consumption - zero;
- static const memory consumption - zero;
- stack consumption - lowest;
- binary code size - minimum;
- doesn't require "ECL_common.c" file to be compiled;


## FORMATS
Formats of compressors are described in "formats/" dir, there are common features shared between compressors (except `ZeroEater`, which is simple and independent):
- "formats/ECL_JH.txt" - format of Jumping Header, main feature and core of the library;
- "formats/ECL_E_number_format.txt" - format of 'Extensible' numbers;


## EXTRA CONFIGURING
See **ECL_config.h** for details on configuring, mostly controlled by ECL_USE* macros.
- you can explicitly specify bitness of length variables on your consideration (`ECL_USE_BITNESS_16` / 32 / 64 macro), default is 32;
- you can enable/disable branchless optimizations for your consideration (currently inefficient) - see `ECL_USE_BRANCHLESS`;
- you can enable/disable internal asserts (work if system assert works e.g. no `NDEBUG` macro specified) - see `ECL_USE_ASSERT`;
- you can disable malloc/free in case they cause compilation errors on some restricted platforms - see `ECL_DISABLE_MALLOC`;
- you can disable/exclude memory-demanding functions by defining ECL_EXCLUDE_HIMEM (useful for 16bit compilers, e.g. arduino environment) to fix some warnings;
- you can allow all `NanoLZ` schemes or only specific one - to let compiler inline more for better performance - see `ECL_NANO_LZ_ONLY_SCHEME`;
- to use ECL as dynamic library - uncomment define for `ECL_USE_DLL`, to build dynamic library - define also `ECL_DLL_EXPORT`;
- in case you don't have uint*_t types defined in `stdint.h` - define those types there near "user setup part";


## PERFORMANCE BENCHMARKS
PC benchmarks are performed for *Intel core i5-3570k @ 3.4 GHz / Windows 7 64 bit / 16gb RAM 1600 MHz*.
All benchmarks are performed for ECL version 1.0.0.
Compiled with GCC 7.2.0, options: `-m32 -Wall -Wextra -pedantic -O3`.
- ECL sources are compiled as single file: "ecl-all-c-included/ECL_all_c_included.c" (and for Embedded benchmarks too);
- Speed is in megabytes per second (mb/s);
- For `NanoLZ` used Scheme1 unless otherwise specified;
- ECL is built for 32 bits (`ECL_USE_BITNESS_32`, default option) unless otherwise specified. In most cases 32bit build is the most efficient;
- Compressor parameter (for LZ4_HC - *compressionLevel*, for `NanoLZ` - *search_limit*) is further referenced as *Param*;
- *Ratio* is size of compressed data comparing to size of original data, e.g. compressing 1000 bytes -> 100 bytes corresponds to ratio 0.1.

Benchmarking small datasets (PC and Embedded environment)
---
Used samples around 2 kb each. Run in big external cycle:
- main comparison - `NanoLZ` versus [LZ4] (which can also be configured for low-memory);
- for PC: `NanoLZ` demo scheme (Scheme2) - to show that you can achieve different parameters within `NanoLZ`, if needed;
- for PC: [LZ4] in high-compression mode (LZ4_HC) with big memory buffers used;
- for PC: `NanoLZ` with bigger memory buffers used (mid2min, fast1/fast2 using *window_size_bits=11* - further referenced as *Window*) for detailed codec comparison on small data.

### PC environment

| Compressor                                    | Param |  Ratio  | Compression  | Decompression | Compressor memory             |
| --------------------------------------------- | ----- | ------- | ------------ | ------------- | ----------------------------- |
| LZ4_compress_default (v1.8.1)                 |       |**0.548**|  **625 mb/s**|    2020 mb/s  | **256 bytes**                 |
| **ECL_NanoLZ_Compress_mid1min**               |       |**0.465**|  **249 mb/s**|     335 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |   2   |  0.427  |    114 mb/s  |     332 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |   3   |  0.413  |     81 mb/s  |     332 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |   4   |  0.4    |     65 mb/s  |     332 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |   5   |  0.397  |     56 mb/s  |     337 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |  10   |  0.387  |     39 mb/s  |     344 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1                      |  50   |**0.377**|     24 mb/s  |     355 mb/s  | **256 bytes**                 |
| ECL_NanoLZ_Compress_mid1min **Scheme2 (demo)**|       |  0.592  |    228 mb/s  |   **890 mb/s**| **256 bytes**                 |
|                                               |       |         |              |               |                               |
| **ECL_NanoLZ_Compress_mid2min**               |       |**0.465**|  **347 mb/s**|     334 mb/s  | **513 bytes**                 |
| ECL_NanoLZ_Compress_fast1                     |  20   |  0.383  |     60 mb/s  |     340 mb/s  | 4612 bytes (**16bit build**)  |
| ECL_NanoLZ_Compress_fast1                     |  20   |  0.383  |     66 mb/s  |     342 mb/s  | 9224 bytes                    |
| **ECL_NanoLZ_Compress_fast2**                 |  20   |  0.377  |   **93 mb/s**|     343 mb/s  | 135172 bytes (**16bit build**)|
| **ECL_NanoLZ_Compress_fast2**                 |  20   |  0.377  |   **79 mb/s**|     350 mb/s  | 270344 bytes                  |
| ECL_NanoLZ_Compress_fast2                     | 100   |**0.37** |     54 mb/s  |     350 mb/s  | 135172 bytes (**16bit build**)|
| ECL_NanoLZ_Compress_fast2                     | 100   |**0.37** |     50 mb/s  |     360 mb/s  | 270344 bytes                  |
|                                               |       |         |              |               |                               |
| LZ4_compress_HC (v1.8.1)                      |   3   |**0.49** |     87 mb/s  |    2230 mb/s  | 384 kb                        |
| LZ4_compress_HC (v1.8.1)                      |   9   |**0.49** |     59 mb/s  |    2230 mb/s  | 384 kb                        |
| LZ4_compress_HC (v1.8.1)                      |  11   |**0.49** |     16 mb/s  |    2230 mb/s  | 384 kb                        |
| LZ4_compress_HC (v1.8.1)                      |  12   |**0.49** |     10 mb/s  |    2230 mb/s  | 384 kb                        |

On some other small datasets (highly compressible) `NanoLZ`:Scheme1 decompression speed exceeded **1000 mb/s**,
while compression speed of ECL_NanoLZ_Compress_mid2min reached **570 mb/s**.

### Embedded environment
- hardware: ARM Cortex-M3 120 MHz;
- compiler: armcc 5.06;
- optimization options: `-O3`.

| Compressor                      | Param |  Ratio  | Compression  | Decompression | Compressor memory |
| ------------------------------- | ----- | ------- | ------------ | ------------- | ----------------- |
| LZ4_compress_default (v1.8.0)   |       |**0.53** |**1.785 mb/s**|   10.12 mb/s  | **1024 bytes**    |
| **ECL_NanoLZ_Compress_mid2min** |       |**0.465**|**1.822 mb/s**|    2.42 mb/s  | **513 bytes**     |
| ECL_NanoLZ_Compress_mid2        |   2   |  0.427  |  0.865 mb/s  |    2.42 mb/s  | **513 bytes**     |
| ECL_NanoLZ_Compress_mid2        |   3   |  0.413  |  0.718 mb/s  |    2.46 mb/s  | **513 bytes**     |
| ECL_NanoLZ_Compress_mid2        |   4   |  0.4    |  0.636 mb/s  |    2.48 mb/s  | **513 bytes**     |
| ECL_NanoLZ_Compress_mid2        |   5   |  0.397  |  0.584 mb/s  |    2.54 mb/s  | **513 bytes**     |
| ECL_NanoLZ_Compress_mid2        |  10   |  0.387  |  0.455 mb/s  |    2.57 mb/s  | **513 bytes**     |

[LZ4]: http://www.lz4.org/

Benchmarking large datasets (only PC environment)
---
Big datasets used here to show performance measured on big data without wrapping in external cycle, they also show that `NanoLZ` is inappropriate choice for big data.
On my measurements compression ratio of [LZ4] wins on data bigger than around 25 kb (this bound isn't accurately measured and appropriate statistic isn't provided).
Though, again, **with custom scheme** you are able to achieve different characteristics.

#### Benchmarks for data samples from [Silesia Corpus] (further referenced as *Silesia*): 12 files of different types, sizes range in 6..51 mb:

| Compressor                | Param | Window | Compression    | Decompression   | Compressor memory |
| ------------------------- | ----- | ------ | -------------- | --------------- | ----------------- |
| ECL_NanoLZ_Compress_fast2 |    1  |   16   |  57..158 mb/s  |  122..310 mb/s  | 512kb             |
| ECL_NanoLZ_Compress_fast2 |   10  |   16   |  25..98 mb/s   |  132..435 mb/s  | 512kb             |
| ECL_NanoLZ_Compress_fast2 |  100  |   16   |  8..31 mb/s    |  164..569 mb/s  | 512kb             |
| ECL_NanoLZ_Compress_fast2 |   10  |   18   |  22..97 mb/s   |  135..436 mb/s  | 1.3mb             |
| ECL_NanoLZ_Compress_fast2 |  100  |   18   |  5..23 mb/s    |  161..586 mb/s  | 1.3mb             |
|                           |       |        |                |                 |                   |
| ECL_ZeroEater_Compress    |       |        | 518..1446 mb/s | 1353..7000 mb/s | 0                 |
| ECL_ZeroDevourer_Compress |       |        | 341..1456 mb/s | 739..10317 mb/s | 0                 |

Note that prodigious decompression speed of `ZeroEater` and `ZeroDevourer` is achieved on files they cannot compress, see next table for accumulated statistic.

#### Benchmarks for single *Silesia*.tar file (202 mb):

| Compressor                    | Param | Window |  Ratio  | Compression | Decompression | Compressor memory |
| -------------------------     | ----- | ------ | ------- | ----------- | ------------- | ----------------- |
| memcpy                        |       |        |  1.000  |  6300 mb/s  |  6300 mb/s    | 0                 |
| LZ4_compress_default (v1.8.1) |       |        |  0.479  |  437 mb/s   |  1543 mb/s    | 16 kb             |
| LZ4_compress_HC (v1.8.1)      |    3  |        |  0.383  |  74.5 mb/s  |  1704 mb/s    | 384 kb            |
| LZ4_compress_HC (v1.8.1)      |    9  |        |**0.367**|  26.9 mb/s  |  1783 mb/s    | 384 kb            |
|                               |       |        |         |             |               |                   |
| ECL_NanoLZ_Compress_fast2     |    1  |   16   |  0.473  |  92.5 mb/s  |  192.7 mb/s   | 512 kb            |
| ECL_NanoLZ_Compress_fast2     |   10  |   16   |  0.407  |  47.5 mb/s  |  221.5 mb/s   | 512 kb            |
| ECL_NanoLZ_Compress_fast2     |  100  |   16   |  0.39   |  15.3 mb/s  |  250 mb/s     | 512 kb            |
| ECL_NanoLZ_Compress_fast2     |   10  |   18   |  0.405  |  42.7 mb/s  |  222 mb/s     | 1.3 mb            |
| ECL_NanoLZ_Compress_fast2     |  100  |   18   |**0.385**|  10.5 mb/s  |  254 mb/s     | 1.3 mb            |
|                               |       |        |         |             |               |                   |
| ECL_ZeroEater_Compress dry    |       |        |  0.948  | 1136 mb/s   |               | 0                 |
| ECL_ZeroEater_Compress        |       |        |  0.948  |**925 mb/s** |**2940 mb/s**  | 0                 |
| ECL_ZeroDevourer_Compress     |       |        |  0.935  |  696 mb/s   |  1905 mb/s    | 0                 |

[Silesia Corpus]: http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia


## MULTITHREADING
Codecs don't share any non-const data, so API is thread safe.


## SAMPLE TESTING PROGRAM
There's sample program to compress/decompress in `NanoLZ`:Scheme1 format via command line: see **"sample/"** dir.
It's enough to compile single **"sample/sample.cpp"** file, some building scripts are provided in same dir.
Program is unable to compress too large files.


## USAGE
In general usage is pretty straightforward - you call \*Compress method, you call \*Decompress method - that's it.
- usage samples present in headers of each codec;
- see **"sample/sample.cpp"** example program to encode/decode files;
- you can find examples in tests located in **"tests/"** dir.


## BUILDING
It's enough to build single **"ecl-all-c-included/ECL_all_c_included.c"** file, so do it unless you have reasons to compile minimum amount of code.
If you need to minimize amount of code to compile, then: to have compressor "FOO" available - include "ECL_FOO.h", compile "ECL_common.c" + "ECL_FOO.c".

If you need a static/dynamic library instead of adding ECL sources to your project - you will have to compile it yourself (no such scripts here). To build dynamic library define `ECL_USE_DLL` and `ECL_DLL_EXPORT` macros, to import it - define only `ECL_USE_DLL` macro.

Note that building as C rather than C++ results in higher performance due to absence of code generated for exception handling, so simple #including code into some of your C++ source files is not the most efficient option.


## TESTS
- unit tests are in **"tests/"** dir, can be built and launched with scripts in same place. Basically single **"tests/tests.cpp"** file is enough to be compiled;
- scripts in **"tests/"** dir build and run ECL consequently for 16, 32 and 64 bits (bitness of ECL_usize);
- executable test file has optional "depth" parameter on launch: a number between -10 and 1000 (e.g. "tests.exe 50"). It determines tests coverage and time spent on tests (-10 for fast run, 1000 for very deep/slow run);
- sources of tests can be easily embedded into another project and run any amount of times with any parameters from there, see **"tests/tests.cpp"**.
