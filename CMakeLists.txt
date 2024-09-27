cmake_minimum_required(VERSION 3.29)
project(bspstudy C)

add_executable(bspstudy "source/main.c"  "source/goldsrc_bspfile.h"  "source/goldsrc_bspfile.c")

set(CMAKE_C_STANDARD 99)

target_include_directories(bspstudy PUBLIC
        source/
        glfw/include
)

target_link_directories(bspstudy PUBLIC glfw/lib)
target_link_libraries(bspstudy glfw3 opengl32 glu32)
