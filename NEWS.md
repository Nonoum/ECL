## Release 1.0.2
- Added/used ECL_SCOPED_CONST define
- Added handling for ECL_EXCLUDE_HIMEM define (16bit-compilers friendly)
- Minor fix in ZeroDevourer for ECL_USE_BITNESS_16

## Release 1.0.1
- Fixed tests compilation for clang 64 bit
- Suppressed rare warning

## Release 1.0.0
- NanoLZ generic codec: decompressor, 7 compression modes, 2 *auto compression functions
- NanoLZ schemes: Scheme1, Scheme2(demo)
- ZeroDevourer codec
- ZeroEater codec
- high test coverage for all above
- sample program to compress/decompress files in NanoLZ:Scheme1 format
