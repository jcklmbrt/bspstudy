cmake_minimum_required(VERSION 3.0)
project(bspstudy C)

add_executable(bspstudy "source/main.c"  "source/goldsrc_bspfile.h"  "source/goldsrc_bspfile.c")

set(CMAKE_C_STANDARD 99)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

if( NOT DEFINED OPENGL_GLU_FOUND ) 
  message(FATAL_ERROR "GLU not found")
endif()

target_include_directories(bspstudy PUBLIC
        source/
        glfw/include
)

target_link_directories(bspstudy PUBLIC ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIRS})
target_link_libraries(bspstudy ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLFW3_LIBRARY})

