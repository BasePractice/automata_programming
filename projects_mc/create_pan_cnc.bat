@echo off

set SPIN=%CD%\bin\spin.exe
set PATH=%PATH%;D:\msys64\mingw64\bin

%SPIN% -v -w -a cnc.pml

gcc -o pan_cnc pan.c 
del pan.b pan.c pan.h pan.m pan.p pan.t pan_t.c pan_t.t pan_t.m pan_t.b pan_t.h
pan_cnc -a