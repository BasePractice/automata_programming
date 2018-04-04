@echo off

set SPIN=%CD%\bin\spin.exe
set PATH=%PATH%;D:\msys64\mingw64\bin

%SPIN% cnc.pml

