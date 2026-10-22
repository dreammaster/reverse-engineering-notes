add_library(shared STATIC bytes.cpp image.cpp)
target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(shared PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>)
