EMBEDDED COMPRESSION LIBRARY (ECL) is first of all NOT ONLY for embedded, it just has special low-memory modes for restricted environment and works greatly for it.

Language: C
Platforms: any
Library version: 0.1.0


BUILDING:
If you need this library - you're assumed to have project to use it in, so there's no build scripts. Just add appropriate files to your project.
To have compressor "FOO" available - include "ECL_FOO.h", compile "ECL_common.c" + "ECL_FOO.c" or just include those ".c" files in the end of some of your sources.


COMPRESSORS:
Some of modes of some compressors use intermediate buffers for compression, they don't use any implicit allocation - user can easily choose how to allocate buffers.
Every compression method that uses temporary buffer (say, more than 10 bytes) - has it specified in documentation near method declaration.

ECL:AHOC - TBD

ECL:NanoLZ - meticulously formatted version of traditional LZ77 algorithm.
- use cases - various, same with other pure LZ algorithms;
- takes advantage of repeated sequences with length of 2 bytes;
- provides API for adjusting compressed data format (see "schemes") to gain better compression ratio for user's datasets (for advanced users);
- can be beneficial for very small amounts of data (e.g. to fit some data into single Bluetooth Low Energy packet);
- compression ratio - middle-high;
- compression ratio limit - roughly infinite;
- compressor performance - quadratic to cubic, low-middle, provides different modes, configurable compression/performance trade off;
- compressor performance for small data payloads - optimized;
- decompressor performance - linear, middle-high;
- compressor buffer memory consumption - from zero to any, has different modes using 0, 256, 512,.. bytes for temporary buffers;
- decompressor buffer memory consumption - zero;
- static const memory consumption - low;
- stack consumption - normal;
- (TBD if needed) stream modes for processing infinite amounts of data;

ECL:ZeroDevourer - a diff-oriented compressor that takes advantage of zero bytes (even single ones) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - roughly, linearly depends on percentage of zeroes in your data, for target use cases - high;
- compression ratio limit - roughly infinite;
- compressor performance - linear, high (up to gigabyte per second);
- decompressor performance - linear, high (up to several gigabytes per second);
- compressor buffer memory consumption - zero;
- decompressor buffer memory consumption - zero;
- static const memory consumption - 10 bytes;
- stack consumption - low;
- binary code size - low;

ECL:ZeroEater - a diff-oriented compressor that takes advantage of zero bytes (starting from two bytes in a row) in your stream.
- use cases - incremental update of a structure FOO where you compress (FOO_before XOR FOO_after) rather than FOO itself, or data with significant amount of zeroes;
- compression ratio - similar to ZeroDevourer but worse, for target use cases - high;
- compression ratio limit - 64;
- compressor performance - linear, very high (up to gigabyte per second);
- decompressor performance - linear, very high (up to several gigabytes per second);
- compressor buffer memory consumption - zero;
- decompressor buffer memory consumption - zero;
- static const memory consumption - zero;
- stack consumption - lowest;
- binary code size - minimum;
- doesn't require ECL_common.c file to be compiled;


FORMATS:
Formats of compressors are described in "formats/" dir, there are common features shared between compressors (except ZeroEater, which is simple and independent):
- "formats/ECL_JH.txt" - format of Jumping Header, main feature and core of the library;
- "formats/ECL_E_number_format.txt" - format of 'Extensible' numbers;


CONFIGURING:
See ECL_config.h:
- you can explicitly specify bitness of length variables on your consideration (ECL_USE_BITNESS_16 / 32 / 64 macro);
- you can enable/disable branchless optimizations for your consideration (ECL_USE_BRANCHLESS macro);
- in case you don't have uint*_t types defined in stdint.h - define those types there near "user setup part";


PERFORMANCE BENCHMARKS:
*Tested for Intel core i5-3570k @ 3.4 GHz / Windows 7 64 bit / 16gb RAM 1600 MHz
*Compiled with GCC 4.8.1, options = -O3 -DECL_USE_BITNESS_32 -DECL_USE_ASSERT
*Tested with data samples from "Silesia compression corpus" (file sizes = 6 .. 51 mb)
*Speed is in megabytes per second (mb/s)

1. ECL:NanoLZ:Scheme1:
- ECL_NanoLZ_Decompress: speed = 80 .. 592 mb/s
- ECL_NanoLZ_Compress_fast2 26 bits (256 mb), search_limit 2: speed = 34 .. 128 mb/s
- ECL_NanoLZ_Compress_fast2 26 bits (256 mb), search_limit 5: speed = 14 .. 104 mb/s
- ECL_NanoLZ_Compress_fast2 26 bits (256 mb), search_limit 10: speed = 5.3 .. 81 mb/s
- ECL_NanoLZ_Compress_fast2 26 bits (256 mb), search_limit 20: speed = 2.3 .. 58 mb/s
- ECL_NanoLZ_Compress_fast2 26 bits (256 mb), search_limit 50: speed = 1.18 .. 31 mb/s

- ECL_NanoLZ_Compress_fast2 16 bits (512 kb), search_limit 10: speed = 20 .. 83 mb/s
- ECL_NanoLZ_Compress_fast2 16 bits (512 kb), search_limit 50: speed = 11 .. 40 mb/s
- ECL_NanoLZ_Compress_fast2 16 bits (512 kb), search_limit 100: speed = 7 .. 27 mb/s

- ECL_NanoLZ_Compress_fast2 12 bits (272 kb), search_limit 100: speed = 18 .. 60 mb/s
- ECL_NanoLZ_Compress_fast2 12 bits (272 kb), search_limit 200: speed = 16 .. 56 mb/s

- ECL_NanoLZ_Compress_fast2 10 bits (260 kb), search_limit 100: speed = 32 .. 85 mb/s
- ECL_NanoLZ_Compress_fast2 10 bits (260 kb), search_limit 200: speed = 27 .. 83 mb/s

On whole .tar archive as single file (202 mb):
- ECL_NanoLZ_Compress_fast2 16 bits (512 kb), search_limit 100: speed = 14.17 mb/s
- ECL_NanoLZ_Decompress: speed = 184 mb/s
- ratio: 0.418 (compressed size = 41.8% or input file)

Compression ratio: these huge files have nothing to do with embedded, they're used for more accurate performance benchmarking.
Resulted ratio loses to most of compressors, while it's better on small (you may say 'very small') files.
For my target datasets (around 1-2 kb) I achieved compressed size = around 75-80% of "LZ4_HC -9" compressed output, which is pretty satisfying.

*NOTE: actual performance depends on particular file.
*NOTE: decompression speed grows with bigger search_limit specified on compression.
*NOTE: ECL_NanoLZ_Compress_fast2 is fastest NanoLZ compression mode, consumes maximum amount of memory among all modes.
*NOTE: "16 bits (512 kb)" means that algorithm uses 16 bit searching window, 512 kb of temporary buffers in total.
