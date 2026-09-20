# ALMP3 2.0.5 -- static library build for the ags/src reimplementation.
# A thin Allegro-based wrapper (src/almp3.c) around the mpg123/mpglib MP3
# decoder (decoder/*.c). All sources here are plain C (the "_i386" file
# names refer to the target architecture's tuning, not assembly).

file(GLOB ALMP3_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB ALMP3_DECODER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/decoder/*.c")

add_library(almp3 STATIC
  ${ALMP3_SOURCES}
  ${ALMP3_DECODER_SOURCES}
)

target_include_directories(almp3 PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(almp3 PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/decoder"
)

target_link_libraries(almp3 PUBLIC allegro)
