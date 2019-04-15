# CMakeList.txt : CMake project for decoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (decoder "decoder.cpp" "decoder.h" "KgDecoder.cpp" "KgDecoder.h" "KgFrame.cpp" "KgFrame.h" "myTypeDef.h")

# TODO: Add tests and install targets if needed.
