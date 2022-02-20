cmake_minimum_required(VERSION 3.0.0)
project(discrete-figures-app VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

# Graphical application using Raylib as graphics library.
file(GLOB_RECURSE src_app CONFIGURE_DEPENDS ./*)
add_executable(discrete-figures-app ${src_app})
target_link_libraries(discrete-figures-app PUBLIC raylib)

