cmake_minimum_required(VERSION 3.0.0)
project(discrete-figures VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

# Graphical application made with files from src/application/*, using FLTK as graphics library.
file(GLOB_RECURSE src_app CONFIGURE_DEPENDS src/application/*)
add_executable(discrete-figures ${src_app})
target_link_libraries(discrete-figures PUBLIC raylib)

# Tests applications
add_executable(test-hash src/tests/test_hash.cpp)
add_executable(test-martinalgo src/tests/test_martinalgo.cpp)

