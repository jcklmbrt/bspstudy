#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "glad/gl.h"
#include "v_math.h"

#include "lightmap.h"
#include "camera.h"
#include "bsp.h"
#include "wad.h"
#include "gl.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#include <GLFW/glfw3.h>


typedef struct {
	float x, y, z;
	float s, t;
	float ls, lt; // lightmap coords
} rvtx_t;


static void miptexrgba(const miptex_t *mt, uint8_t *rgba)
{
	uint8_t *miptex_p = (uint8_t *)mt;
	int32_t width = mt->width;
	int32_t height = mt->height;
	
	uint8_t *palette = miptex_p + mt->offsets[3] + (width / 8) * (height / 8) + 2;
	uint8_t *mip = miptex_p + mt->offsets[0];
	
	for(int i = 0; i < height * width; i++) {
		int32_t p = mip[i] * 3;
		rgba[i * 4 + 0] = palette[p + 0];
		rgba[i * 4 + 1] = palette[p + 1];
		rgba[i * 4 + 2] = palette[p + 2];
		// https://developer.valvesoftware.com/wiki/Texture_prefixes
		if(mip[i] == 255 && mt->name[0] == '{') {
			for(int j = 0; j < 4; j++) {
				rgba[i * 4 + j] = 0x0;
			}
		} else {
			rgba[i * 4 + 3] = 0xFF;
		}
	}
}


static bool r_loadtextures(rctx_t *r, const bsp_t *bsp)
{
	uint8_t *rgba = NULL;
	const char *wadname = bsp_wadname(bsp);
	
	wad_t *wad = NULL;
	wad = wad_open(wadname);
	if(wad == NULL) {
		fprintf(stderr, "Failed to open wad file %s\n", wadname);
		wad = wad_open("halflife.wad");
		if(wad == NULL) {
			goto bad;
		}
	}

	r->textures = calloc(bsp->miphdr->nummiptex, sizeof(GLuint));
	if(r->textures == NULL) {
		goto bad;
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(bsp->miphdr->nummiptex, r->textures);

	int32_t maxdim = 0;

	for(int32_t i = 0; i < bsp->miphdr->nummiptex; i++) {
		if(bsp->miphdr->dataofs[i] == 0 || bsp->miphdr->dataofs[i] == -1) {
			continue;
		}

		miptex_t *miptex = (miptex_t *)(bsp->textures + bsp->miphdr->dataofs[i]);

		int32_t dim = miptex->width * miptex->height;
		
		if(miptex->offsets[0] == 0) {
			miptex = wad_getmiptex(wad, miptex->name);
			if(miptex != NULL) {
				dim = miptex->width * miptex->height;
			}
		}

		if(maxdim < dim) {
			maxdim = dim;
		}
	}
	
	rgba = malloc(maxdim * 4);
	if(rgba == NULL) {
		goto bad;
	}
	
	for(int32_t i = 0; i < bsp->miphdr->nummiptex; i++) {
		if(bsp->miphdr->dataofs[i] == 0 || bsp->miphdr->dataofs[i] == -1) {
			continue;
		}

		miptex_t *miptex = (miptex_t *)(bsp->textures + bsp->miphdr->dataofs[i]);

		if(miptex->offsets[0] == 0) {
			// texture is stored in WAD file
			const char *name = miptex->name;
			miptex = wad_getmiptex(wad, name);
			if(miptex == NULL) {
				fprintf(stderr, "%s: failed to find texture %s\n", wadname, name);
				miptex = wad_getmiptex(wad, "aaatrigger");
				if(miptex == NULL) {
					continue;
				}
			}
		}
		miptexrgba(miptex, rgba);
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &r->textures[i]);
		glBindTexture(GL_TEXTURE_2D, r->textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, miptex->width, miptex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	}	
	wad_close(wad);
	return true;
bad:
	wad_close(wad);
	free(r->textures);
	free(rgba);
	return false;
}


static bool r_setupvertices(rctx_t *r, const bsp_t *bsp, const lightmap_t *lm)
{
	int32_t numvertices = 0;
	for(int32_t i = 0; i < bsp->numfaces; i++) {
		dface_t *face = &bsp->faces[i];
		numvertices += face->numedges;
	}

	rvtx_t *vertices = calloc(numvertices, sizeof(rvtx_t));

	r->vtxlookup = calloc(bsp->numfaces, sizeof(int32_t));

	int32_t nv = 0;

	for(int32_t f = 0; f < bsp->numfaces; f++) {
		dface_t *face = &bsp->faces[f];
		texinfo_t *texinfo = &bsp->texinfo[face->texinfo];
		int32_t mipofs = bsp->miphdr->dataofs[texinfo->miptex];
		miptex_t *miptex = (miptex_t *)(bsp->textures + mipofs);

		r->vtxlookup[f] = nv; 

		for(int32_t i = 0; i < face->numedges; i++) {
		
			int32_t edge = bsp->surfedges[face->firstedge + i];

			int32_t v;
			if(edge >= 0) {
				v = bsp->edges[edge].v[0];
			} else {
				v = bsp->edges[-edge].v[1];
			}

			dvertex_t vtx = bsp->vertices[v];
		
			float s = v3dot(vtx.point, texinfo->vecs[0]) + texinfo->vecs[0][3];
			float t = v3dot(vtx.point, texinfo->vecs[1]) + texinfo->vecs[1][3];

			float lm_s = 1.0f;
			float lm_t = 1.0f;

			if(face->lightmapoffset > 0) {
				lm_s = (s - lm->faces[f].texturemins[0]) + lm->rects[f].x * 16.0f + 8.0f;
				lm_t = (t - lm->faces[f].texturemins[1]) + lm->rects[f].y * 16.0f + 8.0f;
			
				lm_s /= LIGHTMAP_WIDTH * 16.0f;
				lm_t /= LIGHTMAP_HEIGHT * 16.0f;
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
	glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
	glBufferData(GL_ARRAY_BUFFER, numvertices * sizeof(rvtx_t), vertices, GL_STATIC_DRAW);
	free(vertices);
	return true;
}

size_t r_world(rctx_t *r, const bsp_t *bsp, const cam_t *cam)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	float mvp[4][4];
	m4mult(cam->proj, cam->view, mvp);

	size_t size = 0;
	
	glUseProgram(r->shader);
	glUniformMatrix4fv(r->u_mvp, 1, GL_FALSE, (const GLfloat *)mvp);

	glBindVertexArray(r->vao);
	glBindBuffer(GL_ARRAY_BUFFER, r->vbo);

	for(int32_t i = 0; i < bsp->miphdr->nummiptex; i++) {
		
		size_t ni = 0;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, r->textures[i]);
		
		for(int32_t f = 0; f < bsp->numfaces; f++) {
			if(isbitset(&cam->texturebits[i * ((bsp->numfaces + 7) / 8)], f)) {
			dface_t *face = &bsp->faces[f];

			int32_t v = r->vtxlookup[f];
			for(int32_t e = 0; e < face->numedges; e++) {
				if(e >= 2) {
					r->idx[ni++] = v;
					r->idx[ni++] = (v + e) - 1;
					r->idx[ni++] = (v + e);
				}
			}
			}
		}
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), r->idx, GL_STATIC_DRAW);
		glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_INT, 0);

		size += ni;
	}

	return size;
}

