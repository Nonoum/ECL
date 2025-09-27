@echo off
IF %1.==. GOTO MissingParam
@echo on
cl %1 /O2 /EHsc /Fe:ecl.exe

@echo off
GOTO Exit
@echo on

:MissingParam
  ECHO specify sample *.cpp file name to compile
:Exit
