#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/gl.h>

#include "lightmap.hpp"
#include "camera.hpp"
#include "bsp.hpp"
#include "wad.hpp"
#include "render.hpp"

#include <GLFW/glfw3.h>

#include <stb_rect_pack.h>

struct rvtx_t {
	float x, y, z;
	float s, t;
	float ls, lt; // lightmap coords
};


bool render_t::loadtextures(const bsp_t &bsp)
{
	const char *wadname = bsp.wadname();
	
	wad_t wad;
	if(!wad.open(wadname)) {
		fprintf(stderr, "Failed to open wad file %s\n", wadname);
		wadname = "halflife.wad";
		if(!wad.open(wadname)) {
			return false;
		}
	}

	m_textures.resize(bsp.m_mipofs.size());
	if(m_textures.data() == nullptr) {
		return false;
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(bsp.m_mipofs.size(), m_textures.data());

	miptex_t mt;
	
	for(size_t i = 0; i < bsp.m_mipofs.size(); i++) {
		if(bsp.m_mipofs[i] == 0 || bsp.m_mipofs[i] == -1) {
			continue;
		}

		if(!mt.load(bsp.m_textures.data() + bsp.m_mipofs[i])) {
			// texture is stored in WAD file
			if(!mt.load(wad, mt.name)) {
				std::cerr << wadname << ": failed to find texture " << mt.name << std::endl;
				if(!mt.load(wad, "aaatrigger")) {
					continue;
				}
			}
		}

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &m_textures[i]);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mt.width, mt.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mt.rgba.data());
	}
	
	return true;
}


bool render_t::setupvertices(const bsp_t &bsp, const lightmap_t &lm)
{
	int32_t numvertices = 0;
	for(size_t i = 0; i < bsp.m_faces.size(); i++) {
		const dface_t *face = &bsp.m_faces[i];
		numvertices += face->numedges;
	}

	rvtx_t *vertices = new rvtx_t[numvertices];

	m_idx.reserve(numvertices);
	m_vtxlut = new int32_t[bsp.m_faces.size()];

	int32_t nv = 0;

	for(size_t f = 0; f < bsp.m_faces.size(); f++) {
		const dface_t *face = &bsp.m_faces[f];
		const texinfo_t *texinfo = &bsp.m_texinfo[face->texinfo];
		int32_t mipofs = bsp.m_mipofs[texinfo->miptex];
		miptex_t *miptex = (miptex_t *)(bsp.m_textures.data() + mipofs);

		m_vtxlut[f] = nv; 

		for(int32_t i = 0; i < face->numedges; i++) {
		
			int32_t edge = bsp.m_surfedges[face->firstedge + i];

			int32_t v;
			if(edge >= 0) {
				v = bsp.m_edges[edge].v[0];
			} else {
				v = bsp.m_edges[-edge].v[1];
			}

			dvertex_t vtx = bsp.m_vertices[v];

			float s = texinfo->vecs[0][0] * vtx.point[0] +
				  texinfo->vecs[0][1] * vtx.point[1] +
				  texinfo->vecs[0][2] * vtx.point[2] +
				  texinfo->vecs[0][3];

			float t = texinfo->vecs[1][0] * vtx.point[0] +
				  texinfo->vecs[1][1] * vtx.point[1] +
				  texinfo->vecs[1][2] * vtx.point[2] +
				  texinfo->vecs[1][3];
			
			float lm_s = 1.0f;
			float lm_t = 1.0f;

			if(face->lightmapoffset > 0) {
				lm.texcoords(f, s, t, lm_s, lm_t);
			}

			s /= miptex->width;
			t /= miptex->height;

			vertices[nv].x = vtx.point[0];
			vertices[nv].y = vtx.point[1];
			vertices[nv].z = vtx.point[2];
			vertices[nv].s = s;
			vertices[nv].t = t;
			vertices[nv].ls = lm_s;
			vertices[nv].lt = lm_t;
			nv++;
		}
	}
	/* all our data is now in the vertex buffer. we don't have to update it */
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, numvertices * sizeof(rvtx_t), vertices, GL_STATIC_DRAW);

	delete[] vertices;

	return true;
}



size_t render_t::drawworld(const bsp_t &bsp, const cam_t &cam)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	glm::mat4 mvp = cam.m_proj * cam.m_view;

	size_t size = 0;
	
	glUseProgram(m_shader);
	glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	for(size_t i = 0; i < bsp.m_mipofs.size(); i++) {
		m_idx.clear();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);

		uint64_t *facebits = &cam.m_texturebits[i * ((bsp.m_faces.size() + 63) / 64)];
		
		for(size_t f = 0; f < ((bsp.m_faces.size() + 63) / 64); f++) {
			uint64_t qword = facebits[f];
			if(qword == 0) {
				/* entire qword is empty, we can skip it */
			} else for(uint64_t b = 0; b < 64; b++) {
				if(qword & (1ull << b)) {
					const dface_t *face = &bsp.m_faces[f * 64 + b];
					int32_t v = m_vtxlut[f * 64 + b];
					for(int32_t e = 0; e < face->numedges; e++) {
						if(e >= 2) {
							m_idx.push_back(v);
							m_idx.push_back(v + e - 1);
							m_idx.push_back(v + e);
						}
					}
					size++;
				}
			}			
		}
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idx.size() * sizeof(GLuint), m_idx.data(), GL_STATIC_DRAW);
		glDrawElements(GL_TRIANGLES, m_idx.size(), GL_UNSIGNED_INT, 0);
	}

	return size;
}

