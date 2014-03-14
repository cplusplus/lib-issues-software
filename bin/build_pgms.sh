#!/bin/sh
echo '"Use -m32 switch to force 32-bit build"'
g++ $* -std=c++11 -DNDEBUG -O2 -o bin/lists src/date.cpp src/issues.cpp src/sections.cpp src/mailing_info.cpp src/report_generator.cpp src/file_names.cpp src/lists.cpp
g++ $* -std=c++11 -o bin/section_data src/section_data.cpp
g++ $* -std=c++11 -o bin/toc_diff src/toc_diff.cpp
g++ $* -std=c++11 -DNDEBUG -O2 -o bin/list_issues src/date.cpp src/issues.cpp src/sections.cpp src/list_issues.cpp
g++ $* -std=c++11 -DNDEBUG -O2 -o bin/set_status  src/set_status.cpp

