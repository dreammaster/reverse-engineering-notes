cmake_minimum_required(VERSION 3.10)
project(Xanth CXX)

# C++11 at most (ScummVM's limit), no exceptions, no RTTI: the code must be importable into a ScummVM engine unchanged in style.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(-Wall -Wextra -pedantic -fno-exceptions -fno-rtti)
endif()

option(XANTH_SDL2 "Build the SDL2 standalone game (needs SDL2 development files)" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# common/ + legend/: code shared by Legend engines
add_library(legend STATIC
	common/files.cpp
	legend/dcl.cpp
	legend/pic.cpp
	legend/rgn.cpp
	legend/messages.cpp
	legend/audiofiles.cpp
)

# xanth/: Companions of Xanth
add_library(xanth_engine STATIC
	xanth/objects.cpp
)
target_link_libraries(xanth_engine legend)

enable_testing()

add_executable(test_formats tests/test_formats.cpp)
target_link_libraries(test_formats legend)
add_test(NAME formats COMMAND test_formats)

add_executable(test_objects tests/test_objects.cpp)
target_link_libraries(test_objects xanth_engine)
add_test(NAME objects COMMAND test_objects)
