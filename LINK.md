[http://spinroot.com/spin/Bin/spin647_windows64.exe.gz](SPIN Win64)
[http://repo.msys2.org/distrib/msys2-x86_64-latest.exe](MINGW w64)


(`pacman -Syuu`, `pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake git`)



```
git submodule init
git submodule update --recursive
mkdir .build
cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
mingw32-make.exe

```
