@echo off

set SPIN=%CD%\bin\spin.exe
set PATH=%PATH%;D:\msys64\mingw64\bin

%SPIN% -v -w -a conditioner_multi.pml

gcc -pedantic -Wall -o pan_multi pan.c 
del pan.b pan.c pan.h pan.m pan.p pan.t pan_t.c pan_t.t pan_t.m pan_t.b pan_t.h
rem ./pan