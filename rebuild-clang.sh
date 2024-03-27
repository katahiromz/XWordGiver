#!/bin/bash
rm CMakeCache.txt
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" .
mingw32-make.exe -j4
strip *.exe
