#ifndef _LIGHTMAP_H
#define _LIGHTMAP_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp.hpp"
#include "render.hpp"

#define LIGHTMAP_TEXTURE_UNIT GL_TEXTURE1
#define LIGHTMAP_WIDTH 1024
#define LIGHTMAP_HEIGHT 1024
#define LIGHTMAP_SPACING 4

struct stbrp_rect;

struct lmface_t {
	// basically a stripped down version of msurface_t
	// I think as I add more features I'm slowly going to reinvent msurface_t LOL.
	int32_t texturemins[2];
	int32_t extents[2];
};

struct lightmap_t {
	bool init(const bsp_t &bsp);
	~lightmap_t();

	GLuint texture = 0;
	stbrp_rect *rects = nullptr;
	lmface_t *faces = nullptr;
};


#endif
