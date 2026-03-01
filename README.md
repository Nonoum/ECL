EMBEDDED COMPRESSION LIBRARY
===

**ECL** aka EMBEDDED COMPRESSION LIBRARY is NOT ONLY for embedded, it is **mostly oriented towards small data** and has optimized **low-memory** modes/algorithms for restricted environments, that also makes it highload-friendly.

---
### Language: C
### Platforms: any
### Endianness: any
### Library version: 1.2.0
---
## NEWS

### [PINNED] 11 July 2025: Announcing new functionality development, adding Patreon page for support
- new type low-memory codec: TBA **[released Huff8 @ 30 September 2025]**
- new type specialized low-memory codec (2): TBA **[released SLA @ 01 March 2026]**
- new type specialized low-memory codec (3): TBA
- new type codec (4): TBA
- Patreon page: https://www.patreon.com/Nonoum

### 01 March 2026: Releasing SLA (Sequence Levels Analyzer) specialized codec. ECL 1.2.0
- See description and benchmarks in sections with "ECL:SLA" below
- Simple, fast, very low-memory
- The compressor (with aux conversion functions) targets chart data (e.g. integer array of sensor readings, audio samples etc)
- Can be used for lossless audio data/stream compression (also applicable for very small independent data chunks) - alternative to codecs like "flac"
- Can be used for lossless graphic image compression (TBD: Ultra Low Memory 2d image codec - PNG alternative)

### 30 September 2025: Releasing Huff8 (Huffman 8-bit) Ultra Low Memory codec. ECL 1.1.0
- See description and benchmarks in sections with "ECL:Huff8" below
- Fully Recursion-Free and predictable memory use
- Compression with just 1.8kb extra RAM (or 1.5kb options)
- Decompression with just 1kb extra RAM (or less for some cases, if needed)
- Fast algorithms, for typical small data case even sorting is optimized to O(n)
- Customizable read/write contexts for compressed data (for advanced use - e.g. embedding compressed data into Flash memory, unpacking into RAM)
- Rich API
- Fixed various warnings throughout library for XC8 compiler
- Improved sample_nanolz (added *_auto_ex2 compressor function) file size handling capabilities
- TBD next: *Extreme Low Memory* options (< 100 bytes extra RAM) with more advanced technique

### 23 October 2021: Release 1.0.3
- Suppressed some 'loses precision' warnings (explicit conversions are added)

### 23 October 2021: Release 1.0.2
- Added/used ECL_SCOPED_CONST define
- Added handling for ECL_EXCLUDE_HIMEM define (16bit-compilers friendly)
- Minor fix in ZeroDevourer for ECL_USE_BITNESS_16

### 28 March 2018: Release 1.0.1
- Fixed tests compilation for clang 64 bit
- Suppressed rare warning

### 10 March 2018: Release 1.0.0
- NanoLZ generic codec: decompressor, 7 compression modes, 2 *auto compression functions
- NanoLZ schemes: Scheme1, Scheme2(demo)
- ZeroDevourer codec
- ZeroEater codec
- high test coverage for all above
- sample program to compress/decompress files in NanoLZ:Scheme1 format


---
### Version 1.0.1 Tested on:
- Windows 7: msvc2013, msvc2015, gcc 4.8, gcc 7.2
- Mac OS 10.12: clang (Apple LLVM version 8.0.0)
- Embedded ARM Cortex-M3: armcc 5.06

### Version 1.1.0 Tested on:
- Windows 10: msvc2022, gcc 15.2.0, XC8 2.32 (compile only), arduino IDE (compile only)

### Version 1.2.0 Tested on:
- Windows 10: msvc2022, gcc 15.2.0, XC8 2.32 (compile only), arduino IDE (compile only)
---


## COMPRESSORS
Some of modes of some compressors use intermediate buffers for compression, they don't use any implicit allocation (unless otherwise specified) - user can easily choose how to allocate buffers.
Every compression method that uses temporary buffer (say, more than 10 bytes) - has it specified in documentation near method declaration.

### ECL:SLA (Sequence Levels Analyzer) - a specialized codec to compress a sequence of low-mean-amplitude values (which can be a spatial diff).
- use cases - chart data (integer array of sensor readings), audio, graphics (all adapted in a certain way)
- can be beneficial for very small amounts of data, only 6 bit extra info is added to output;
- compression ratio - small..middle;
- compression ratio limit - 8, or infinite for edge case (if all bytes are zero);
- compressors complexity: linear;
- compressors performance - middle..high;
- decompressor complexity - linear;
- decompressor performance - middle..high;
- compressor, decompressor buffer memory consumption - 0 (except from packing/unpacking aux functions that can normally use about some 128 bytes - according to user chosen block size);
- static const memory consumption for Analyze/Compress - 256 bytes + ~20 bytes;
- static const memory consumption for Decompress - ~20 bytes;
- stack consumption - normal;

