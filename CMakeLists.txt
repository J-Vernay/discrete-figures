cmake_minimum_required(VERSION 3.0.0)
project(discrete-figures VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(raylib REQUIRED)

# Graphical application made with files from src/application/*, using FLTK as graphics library.
file(GLOB_RECURSE src_app CONFIGURE_DEPENDS src/application/*)
add_executable(discrete-figures ${src_app})
target_link_libraries(discrete-figures PUBLIC raylib)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
