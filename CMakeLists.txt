cmake_minimum_required(VERSION 3.0.0)
project(discrete-figures)

add_executable(simple_enumeration src/simple_enumeration.cpp)
add_executable(opti_enumeration src/opti_enumeration.cpp)
add_executable(white_rejected src/white_rejected.cpp)

add_subdirectory(app)