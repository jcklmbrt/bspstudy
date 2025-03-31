cmake_minimum_required(VERSION 3.5)
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
  "source/entity.c"
  "source/hud.h"
  "source/hud.c"
  "source/lightmap.c"
  "source/lightmap.h")

set(CMAKE_C_STANDARD 11)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if(MSVC)
  #target_compile_options(bspstudy PRIVATE /W4 /WX)
else()
  target_compile_options(bspstudy PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL REQUIRED)

set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/3rd_party/glad")
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_33 REPRODUCIBLE LOADER API gl:core=3.3)

add_subdirectory("3rd_party/glfw" EXCLUDE_FROM_ALL)
target_include_directories(bspstudy PUBLIC 3rd_party/glfw/include)

if(WIN32)
	target_link_libraries(bspstudy glfw opengl32 glu32 glad_gl_core_33)
else()
	find_package(OpenGL REQUIRED)
	target_link_libraries(bspstudy glfw OpenGL::GL OpenGL::GLU m glad_gl_core_33)
endif()

target_include_directories(bspstudy PUBLIC
  source/
  3rd_party/stb
)
