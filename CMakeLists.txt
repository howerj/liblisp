# Super minimal CMake 
cmake_minimum_required (VERSION 3.0) # May work with earlier versions.
project (lisp)
include_directories(./src/)
file(GLOB CFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_executable(lisp ${CFILES})
