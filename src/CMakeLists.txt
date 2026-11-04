cmake_minimum_required(VERSION 3.10)
project(cadaver C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(cadaver_engine STATIC
  engine/mjm.c
  engine/pc1.c
  engine/lng.c
  engine/sprites.c
  engine/mp2.c
  engine/rooms.c
  engine/gamefiles.c)
target_include_directories(cadaver_engine PUBLIC engine)
target_compile_options(cadaver_engine PRIVATE -Wall -Wextra)

add_executable(cadaver_unpack tools/cadaver_unpack.c)
target_link_libraries(cadaver_unpack cadaver_engine)

add_executable(test_formats tests/test_formats.c)
target_link_libraries(test_formats cadaver_engine)

find_package(SDL2 QUIET)
if(SDL2_FOUND)
  add_executable(cadaver main.c)
  target_include_directories(cadaver PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(cadaver cadaver_engine ${SDL2_LIBRARIES})
else()
  message(STATUS "SDL2 not found: building tools only")
endif()

enable_testing()
add_test(NAME formats COMMAND test_formats ${CMAKE_SOURCE_DIR}/..)
