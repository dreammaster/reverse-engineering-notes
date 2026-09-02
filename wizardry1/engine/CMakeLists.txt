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

# Core data + platform layer -- portable to a ScummVM engine.
add_library(wizcore STATIC
  src/ucsd_volume.cpp
  src/string_pool.cpp
  src/scenario.cpp
  src/character.cpp
  src/surface.cpp
  src/bitmap.cpp
  src/font.cpp
  src/textscreen.cpp
  src/roster.cpp
  src/roller_ui.cpp
  src/party.cpp
  src/shop.cpp
  src/maze.cpp
  src/maze3d.cpp
  src/maze_ui.cpp
  src/combat.cpp
  src/combat_ui.cpp
  src/town_ui.cpp
  src/platform_null.cpp
  src/platform_sdl.cpp
)
target_include_directories(wizcore PUBLIC include)

# Optional SDL2 window backend.  Uses find_package, or the ScummVM prebuilt
# libs if WIZ_SDL_DIR points at them (e.g. C:/dev/scummvm_libs_2015).
set(WIZ_SDL_DIR "" CACHE PATH "prebuilt SDL2 (contains include/SDL2 and lib/<arch>/Release)")
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND AND WIZ_SDL_DIR AND EXISTS "${WIZ_SDL_DIR}/include/SDL2/SDL.h")
  add_library(SDL2::SDL2 UNKNOWN IMPORTED)
  file(GLOB _sdl_lib "${WIZ_SDL_DIR}/lib/x64/Release/SDL2.lib" "${WIZ_SDL_DIR}/lib/SDL2.lib")
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "${_sdl_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${WIZ_SDL_DIR}/include/SDL2"
    # the ScummVM prebuilt SDL2.lib is static -- pull in the Win32 libs it needs
    INTERFACE_LINK_LIBRARIES "winmm;setupapi;imm32;version;cfgmgr32")
  if(_sdl_lib)
    set(SDL2_FOUND TRUE)
  endif()
endif()
if(SDL2_FOUND)
  target_link_libraries(wizcore PUBLIC SDL2::SDL2)
  target_compile_definitions(wizcore PUBLIC WIZ_HAVE_SDL)
  message(STATUS "wiz1: SDL2 window backend enabled")
else()
  message(STATUS "wiz1: no SDL2 -- headless (PPM) backend only")
endif()

add_executable(wiz1 src/main.cpp)
target_link_libraries(wiz1 PRIVATE wizcore)

enable_testing()
add_test(NAME dump_toc
         COMMAND wiz1 toc "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA")
add_test(NAME roster_roundtrip
         COMMAND wiz1 roster "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA")
add_test(NAME roller_flow
         COMMAND wiz1 roller-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "ZORK\\rY\\r\\rBA++++++++++\\eAY\\r")
set_tests_properties(roller_flow PROPERTIES PASS_REGULAR_EXPRESSION "ZORK .*C0 A1")
add_test(NAME town_flow
         COMMAND wiz1 town-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "GATHESUS\\r\\rALYSANDER\\r\\rATREON\\r\\r\\rEM")
set_tests_properties(town_flow PROPERTIES
    PASS_REGULAR_EXPRESSION "exit: MAZE(.|\n)*3\\) TREON +slot=3")
add_test(NAME inn_levelup
         COMMAND wiz1 inn-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA")
set_tests_properties(inn_levelup PROPERTIES
    PASS_REGULAR_EXPRESSION "rested 3 week.*(.|\n)*YOU MADE A LEVEL(.|\n)*after:  L2")
add_test(NAME shop_buy_sell
         COMMAND wiz1 town-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "GATHESUS\\r\\r\\rB1BP1\\rP2\\rLS1\\r\\rL\\rEM"
                 ""
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/ASCII.KRN")
# buy #1 (25 gp) + #2 (15 gp), sell #1 back unidentified-safe (identified: 12 gp):
# 185 - 25 - 15 + 12 = 157, keeping item #2.
set_tests_properties(shop_buy_sell PROPERTIES
    PASS_REGULAR_EXPRESSION "exit: MAZE(.|\n)*157gp poss: #2")
add_test(NAME temple_rules COMMAND wiz1 temple-test)
set_tests_properties(temple_rules PROPERTIES
    PASS_REGULAR_EXPRESSION "paralyzed.*CURED(.|\n)*ashes-lo-vit.*WORSENED(.|\n)*WILL BE BURIED")
add_test(NAME temple_heal
         COMMAND wiz1 town-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "GATHESUS\\r\\r\\rCA1\\r\\rEM" "" ""
                 "wound=6:3")
# THESUS (185 gp) tithes 100 for a level-1 paralysis cure -> 85 gp, and roster
# slot 6 is no longer listed as wounded.
set_tests_properties(temple_heal PROPERTIES
    PASS_REGULAR_EXPRESSION "exit: MAZE(.|\n)*THESUS[^\n]*85gp")
set_tests_properties(temple_heal PROPERTIES
    FAIL_REGULAR_EXPRESSION "roster 6")
add_test(NAME maze_walk
         COMMAND wiz1 maze
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA" 1
                 "FFFRFF")
# Wizardry level 1 starts at (0,0) NORTH on the castle stairs; three steps
# north hug the west wall, then a wall blocks two attempts to move east.
set_tests_properties(maze_walk PROPERTIES
    PASS_REGULAR_EXPRESSION "RESULT .0,3. EAST bumps=2 square=0")
add_test(NAME maze_3d
         COMMAND wiz1 maze
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA" 1
                 "FFFR")
# Facing the dead-end wall east of (0,3): DRAWFRNT fills the top of the
# 82x79 pic with a solid wall edge.
set_tests_properties(maze_3d PROPERTIES
    PASS_REGULAR_EXPRESSION "###################################")
add_test(NAME maze_play
         COMMAND wiz1 maze-play-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "NFFFR\\e")
# decline the castle stairs, walk three north, turn east, then leave:
set_tests_properties(maze_play PROPERTIES
    PASS_REGULAR_EXPRESSION "exit: TO-TOWN   pos .0,3. EAST  level 1")
add_test(NAME town_to_maze
         COMMAND wiz1 town-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA"
                 "GATHESUS\\r\\r\\rEMNFF\\e")
# form a party, Edge of Town -> Maze, decline stairs, step twice, leave:
set_tests_properties(town_to_maze PROPERTIES
    PASS_REGULAR_EXPRESSION "maze exit: TO-TOWN  pos .0,2. NORTH")
add_test(NAME combat_fight
         COMMAND wiz1 combat-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/200.CHARSET"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/SCENARIO.DATA" 1
                 "\\rFFFFFF\\rFFFFFF\\rFFFFFF\\rFFFFFF\\rFFFFFF\\r\\r\\r"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/wiz1/ASCII.KRN")
# a full party FIGHTs the weakest monster group and wins with XP.
set_tests_properties(combat_fight PROPERTIES
    PASS_REGULAR_EXPRESSION "result: WON(.|\n)*THESUS[^\n]* [1-9][0-9]* ep")
