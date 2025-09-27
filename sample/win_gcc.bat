@echo off
IF %1.==. GOTO MissingParam
@echo on
gcc %1 -Wall -Wextra -pedantic -O3 -std=c++17 -lstdc++ -o ecl.exe

@echo off
GOTO Exit
@echo on

:MissingParam
  ECHO specify sample *.cpp file name to compile
:Exit
