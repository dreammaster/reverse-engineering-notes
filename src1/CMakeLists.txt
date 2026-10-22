cmake_minimum_required(VERSION 3.10)
project(yendor1 CXX)

# Reimplementation of Yendorian Tales Book I (Chapter 1): C++11, one namespace per original
# executable under Yendor1:: (Sw, Prog0 .. Prog18) plus the shared layers Vid, Data and Shared.
# Game logic is platform independent; SDL2 is only used by platform/ and the viewer tools.
#
#   cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build --output-on-failure
#
# The tests that read game data look in ../../yendor1/game (or $YENDOR1_GAME_DIR) and skip
# themselves when the files are not there.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

# One static library per namespace-level layer; every program module joins the engine library.
file(GLOB_RECURSE YENDOR1_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/data/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vid/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sw/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/prog*/*.cpp)
add_library(yendor1 STATIC ${YENDOR1_SOURCES})
target_include_directories(yendor1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
file(GLOB YENDOR1_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
foreach(test_source ${YENDOR1_TESTS})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} yendor1)
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
  add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endforeach()

# Command line tools (graphics dumpers etc.); they link PNG output, which is tools/ only.
add_library(yendor1_toolutil STATIC tools/Png.cpp)
target_include_directories(yendor1_toolutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB YENDOR1_TOOLS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp)
foreach(tool_source ${YENDOR1_TOOLS})
  get_filename_component(tool_name ${tool_source} NAME_WE)
  if(tool_name STREQUAL "Png")
    continue()
  endif()
  add_executable(${tool_name} ${tool_source})
  target_link_libraries(${tool_name} yendor1 yendor1_toolutil)
  set_target_properties(${tool_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools)
endforeach()
