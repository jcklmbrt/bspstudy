
#ifndef _HUD_H
#define _HUD_H

#include <limits.h>
#include <stdbool.h>
#include <vector>
#include <glm/glm.hpp>

#include "render.hpp"

#include <stb_truetype.h>

#define FONT_SIZE 12
#define FONT_RANGE CHAR_MAX
#define FONTATLAS_WIDTH 512
#define FONTATLAS_HEIGHT 512

// if we stick to rects then we will have 3/2 indices for each vertex
#define HUD_MAXVTX (1024 * 16)
#define HUD_MAXIDX ((HUD_MAXVTX * 3) / 2)


struct hudvtx_t {
	float x, y;
	float r, g, b, a;
	float s, t;
};


struct hud_t {
	GLuint fontatlas;
	stbtt_packedchar pc[FONT_RANGE];

	GLuint shader;
	GLuint vbo;
	GLuint vao;
	GLuint ibo;

	glm::mat4 ortho;
	GLuint u_ortho;

	std::vector<GLuint> idx;
	std::vector<hudvtx_t> vtx;
	
	bool init();
	~hud_t();
	void clear();
	void drawelems();
	void onresize(float w, float h);
	void strsize(float &w, float &h, const char *s, size_t len) const;
	int puts(float x, float y, const char *s, size_t len);
};

#endif
