cmake_minimum_required(VERSION 3.16)
project(wasteland_engine CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(fod STATIC
    src/lzss.cpp src/archive.cpp src/map.cpp src/script.cpp src/gfx.cpp src/data.cpp)
target_include_directories(fod PUBLIC include)
target_compile_options(fod PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>)

add_executable(fodtool tools/fodtool.cpp)
target_link_libraries(fodtool fod)
