#ifndef _MY_GL_H
#define _MY_GL_H

#include <vector>
#include <memory>

#include <glad/gl.h>
#include <stdbool.h>
#include <stdint.h>

#include "bsp.hpp"
#include "camera.hpp"

#include <stb_rect_pack.h>

struct lightmap_t;

struct render_t {
	static constexpr size_t ATLAS_WIDTH = 1024;
	static constexpr size_t ATLAS_HEIGHT = 1024;
public:
	bool init(const bsp_t &bsp, const lightmap_t &lm);
	~render_t();
	size_t drawworld(const bsp_t &bsp, const cam_t &cam);
	void drawmodel(const bsp_t &bsp, const cam_t &cam, size_t index);
private:
	bool loadtextures(const bsp_t &bsp);
	bool setupvertices(const bsp_t &bsp, const lightmap_t &lm);
	// gl objects
	GLuint m_shader = 0;
	GLuint m_vbo = 0;
	GLuint m_ibo = 0;
	GLuint m_vao = 0;
	// uniforms
	GLint u_mvp = 0;
	// offset into the vertex buffer for each face
	// size will be dface_t::numedges
	int32_t *m_vtxlut = nullptr;
	// texture atlas for each texture in LUMP_TEXTURES
	// indexed by texinfo_t::miptex, repeat elements means same atlas.
	std::vector<GLuint> m_textures;
	// rect within atlas for each texture
	std::vector<stbrp_rect> m_rects;
	//
	std::vector<GLuint> m_idx;
};

GLuint gl_compileshaders(const char *vs_src, const char *fs_src);

#endif
