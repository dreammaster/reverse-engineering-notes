# libcda 0.4 (Windows component) -- static library build for the ags/src
# reimplementation. Windows CD-audio playback via mciSendString.
#
# libcda.h was reconstructed for this build -- the original archive this
# project has only ever preserved libcdaWin.C itself, not its own header
# (see reversing/notes/third-party-library-identification.md).

add_library(libcda STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/libcdaWin.C"
)
# CMake treats a capital-.C extension as C++ by default; this is plain C.
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/libcdaWin.C" PROPERTIES LANGUAGE C
)

target_include_directories(libcda PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(libcda PUBLIC winmm)
