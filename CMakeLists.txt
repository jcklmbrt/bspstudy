cmake_minimum_required(VERSION 3.0)
project(bspstudy C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(bspstudy
  "source/main.c"
  "source/bsp.h"
  "source/bsp.c"
  "source/wad.h"
  "source/wad.c"
  "source/gl.h"
  "source/gl.c"
  "source/entity.h"
  "source/entity.c")

set(CMAKE_C_STANDARD 11)

set(OpenGL_GL_PREFERENCE "LEGACY")

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

if(NOT DEFINED OPENGL_GLU_FOUND) 
  message(FATAL_ERROR "GLU not found")
endif()

target_include_directories(bspstudy PUBLIC
  source/
  3rd_party/stb
)

target_link_directories(bspstudy PUBLIC ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIRS})
target_link_libraries(bspstudy ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} glfw m)


