#include <stddef.h>

#include "gl.h"
#include "glad/gl.h"
#include "v_math.h"
#include "hud.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>


static void setvertex(hudvtx_t *vtx, float x, float y, float s, float t)
{
	vtx->x = x;
	vtx->y = y;
	vtx->r = 1.0f;
	vtx->g = 1.0f;
	vtx->b = 1.0f;
	vtx->a = 1.0f;
	vtx->s = s;
	vtx->t = t;
}


void hud_free(hud_t *hud)
{
	free(hud->vtx);
	free(hud->idx);
	glDeleteShader(hud->shader);
	glDeleteBuffers(1, &hud->vbo);
	glDeleteBuffers(1, &hud->ibo);
	glDeleteVertexArrays(1, &hud->vao);
}


bool hud_init(hud_t *hud)
{
	static const char *vs_src =
		"#version 330 core\n"
		
		"layout (location = 0) in vec2 pos;\n"
		"layout (location = 1) in vec4 color;\n"
		"layout (location = 2) in vec2 uv;\n"
		
		"out vec4 a_color;\n"
		"out vec2 a_uv;\n"
		
		"uniform mat4 u_ortho;\n"

		"void main() {\n"
		"gl_Position = u_ortho * vec4(pos, 0.0, 1.0);\n"
		"a_color = color;\n"
		"a_uv = uv;\n"
		"}";

	static const char *fs_src =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec4 a_color;\n"
		"in vec2 a_uv;\n"
		"uniform sampler2D u_texture;\n"
		"void main() {"
		"vec4 tx_color = texture(u_texture, a_uv) * a_color;\n"
		"float do_tx = 1.0f - floor(a_uv.x * a_uv.y)\n;"
		"FragColor = texture(u_texture, a_uv);\n"
		"}";

	hud->shader = gl_compileshaders(vs_src, fs_src);
	glUseProgram(hud->shader);

	hud->u_ortho = glGetUniformLocation(hud->shader, "u_ortho");

	GLuint u_texture = glGetUniformLocation(hud->shader, "u_texture");
	glUniform1i(u_texture, 0);

	glGenBuffers(1, &hud->vbo);
	glGenBuffers(1, &hud->ibo);
	glGenVertexArrays(1, &hud->vao);

	glBindVertexArray(hud->vao);
	glBindBuffer(GL_ARRAY_BUFFER, hud->vbo);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, x));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, r));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, s));
	glEnableVertexAttribArray(2);

	hud->vtx = malloc(HUD_MAXVTX * sizeof(hudvtx_t));
	hud->idx = malloc(HUD_MAXIDX * sizeof(GLuint));
	hud_clear(hud);

#ifndef _WIN32
	const char *font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#else
	const char *font = "C:\\Windows\\Fonts\\DejavuSans.ttf";
#endif
	FILE *fp = fopen(font, "rb");
	if(fp == NULL) {
		fprintf(stderr, "Failed to open font: %s\n", font);
	}
	
	fseek(fp, 0, SEEK_END);
	size_t ttfsize = ftell(fp);
	uint8_t *rawttf = malloc(ttfsize);
	
	fseek(fp, 0, SEEK_SET);
	fread(rawttf, 1, ttfsize, fp);
	fclose(fp);

	stbtt_pack_context ctx;
	const float fontsize =  16.0f;
	uint8_t *alpha = malloc(FONTATLAS_WIDTH * FONTATLAS_HEIGHT);
	uint8_t *rgba = malloc(FONTATLAS_WIDTH * FONTATLAS_HEIGHT * 4);
	stbtt_PackBegin(&ctx, alpha, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, 1, NULL);
	stbtt_PackFontRange(&ctx, rawttf, 0, fontsize, 0, FONT_RANGE, hud->pc);
	stbtt_PackEnd(&ctx);

	for(int i = 0; i < FONTATLAS_WIDTH * FONTATLAS_HEIGHT; i++) {
		rgba[i * 4 + 0] = 0xFF;
		rgba[i * 4 + 1] = 0xFF;
		rgba[i * 4 + 2] = 0xFF;
		rgba[i * 4 + 3] = alpha[i];
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &hud->fontatlas);
	glBindTexture(GL_TEXTURE_2D, hud->fontatlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	
	free(rawttf);
	free(alpha);
	free(rgba);

	return true;
}

void hud_clear(hud_t *hud)
{
	hud->nv = 0;
	hud->ni = 0;
}


void hud_drawelems(const hud_t *hud)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glUseProgram(hud->shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hud->fontatlas);

	glUniformMatrix4fv(hud->u_ortho, 1, GL_FALSE, (const GLfloat *)hud->ortho);
	
	glBindVertexArray(hud->vao);
	glBindBuffer(GL_ARRAY_BUFFER, hud->vbo);
	glBufferData(GL_ARRAY_BUFFER, hud->nv * sizeof(hudvtx_t), hud->vtx, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hud->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, hud->ni * sizeof(GLuint), hud->idx, GL_STATIC_DRAW);
	glDrawElements(GL_TRIANGLES, hud->ni, GL_UNSIGNED_INT, 0);
}


void hud_strsize(hud_t *hud, float *w, float *h, const char *s, size_t len)
{
	*w = 0.0f;
	*h = FONT_SIZE;
	float dx = 0.0f;
	for(size_t i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(hud->pc, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, s[i], &dx, h, &q, 0);
		if(s[i] == '\n') {
			*h += FONT_SIZE;
			dx = 0.0f;
		}
		*w = fmax(*w, dx);
	}
}


int hud_puts(hud_t *hud, float x, float y, const char *s, size_t len)
{
	float dx = x;
	for(size_t i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(hud->pc, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, s[i], &dx, &y, &q, 0);

		if(s[i] == '\n') {
			dx = x;
			y += FONT_SIZE;
			continue;
		}
		
		if(hud->nv + 4 > HUD_MAXVTX || hud->ni + 6 > HUD_MAXIDX) {
			fprintf(stderr, "warning: trying to draw too many strings at once, truncating.\n");
			continue;
		}      
		
		int32_t nv = hud->nv;
		setvertex(&hud->vtx[hud->nv++], q.x0, q.y0, q.s0, q.t0);
		setvertex(&hud->vtx[hud->nv++], q.x1, q.y0, q.s1, q.t0);
		setvertex(&hud->vtx[hud->nv++], q.x0, q.y1, q.s0, q.t1);
		setvertex(&hud->vtx[hud->nv++], q.x1, q.y1, q.s1, q.t1);

		hud->idx[hud->ni++] = nv + 0;
		hud->idx[hud->ni++] = nv + 1;
		hud->idx[hud->ni++] = nv + 2;
		hud->idx[hud->ni++] = nv + 1;
		hud->idx[hud->ni++] = nv + 3;
		hud->idx[hud->ni++] = nv + 2;
	}

	return len;
}


void hud_onresize(hud_t *hud, float w, float h)
{
	m4ortho2d(0, w, h, 0, hud->ortho);
}
