cmake_minimum_required(VERSION 3.10)
project(yendor23 C)

# The Yendorian Tales Book I (Chapters 2 and 3) engine logic: one static library of every module in this directory, the test suites
# (tests/test_*.c, run with ctest from tests/) and the tools (tools/*.c). Plain C99, no platform code in the library itself; only
# tools/explore_sdl.c needs SDL2.
#
#   cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
#
# The tests that read game data look for ../../yendor2/game and ../../yendor3/game (or YENDOR2_GAME_DIR / YENDOR3_GAME_DIR) and skip
# themselves when the files are not there.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

file(GLOB YENDOR23_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_library(yendor23 STATIC ${YENDOR23_SOURCES})
target_include_directories(yendor23 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
  target_link_libraries(yendor23 PUBLIC m)
endif()

enable_testing()
file(GLOB YENDOR23_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.c)
foreach(test_source ${YENDOR23_TESTS})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} yendor23)
  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
  add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endforeach()

# Command line tools (renderers, the intro and sound dumpers, a fuzzer); explore_sdl is the playable front end and needs SDL2.
file(GLOB YENDOR23_TOOLS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/*.c)
find_package(SDL2 QUIET)
foreach(tool_source ${YENDOR23_TOOLS})
  get_filename_component(tool_name ${tool_source} NAME_WE)
  if(tool_name STREQUAL "explore_sdl")
    if(NOT SDL2_FOUND)
      message(STATUS "SDL2 not found: explore_sdl is not built")
      continue()
    endif()
  endif()
  add_executable(${tool_name} ${tool_source})
  target_link_libraries(${tool_name} yendor23)
  set_target_properties(${tool_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools)
  if(tool_name STREQUAL "explore_sdl")
    target_link_libraries(${tool_name} SDL2::SDL2)
  endif()
endforeach()
