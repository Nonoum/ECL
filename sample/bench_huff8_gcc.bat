@echo off
IF %1.==. GOTO MissingParam
@echo on

cls
set block_size=32000
gcc sample_huff8_blocked.cpp -Wall -Wextra -pedantic -O3 -std=c++17 -lstdc++ -o ecl.exe
ecl.exe c %block_size% %1 xcomp
gcc sample_huff8_blocked.cpp -DSAMPLE_C768 -Wall -Wextra -pedantic -O3 -std=c++17 -lstdc++ -o ecl.exe
ecl.exe c %block_size% %1 xcomp
gcc sample_huff8_blocked.cpp -DSAMPLE_C512 -Wall -Wextra -pedantic -O3 -std=c++17 -lstdc++ -o ecl.exe
ecl.exe c %block_size% %1 xcomp
ecl.exe d xcomp xrec
gcc sample_huff8_blocked.cpp -DSAMPLE_DSLOW -Wall -Wextra -pedantic -O3 -std=c++17 -lstdc++ -o ecl.exe
ecl.exe d xcomp xrec


@echo off
GOTO Exit
@echo on

:MissingParam
  ECHO specify file name to try compression with. Output files are "xcomp" and "xrec"
:Exit
