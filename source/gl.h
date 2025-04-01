#ifndef _MY_GL_H
#define _MY_GL_H

#include <glad/gl.h>
#include <stb_truetype.h>

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"
#include "camera.h"

typedef struct lightmap_s lightmap_t;

typedef struct {
	// gl objects
	GLuint shader;
	GLuint vbo;
	GLuint ibo;
	GLuint vao;
	// uniforms
	GLint u_mvp;
	// offset into the vertex buffer for each face
	// size will be dface_t::numedges
	int32_t *vtxlookup;
	// array of textures
	// index will be mapped to texinfo_t::miptex
	GLuint *textures;
	// 
	GLuint *idx;
} rctx_t;


GLuint gl_compileshaders(const char *vs_src, const char *fs_src);


bool r_init(rctx_t *r, const bsp_t *bsp, const lightmap_t *lm);
void r_free(rctx_t *r, const bsp_t *bsp);
size_t r_world(rctx_t *r, const bsp_t *bsp, const cam_t *cam);
void r_model(rctx_t *r, const bsp_t *bsp, const cam_t *cam, size_t index);


#endif