### ECL:Huff8 - a Huffman codec implementation highly optimized for restricted memory environments. Works per-byte (at most 256 different codes).
- use cases - various, useful for non-equal distribution of bytes in user data;
- **all code is recursion-free**, no large stack arrays allocation, fully predictable and documented memory use;
- provides API for advanced use, medium use, trivial use;
- works with data blocks up to 65535 bytes (uint16 size), for larger datasets data should be split in smaller blocks (see sample); larger blocks make little sense as they harm statictics;
- can be beneficial for very small amounts of data if data set has not many unique bytes;
- compression ratio - small..middle;
- compression ratio limit - 8, or infinite for edge case (if all bytes are equal);
- compressors complexity for hypothetical worst case: O(n) + O(256^2)
- compressors complexity for average case: O(n) + O(log2(256) * 256)
- compressors complexity for normal small data case: O(n) + O(256)
- compressors performance - middle..high, provides different modes (used-memory/performance trade off);
- decompressor complexity - O(n);
- decompressor performance - middle..high;
- compressors buffer memory consumption - from 1.5kb to 1.8kb;
- decompressors buffer memory consumption - from 1kb to 1.75kb (can be less than 1kb for some cases, if needed);
- static const memory consumption - low;
- stack consumption - normal;

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


## PERFORMANCE BENCHMARKS for version 1.0.0 : NanoLZ, ZeroEater, ZeroDevourer (**year 2018, old hardware**)
PC benchmarks are performed for *Intel core i5-3570k @ 3.4 GHz / Windows 7 64 bit / 16gb RAM 1600 MHz*.
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


## PERFORMANCE BENCHMARKS for ECL:Huff8 / version 1.1.0 (**year 2025**)
### Windows environment - all measurements are done within 'sample/sample_huff8_blocked.cpp', 'memcpy' variant has overhead of auxiliary code in sample.
- hardware: AMD Ryzen 5600x, 32gb 3600mhz RAM
- compiler: gcc version 15.2.0
- optimization options: -O3
- Param is 'block size' parameter of the sample, which is 1..65535 (~32000 is optimal **here**)
- **ECL_Huff8_Compress16_ULM_Raw** (* **Compress ULM** in table) uses 1.8kb extra memory (fully self-sufficient compresor)
- **ECL_Huff8_TryCompress16_TSpec768_Raw** (* **Compress 768** in table) uses 1.5kb extra memory (fully self-sufficient compresor)
- **ECL_Huff8_TryCompress16_TSpec512_Raw** (* **Compress 512** in table) uses 1.5kb extra memory (fully self-sufficient compresor)
- **ECL_Huff8_Decompress_Raw** (* **Decompress** in table) uses 1kb extra memory (fully self-sufficient decompresor)
- **ECL_Huff8_DecompressWithDTable768_Raw** (* **Decompress Fast** in table) uses 1.75kb extra memory (fully self-sufficient decompresor)
- 'error' mark means that **a restricted compressor function** couldn't process some block (too big size)

#### System performance reference for *Silesia*.tar

| Param | Ratio | "Compression" replaced with memcpy | "Decompression" replaced with memcpy |
| ----- | ----- | ---------------------------------- | ------------------------------------ |
|  1000 | >1.00 |                         ~3300 mb/s |                           ~5800 mb/s |
| 65535 | >1.00 |                         ~3700 mb/s |                           ~6300 mb/s |


