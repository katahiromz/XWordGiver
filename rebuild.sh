#!/bin/bash
rm CMakeCache.txt
CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
make -j4