void r_model(rctx_t *r, const bsp_t *bsp, const cam_t *cam, size_t index)
{
	dmodel_t *mdl = &bsp->models[index];
	
	float mvp[4][4], translation[4][4];
	m4translate(mdl->origin, translation);
	m4mult(cam->proj, cam->view, mvp);
	m4mult((const float(*)[])mvp, (const float(*)[])translation, mvp);

	glUseProgram(r->shader);
	glUniformMatrix4fv(r->u_mvp, 1, GL_FALSE, (const GLfloat *)mvp);

	int32_t nc = 0;
	int16_t children[128];

	children[nc++] = mdl->headnode[0];

	while(nc > 0) {
		int16_t child = children[--nc];
		if(child < 0) {
			dleaf_t *leaf = &bsp->leaves[~child];
			if(!boxinfrustum((float (*)[])cam->frustum, leaf->mins, leaf->maxs)) {
				continue;
			}
			for(int32_t j = 0; j < leaf->nummarksurfaces; j++) {
				int32_t f = bsp->marksurfaces[leaf->firstmarksurface + j];
				dface_t *face = &bsp->faces[f];
				texinfo_t *texinfo = &bsp->texinfo[face->texinfo];

				size_t ni = 0;
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, r->textures[texinfo->miptex]);
				int32_t v = r->vtxlookup[f];
				for(int32_t e = 0; e < face->numedges; e++) {
					r->idx[ni++] = v;
					r->idx[ni++] = (v + e) - 1;
					r->idx[ni++] = (v + e);
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), r->idx, GL_STATIC_DRAW);
				glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_INT, 0);

			}
		} else {
			dnode_t *node = &bsp->nodes[child];
			dplane_t *plane = &bsp->planes[node->plane];

			if(!boxinfrustum((float (*)[])cam->frustum, node->mins, node->maxs)) {
				continue;
			}
			
			float dist = 0;
			switch(plane->type) {
			case PLANE_X: dist = cam->origin[0] - plane->dist; break;
			case PLANE_Y: dist = cam->origin[1] - plane->dist; break;
			case PLANE_Z: dist = cam->origin[2] - plane->dist; break;
			default:
				dist = v3dot(plane->normal, cam->origin) - plane->dist;
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


void r_free(rctx_t *r, const bsp_t *bsp)
{
	free(r->vtxlookup);
	free(r->idx);
	glDeleteBuffers(1, &r->vbo);
	glDeleteBuffers(1, &r->ibo);
	glDeleteVertexArrays(1, &r->vao);
	glDeleteTextures(bsp->miphdr->nummiptex, r->textures);
	glDeleteProgram(r->shader);
}


bool r_init(rctx_t *r, const bsp_t *bsp, const lightmap_t *lm)
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
	
	r->shader = gl_compileshaders(vs_src, fs_src);
	if(r->shader == 0) {
		goto bad;
	}

	glUseProgram(r->shader);
	r->u_mvp = glGetUniformLocation(r->shader, "mvp");
	GLuint u_texture = glGetUniformLocation(r->shader, "u_texture");
	GLuint u_lightmap = glGetUniformLocation(r->shader, "u_lightmap");
	
	glUniform1i(u_texture, 0);
	glUniform1i(u_lightmap, 1);

	glGenBuffers(1, &r->vbo);
	glGenBuffers(1, &r->ibo);
	glGenVertexArrays(1, &r->vao);

	glBindVertexArray(r->vao);
	glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, x));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, s));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(rvtx_t), (void *)offsetof(rvtx_t, ls));
	glEnableVertexAttribArray(2);

	r->idx = malloc(bsp->numvertices * sizeof(int32_t));

	if(!r_loadtextures(r, bsp)) {
		goto bad;
	}
	
	if(!r_setupvertices(r, bsp, lm)) {
		goto bad;
	}
	
	return true;
bad:
	r_free(r, bsp);
	return false;
}
