#ifndef _MY_GL_H
#define _MY_GL_H

#include <glad/gl.h>
#include <stb_truetype.h>

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

typedef struct lightmap_s lightmap_t;

int gl_init(void);
void gl_3dmode(void);
void gl_lookat(const float origin[3], float pitch, float yaw);
void gl_rmodel(bsp_t *bsp, const float origin[3], int32_t index);
void gl_end(const bsp_t *bsp, GLuint textures[]);
void gl_onresize(float w, float h);
GLuint gl_compileshaders(const char *vs_src, const char *fs_src);
GLuint *gl_loadtextures(const bsp_t *bsp);
bool gl_world2screen(const float world[3], float w, float h, float *x, float *y);
bool gl_setupvertices(const bsp_t *bsp, const lightmap_t *lm);

#endif