#### Huff8 performance for *Silesia*.tar
| Param | Ratio | Compress ULM | Compress 768 | Compress 512 | Decompress | Decompress Fast |
| ----- | ----- | ------------ | ------------ | ------------ | ---------- | --------------- |
|       |       |   1816 bytes |   1552 bytes |   1536 bytes | 1024 bytes |      1792 bytes |
|       |       |              |              |              |            |                 |
|   100 | 0.934 |    73.5 mb/s |    70.3 mb/s |    70.1 mb/s |  54.1 mb/s |       54.3 mb/s |
|   500 | 0.734 |   130.3 mb/s |   119.2 mb/s |   118.8 mb/s |  53.0 mb/s |      179.0 mb/s |
|  1000 | 0.686 |   158.1 mb/s |   142.0 mb/s |   141.3 mb/s |  54.9 mb/s |      210.9 mb/s |
|  2000 | 0.654 |**179.8 mb/s**|   159.5 mb/s |   159.0 mb/s |  57.0 mb/s |   **234.7 mb/s**|
|  4000 | 0.634 |   200.7 mb/s |   175.8 mb/s |        error |  60.0 mb/s |      251.2 mb/s |
|  8000 | 0.622 |   223.2 mb/s |   192.6 mb/s |        error |  63.1 mb/s |      263.4 mb/s |
| 16000 | 0.617 |   240.6 mb/s |   205.5 mb/s |        error |  66.4 mb/s |      271.6 mb/s |
| 32000 | 0.616 |   250.5 mb/s |   212.5 mb/s |        error |  69.8 mb/s |      276.5 mb/s |
| 65535 | 0.619 |**258.5 mb/s**|        error |        error |  73.1 mb/s |   **280.0 mb/s**|


#### Huff8 results for "nci" file from *Silesia*.tar
| Param | Ratio | Compress ULM | Decompress Fast |
| ----- | ----- | ------------ | --------------- |
| 32000 | 0.306 |   338.3 mb/s |      391.9 mb/s |

#### Huff8 results for "osdb" file from *Silesia*.tar
| Param | Ratio | Compress ULM | Decompress Fast |
| ----- | ----- | ------------ | --------------- |
| 32000 | 0.836 |   224.2 mb/s |      232.2 mb/s |


## PERFORMANCE BENCHMARKS for ECL:SLA / version 1.2.0 (**year 2026**)
### Windows environment - all measurements are done within 'sample/sample_sla_compress_wav.cpp'
- hardware: AMD Ryzen 5600x, 32gb 3600mhz RAM
- compiler: gcc version 15.2.0
- optimization options: -O3
- used 'block size' parameter = 128 (tests with '64', '32' have pretty similar results with a bit worse compression ratio)
- used several wav files converted from high quality flac audio samples, wav bitness = 16

#### System performance reference for .wav files

| Param | 'Pack + Analyze + Compress' replaced with memcpy | 'Decompress + Unpack' replaced with memcpy |
| ----- | ------------------------------------------------ | ------------------------------------------ |
|  128  |                                      ~13500 mb/s |                                ~13500 mb/s |

*Processed files have sizes 20-50mb, memcpy reference performance varies noticeably.

#### SLA performance for compressing .wav audio files
| Audio file, channel     | Ratio | 'Pack + Analyze + Compress' | 'Decompress + Unpack' |
| ----------------------- | ----- | --------------------------- | --------------------- |
| waterfall; channel L    | 0.334 |                   409 mb/s  |             469 mb/s  |
| waterfall; channel R    | 0.311 |                   444 mb/s  |             609 mb/s  |
| meditation; channel L   | 0.287 |                   462 mb/s  |             675 mb/s  |
| meditation; channel R   | 0.286 |                   458 mb/s  |             674 mb/s  |
| metal song 1; channel L | 0.433 |                   384 mb/s  |             434 mb/s  |
| metal song 1; channel R | 0.433 |                   386 mb/s  |             443 mb/s  |
| metal song 2; channel L | 0.430 |                   384 mb/s  |             427 mb/s  |
| metal song 2; channel R | 0.429 |                   379 mb/s  |             433 mb/s  |


## MULTITHREADING
Codecs don't share any non-const data, so API is thread safe.


## SAMPLE TESTING PROGRAMS
There are sample programs to compress/decompress files via command line (see **"sample/"** dir):
It's enough to compile single **"sample/\<foo\>.cpp"** file, some building scripts are provided in same dir.
Samples are unable to compress too large files as they're simplified (pre-read whole file) and used for benchmarks.
- `NanoLZ`:Scheme1 format (**"sample/sample_nanolz.cpp"**);
- `Huff8` blocked variant (**"sample/sample_huff8_blocked.cpp"**);
- `SLA` blocked audio compression (**"sample/sample_sla_compress_wav.cpp"**);


## USAGE
In general usage is pretty straightforward - you call \*Compress method, you call \*Decompress method - that's it.
- usage samples are present in headers of somes codecs.
- see usage examples in the sample programs (**"sample/"** dir).
- see more API usage examples in tests (**"tests/"** dir).


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
