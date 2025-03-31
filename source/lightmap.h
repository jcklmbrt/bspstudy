#ifndef _LIGHTMAP_H
#define _LIGHTMAP_H

#include <stdint.h>
#include <stdbool.h>

#include <stb_rect_pack.h>

#include "bsp.h"
#include "gl.h"

#define LIGHTMAP_TEXTURE_UNIT GL_TEXTURE1
#define LIGHTMAP_WIDTH 1024
#define LIGHTMAP_HEIGHT 1024
#define LIGHTMAP_SPACING 4

typedef struct {
	// basically a stripped down version of msurface_t
	// I think as I add more features I'm slowly going to reinvent msurface_t LOL.
	int32_t texturemins[2];
	int32_t extents[2];
} lmface_t;

typedef struct lightmap_s {
	GLuint texture;
	stbrp_rect *rects;
	lmface_t *faces;
} lightmap_t;


bool lm_init(const bsp_t *bsp, lightmap_t *lm);
void lm_free(lightmap_t *lm);

	
#endif
