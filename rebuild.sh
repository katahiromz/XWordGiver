#!/bin/bash
rm CMakeCache.txt
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
make -j4
