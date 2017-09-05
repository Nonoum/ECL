EMBEDDED COMPRESSION LIBRARY (ECL) is first of all NOT ONLY for embedded, it just has special low-memory modes for restricted environment and works greatly for it.

Language: C
Platforms: any
Library version: 0.1.0

1. Building
If you need this library - you're assumed to have project to use it in, so there's no build scripts. Just add appropriate files to your project.
To have compressor "FOO" available you need to include "ECL_FOO.h" and compile "ECL_common.c" + "ECL_FOO.c".

2. Compressors
Some of modes of some compressors use intermediate buffers for compression, they don't use any implicit allocation - user can easily choose how to provide buffers.
Every compression method that uses temporary buffer (say, more than 10 bytes) - has it specified in documentation near method declaration.

2.1. ECL:AHOC - TBD in late 2017

2.2. ECL:NanoLZ - meticulously formatted version of traditional LZ77 algorithm.
- use cases - various, same with other pure LZ algorithms;
- takes advantage of repeated sequences with length of 2 bytes, thus provides higher compression in most of cases;
- compression ratio - high;
- compression ratio limit - roughly infinite;
- compressor performance - quadratic to cubic, middle-high, has differernt modes, configurable compression/performance trade off;
- compressor performance for small data payloads - optimized (better than regular compressors for big data);
- decompressor performance - linear, middle-high;
- buffer memory consumption - zero to middle, has different modes using 0, 256, 512,.. 130k bytes of temporary buffer;
- static const memory consumption - low;
- stack consumption - normal;
- has stream modes for processing infinite amounts of data; - TBD

2.3. ECL:ZeroDevourer - a diff-oriented compressor that takes advantage of zero bytes (even single ones) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - roughly linearly depends on percentage of zeroes in your data, for target use cases - high;
- compression ratio limit - roughly infinite;
- compressor performance - linear, high;
- decompressor performance - linear, high;
- buffer memory consumption - zero;
- static const memory consumption - ~20 bytes;
- stack consumption - low;

2.4. ECL:ZeroEater - a diff-oriented compressor that takes advantage of zero bytes (starting from two bytes in a row) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - similar to ZeroDevourer but worse, for target use cases - high;
- compression ratio limit - 64;
- compressor performance - linear, very high;
- decompressor performance - linear, very high;
- buffer memory consumption - zero;
- static const memory consumption - zero;
- stack consumption - lowest;
- doesn't require ECL_common.c file to be compiled;

3. Formats
Formats of compressors are described in "formats/" dir, there are common features shared between compressors (except ZeroEater, which is simple and independent):
- "formats/ECL_JH.txt" - format of Jumping Header, main feature and core of the library;
- "formats/ECL_E_number_format.txt" - format of 'Extendable' numbers;

4. Configuring
See ECL_config.h:
- you can explicitly specify bitness of length variables on your consideration (ECL_USE_BITNESS_16 / 32 / 64 macro);
- you can enable/disable branchless optimizations for your consideration (ECL_USE_BRANCHLESS macro);
- in case you don't have uint*_t types defined in stdint.h - define those types there near "user setup part";
