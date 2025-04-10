#include <cstddef>
#include <glad/gl.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "hud.hpp"
#include "render.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

hud_t::~hud_t()
{
	glDeleteShader(m_shader);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ibo);
	glDeleteVertexArrays(1, &m_vao);
}


bool hud_t::init()
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
		"float alpha = a_color.a * texture(u_texture, a_uv).r;"
		"FragColor = vec4(a_color.xyz, alpha);\n"
		"}";

	m_shader = gl_compileshaders(vs_src, fs_src);
	glUseProgram(m_shader);

	u_ortho = glGetUniformLocation(m_shader, "u_ortho");

	GLuint u_texture = glGetUniformLocation(m_shader, "u_texture");
	glUniform1i(u_texture, 0);

	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);
	glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, x));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, color));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(hudvtx_t), (void *)offsetof(hudvtx_t, s));
	glEnableVertexAttribArray(2);

	clear();

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
	uint8_t *rawttf = new uint8_t[ttfsize];
	
	fseek(fp, 0, SEEK_SET);
	fread(rawttf, 1, ttfsize, fp);
	fclose(fp);

	stbtt_pack_context ctx;
	const float fontsize =  16.0f;
	uint8_t *data = new uint8_t[FONTATLAS_WIDTH * FONTATLAS_HEIGHT];
	stbtt_PackBegin(&ctx, data, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, 1, NULL);
	stbtt_PackFontRange(&ctx, rawttf, 0, fontsize, 0, FONT_RANGE, m_pc);
	stbtt_PackEnd(&ctx);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_fontatlas);
	glBindTexture(GL_TEXTURE_2D, m_fontatlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, data);

	delete[] rawttf;
	delete[] data;

	return true;
}

void hud_t::clear()
{
	m_idx.clear();
	m_vtx.clear();
}


void hud_t::drawelems()
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glUseProgram(m_shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fontatlas);

	glUniformMatrix4fv(u_ortho, 1, GL_FALSE, glm::value_ptr(m_ortho));
	
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_vtx.size() * sizeof(hudvtx_t), m_vtx.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idx.size() * sizeof(GLuint), m_idx.data(), GL_STATIC_DRAW);
	glDrawElements(GL_TRIANGLES, m_idx.size(), GL_UNSIGNED_INT, 0);
}


void hud_t::strsize(float &w, float &h, const char *s, size_t len) const
{
	w = 0.0f;
	h = FONT_SIZE;
	float dx = 0.0f;
	for(size_t i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(m_pc, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, s[i], &dx, &h, &q, 0);
		if(s[i] == '\n') {
			h += FONT_SIZE;
			dx = 0.0f;
		}
		w = std::max(w, dx);
	}
}


int hud_t::puts(float x, float y, color_t color, const char *s, size_t len)
{
	float dx = x;
	for(size_t i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(m_pc, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, s[i], &dx, &y, &q, 0);

		if(s[i] == '\n') {
			dx = x;
			y += FONT_SIZE;
			continue;
		}
		
		int32_t nv = m_vtx.size();
		m_vtx.push_back({ q.x0, q.y0, color, q.s0, q.t0 });
		m_vtx.push_back({ q.x1, q.y0, color, q.s1, q.t0 });
		m_vtx.push_back({ q.x0, q.y1, color, q.s0, q.t1 });
		m_vtx.push_back({ q.x1, q.y1, color, q.s1, q.t1 });

		m_idx.push_back(nv + 0);
		m_idx.push_back(nv + 1);
		m_idx.push_back(nv + 2);
		m_idx.push_back(nv + 1);
		m_idx.push_back(nv + 3);
		m_idx.push_back(nv + 2);
	}
	return len;
}


void hud_t::onresize(float w, float h)
{
	m_ortho = glm::ortho(0.0f, w, h, 0.0f);
}
