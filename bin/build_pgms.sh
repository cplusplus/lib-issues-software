#!/bin/sh
g++ -m32 -std=c++0x -DNDEBUG -O2 -o bin/lists src/lists.cpp src/date.cpp
g++ -m32 -std=c++0x -o bin/section_data src/section_data.cpp
g++ -m32 -std=c++0x -o bin/toc_diff src/toc_diff.cpp
