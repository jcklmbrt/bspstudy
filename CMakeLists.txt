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
find_package(OpenGL REQUIRED)

add_subdirectory("3rd_party/glfw" EXCLUDE_FROM_ALL)
target_include_directories(bspstudy PUBLIC 3rd_party/glfw/include)
if(WIN32)
		target_link_libraries(bspstudy glfw opengl32 glu32)
	else()
		find_package(OpenGL REQUIRED)
		target_link_libraries(bspstudy glfw OpenGL::GL OpenGL::GLU m)
	endif()

target_include_directories(bspstudy PUBLIC
  source/
  3rd_party/stb
)
