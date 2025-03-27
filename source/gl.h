#ifndef _GL_H
#define _GL_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <stdint.h>
#include <GL/gl.h>

#include "bsp.h"
#include "wad.h"

int gl_init(void);
void gl_free(void);
double gl_time(void);

void gl_setupview(void);
void gl_lookat(const float origin[3], float pitch, float yaw);
void gl_rface(bsp_t *bsp, GLuint *gltex, dface_t *face);
void gl_rmodel(bsp_t *bsp, GLuint *gltex, int32_t index);
int gl_printf(float x, float y, const char *fmt, ...);

void gl_clear(float r, float g, float b, float a);
void gl_swapbuffers(void);
void gl_pollevents(void);
int gl_shouldclose(void);

GLuint gl_loadmiptex(miptex_t *miptex);

#endif
