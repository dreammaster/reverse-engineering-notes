# CMake toolchain file forcing the 32-bit MinGW-w64 (i686) compiler.
#
# This project targets 32-bit Windows deliberately: the third-party
# libraries under src/libs/ (Allegro 4.0.2, JGMOD, ALMP3, libcda) are all
# 2002-era code that assumes a 32-bit pointer/long size throughout, matching
# the original Rob Blanc 1 binary (PE32, Intel 80386). Building 64-bit would
# require extensive portability work with no payoff for this project's own
# archaeology goals.
#
# Usage (from a fresh build directory):
#   cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw32.cmake ..
#   cmake --build .
#
# Assumes the 32-bit toolchain was installed to C:/mingw32 (a standalone
# WinLibs MinGW-w64 i686 build -- see reversing/notes/ or the project's own
# setup history for exactly which release). Override MINGW32_ROOT if yours
# lives elsewhere.

if(NOT DEFINED MINGW32_ROOT)
  set(MINGW32_ROOT "C:/mingw32")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER   "${MINGW32_ROOT}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${MINGW32_ROOT}/bin/g++.exe")
set(CMAKE_RC_COMPILER  "${MINGW32_ROOT}/bin/windres.exe")

set(CMAKE_FIND_ROOT_PATH "${MINGW32_ROOT}/i686-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
