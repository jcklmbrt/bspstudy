#ifndef _MY_GL_H
#define _MY_GL_H

#include <glad/gl.h>
#include <stb_truetype.h>

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"
#include "camera.h"

typedef struct lightmap_s lightmap_t;

bool gl_init(const bsp_t *bsp, const lightmap_t *lm);
void gl_free(const bsp_t *bsp);
void gl_renderfaces(const bsp_t *bsp, const cam_t *cam);
void gl_onresize(float w, float h);
GLuint gl_compileshaders(const char *vs_src, const char *fs_src);
bool gl_world2screen(const cam_t *cam, const float world[3], float w, float h, float *x, float *y);

#endif
