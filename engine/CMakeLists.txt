cmake_minimum_required(VERSION 3.16)
project(wasteland_engine CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Three separate code bases: src/shared (helpers both games use), src/fod (Fountain of Dreams) and src/wasteland1
# (Wasteland 1). The games depend on shared only, never on each other. Headers are included as "shared/x.h",
# "fod/x.h" and "wasteland1/x.h".
add_subdirectory(src/shared)
add_subdirectory(src/fod)
add_subdirectory(src/wasteland1)
