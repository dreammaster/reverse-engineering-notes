cmake_minimum_required(VERSION 3.10)
project(shannara_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(shannara STATIC
	src/shanstr.cpp
	src/pic.cpp
	src/rgn.cpp
	src/fnt.cpp
	src/le.cpp
	src/retail_data.cpp
	src/mus.cpp
	src/objectdat.cpp
	src/qvideo.cpp)
target_include_directories(shannara PUBLIC src)
if(NOT MSVC)
	target_compile_options(shannara PRIVATE -Wall -Wextra)
endif()

add_executable(shan_tool src/shan_tool.cpp)
target_link_libraries(shan_tool shannara)
