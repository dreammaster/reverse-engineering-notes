cmake_minimum_required(VERSION 3.16)
project(wiz1 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/W4 /permissive-)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra)
endif()

# Core data layer -- no external dependencies, portable to a ScummVM engine.
add_library(wizcore STATIC
  src/ucsd_volume.cpp
  src/string_pool.cpp
  src/scenario.cpp
)
target_include_directories(wizcore PUBLIC include)

# Standalone CLI harness / test driver.
add_executable(wiz1 src/main.cpp)
target_link_libraries(wiz1 PRIVATE wizcore)

enable_testing()
add_test(NAME dump_toc
         COMMAND wiz1 toc "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA")
