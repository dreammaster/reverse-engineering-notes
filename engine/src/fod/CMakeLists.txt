add_library(fod STATIC
    lzss.cpp archive.cpp map.cpp script.cpp gfx.cpp data.cpp state.cpp world.cpp vm.cpp screen.cpp game.cpp combat.cpp
    combat_fight.cpp progress.cpp shop.cpp party.cpp charsheet.cpp savegame.cpp creator.cpp sound.cpp timed.cpp)
target_link_libraries(fod PUBLIC shared)
target_compile_options(fod PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>)

add_executable(fodrun tools/fodtool.cpp)
target_link_libraries(fodrun fod)
if(MINGW)
    # fully static executables (the system antivirus removes dynamically linked builds here)
    target_link_options(fodrun PRIVATE -static)
endif()
if(WIN32)
    add_executable(fodplay platform/win32_main.cpp)
    target_link_libraries(fodplay fod winmm)
    target_link_options(fodplay PRIVATE -static -mwindows)
endif()
