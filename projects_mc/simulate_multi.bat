@echo off

set SPIN=%CD%\bin\spin.exe
set PATH=%PATH%;D:\msys64\mingw64\bin

rem %SPIN% -v -w conditioner.pml
%SPIN% conditioner_multi.pml

