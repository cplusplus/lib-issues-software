g++ -m32 -std=c++11 -DNDEBUG -O2 -o bin/lists.exe  src/date.cpp src/issues.cpp src/sections.cpp src/mailing_info.cpp src/report_generator.cpp src/lists.cpp
g++ -m32 -std=c++11 -o bin/section_data.exe src/section_data.cpp
g++ -m32 -std=c++11 -o bin/toc_diff.exe src/toc_diff.cpp
