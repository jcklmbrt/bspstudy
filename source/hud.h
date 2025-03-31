
#ifndef _HUD_H
#define _HUD_H

#include "gl.h"
#include <limits.h>
#include <stdbool.h>
#include <stb_truetype.h>

#define FONT_SIZE 12
#define FONT_RANGE CHAR_MAX
#define FONTATLAS_WIDTH 512
#define FONTATLAS_HEIGHT 512

// if we stick to rects then we will have 3/2 indices for each vertex
#define HUD_MAXVTX (1024 * 16)
#define HUD_MAXIDX ((HUD_MAXVTX * 3) / 2)

typedef struct {
	float x, y;
	float r, g, b, a;
	float s, t;
} hudvtx_t;

typedef struct {
	GLuint fontatlas;
	stbtt_packedchar pc[FONT_RANGE];

	GLuint shader;
	GLuint vbo;
	GLuint vao;
	GLuint ibo;

	GLuint u_ortho;
	
	GLuint *idx;
	size_t ni;
	
	hudvtx_t *vtx;
	size_t nv;
} hud_t;

bool hud_init(hud_t *hud);
void hud_clear(hud_t *hud);
void hud_drawelems(const hud_t *hud);
void hud_onresize(hud_t *hud, float w, float h);
void hud_strsize(hud_t *hud, float *w, float *h, const char *s, size_t len);
int hud_puts(hud_t *hud, float x, float y, const char *s, size_t len);

#endif