void render_t::drawmodel(const bsp_t &bsp, const cam_t &cam, size_t index)
{
	const dmodel_t *mdl = &bsp.m_models[index];
	
	glm::mat4 mvp = cam.m_proj * cam.m_view;
	mvp = glm::translate(mvp, glm::vec3(mdl->origin[0], mdl->origin[1], mdl->origin[2]));

	glUseProgram(m_shader);
	glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

	int32_t nc = 0;
	int16_t children[128];

	float mins[3];
	float maxs[3];
	
	children[nc++] = mdl->headnode[0];

	while(nc > 0) {
		int16_t child = children[--nc];
		if(child < 0) {
			const dleaf_t *leaf = &bsp.m_leaves[~child];
			for(int i = 0; i < 3; i++) {
				mins[i] = (float)leaf->mins[i] + mdl->origin[i];
				maxs[i] = (float)leaf->maxs[i] + mdl->origin[i];
			}
			if(!cam.boxinfrustum(mins, maxs)) {
				continue;
			}
			for(int32_t j = 0; j < leaf->nummarksurfaces; j++) {
				int32_t f = bsp.m_marksurfaces[leaf->firstmarksurface + j];
				const dface_t *face = &bsp.m_faces[f];
				const texinfo_t *texinfo = &bsp.m_texinfo[face->texinfo];
				m_idx.clear();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_textures[texinfo->miptex]);
				int32_t v = m_vtxlut[f];
				for(int32_t e = 0; e < face->numedges; e++) {
					if(e >= 2) {
						m_idx.push_back(v);
						m_idx.push_back(v + e - 1);
						m_idx.push_back(v + e);
					}
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idx.size() * sizeof(GLuint), m_idx.data(), GL_STATIC_DRAW);
				glDrawElements(GL_TRIANGLES, m_idx.size(), GL_UNSIGNED_INT, 0);
			}
		} else {
			const dnode_t *node = &bsp.m_nodes[child];
			const dplane_t *plane = &bsp.m_planes[node->plane];

			for(int i = 0; i < 3; i++) {
				mins[i] = (float)node->mins[i] + mdl->origin[i];
				maxs[i] = (float)node->maxs[i] + mdl->origin[i];
			}
			
			if(!cam.boxinfrustum(mins, maxs)) {
				continue;
			}
			
			float dist = 0;
			switch(plane->type) {
			case PLANE_X: dist = cam.m_origin[0] - plane->dist; break;
			case PLANE_Y: dist = cam.m_origin[1] - plane->dist; break;
			case PLANE_Z: dist = cam.m_origin[2] - plane->dist; break;
			default:
				dist = plane->normal[0] * cam.m_origin[0] +
					plane->normal[1] * cam.m_origin[1] +
					plane->normal[2] * cam.m_origin[2]
					-plane->dist;
				break;
			}
			if(nc > 128) {
				fprintf(stderr, "r_model: stack overflow, skipping node %d\n", child);
				continue;
			}
			children[nc++] = node->children[dist > 0 ? 0 : 1];
			children[nc++] = node->children[dist > 0 ? 1 : 0];
		}
	}
}

GLuint gl_compileshaders(const char *vs_src, const char *fs_src)
{
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_src, NULL);
	glCompileShader(vs);

	GLint success;
	char infolog[512];
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(vs, 512, NULL, infolog);
		fprintf(stderr, "%s", infolog);
		return 0;
	}

	GLuint fs;
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_src, NULL);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(fs, 512, NULL, infolog);
		fprintf(stderr, "%s", infolog);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(program, 512, NULL, infolog);
		fprintf(stderr, "%s", infolog);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}


render_t::~render_t()
{
	delete[] m_vtxlut;
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ibo);
	glDeleteVertexArrays(1, &m_vao);
	glActiveTexture(GL_TEXTURE0);
	glDeleteTextures(m_textures.size(), m_textures.data());
	glDeleteProgram(m_shader);
}


bool render_t::init(const bsp_t &bsp, const lightmap_t &lm)
{
	static const char *vs_src =
		"#version 330 core\n"
		
		"layout (location = 0) in vec3 pos;\n"
		"layout (location = 1) in vec2 uv;\n"
		"layout (location = 2) in vec2 lightmap;\n"
		
		"out vec2 a_lightmap;\n"
		"out vec2 a_uv;\n"
		
		"uniform mat4 mvp;\n"

		"void main() {\n"
		
		"gl_Position = mvp * vec4(pos, 1.0);\n"
		"a_lightmap = lightmap;\n"
		"a_uv = uv;\n"
		"}";

	static const char *fs_src =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 a_lightmap;\n"
		"in vec2 a_uv;\n"
		"uniform sampler2D u_texture;\n"
		"uniform sampler2D u_lightmap;\n"
		"void main() {"
		"vec4 tx_color = texture(u_texture, a_uv);"
		"vec4 lm_color = texture(u_lightmap, a_lightmap);"
		"float do_lm = 1.0f - floor(a_lightmap.x * a_lightmap.y);"
		"FragColor = mix(tx_color, tx_color * lm_color, do_lm);\n"
		"}";
	
	m_shader = gl_compileshaders(vs_src, fs_src);
	if(m_shader == 0) {
		return false;
	}

	glUseProgram(m_shader);
	u_mvp = glGetUniformLocation(m_shader, "mvp");
	GLuint u_texture = glGetUniformLocation(m_shader, "u_texture");
	GLuint u_lightmap = glGetUniformLocation(m_shader, "u_lightmap");
	
	glUniform1i(u_texture, 0);
	glUniform1i(u_lightmap, 1);

	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);
	glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, x));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, s));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, ls));
	glEnableVertexAttribArray(2);

	if(!loadtextures(bsp)) {
		return false;
	}
	
	if(!setupvertices(bsp, lm)) {
		return false;
	}
	
	return true;
}
