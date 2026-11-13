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
	legend/font.cpp
	legend/surface.cpp
	legend/draw.cpp
	legend/textwindow.cpp
)

# xanth/: Companions of Xanth
file(GLOB XANTH_LOGIC CONFIGURE_DEPENDS xanth/logic/*.cpp)
add_library(xanth_engine STATIC
	xanth/objects.cpp
	xanth/objectnames.cpp
	xanth/grammar.cpp
	xanth/gamestate.cpp
	xanth/game.cpp
	xanth/mainscreen.cpp
	xanth/display.cpp
	xanth/verbs.cpp
	xanth/interface.cpp
	xanth/logic_support.cpp
	xanth/tables/vars.cpp
	${XANTH_LOGIC}
)
target_link_libraries(xanth_engine legend)

enable_testing()

add_executable(test_formats tests/test_formats.cpp)
target_link_libraries(test_formats legend)
add_test(NAME formats COMMAND test_formats)

add_executable(test_objects tests/test_objects.cpp)
target_link_libraries(test_objects xanth_engine)
add_test(NAME objects COMMAND test_objects)

add_executable(test_state tests/test_state.cpp)
target_link_libraries(test_state xanth_engine)
add_test(NAME state COMMAND test_state)

add_executable(test_graphics tests/test_graphics.cpp)
target_link_libraries(test_graphics xanth_engine)
add_test(NAME graphics COMMAND test_graphics)

add_executable(xanth_play tools/playcli.cpp)
target_link_libraries(xanth_play xanth_engine)

add_executable(test_screen tests/test_screen.cpp)
target_link_libraries(test_screen xanth_engine)

add_executable(test_scan tests/test_scan.cpp)
target_link_libraries(test_scan xanth_engine)
add_test(NAME scan COMMAND test_scan)

add_executable(test_grammar tests/test_grammar.cpp)
target_link_libraries(test_grammar xanth_engine)
add_test(NAME grammar COMMAND test_grammar)

# The tests that compare with the original need its data and the analysis tables from the surrounding reverse-engineering tree (not part of a stand-alone checkout):
# when they exist their absolute paths are handed to the tests, otherwise those comparisons are skipped. XANTH_ROOT is the folder that holds game/, game_out/ and reversing/.
set(XANTH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "folder with the original's game files and the analysis database")
set(XANTH_TEST_ENV "")
if(EXISTS "${XANTH_ROOT}/game")
	list(APPEND XANTH_TEST_ENV "XANTH_DATA=${XANTH_ROOT}/game")
endif()
if(EXISTS "${XANTH_ROOT}/reversing/analysis/xanth_objects.tsv")
	list(APPEND XANTH_TEST_ENV "XANTH_OBJECTS=${XANTH_ROOT}/reversing/analysis/xanth_objects.tsv")
endif()
if(EXISTS "${XANTH_ROOT}/game_out/restart.dat")
	list(APPEND XANTH_TEST_ENV "XANTH_RESTART=${XANTH_ROOT}/game_out/restart.dat")
endif()
add_test(NAME screen COMMAND test_screen)
set_tests_properties(formats objects state grammar scan graphics screen PROPERTIES ENVIRONMENT "${XANTH_TEST_ENV}")
