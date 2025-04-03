#include <string.h>

#include "render.hpp"
#include "bsp.hpp"
#include "lightmap.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

// needs to be exactly the same as CalcFaceExtents.
// https://github.com/ValveSoftware/halflife/blob/master/utils/qrad/lightmap.c#L563
static void faceextents(const bsp_t &bsp, const dface_t &face, lmface_t &s)
{
	texinfo_t *texinfo = &bsp.texinfo[face.texinfo];

	glm::vec2 mins = {};
	glm::vec2 maxs = {};
	
	bool first = true;
	for(int32_t i = 0; i < face.numedges; i++) {
		int32_t edge = bsp.surfedges[face.firstedge + i];
		int32_t v;
		if(edge >= 0) {
			v = bsp.edges[edge].v[0];
		} else {
			v = bsp.edges[-edge].v[1];
		}
		
		dvertex_t vtx = bsp.vertices[v];

		float s = texinfo->vecs[0][0] * vtx.point[0] +
			texinfo->vecs[0][1] * vtx.point[1] +
			texinfo->vecs[0][2] * vtx.point[2] +
			texinfo->vecs[0][3];

		float t = texinfo->vecs[1][0] * vtx.point[0] +
			texinfo->vecs[1][1] * vtx.point[1] +
			texinfo->vecs[1][2] * vtx.point[2] +
			texinfo->vecs[1][3];
		
		if(first) {
			mins[0] = s;
			maxs[0] = s;
			mins[1] = t;
			maxs[1] = t;
			first = false;
		} else {
			if(s < mins[0]) mins[0] = s;
			if(t < mins[1]) mins[1] = t;
			if(s > maxs[0]) maxs[0] = s;
			if(t > maxs[1]) maxs[1] = t;
		}
	}

	glm::i32vec2 bmins = {};
	glm::i32vec2 bmaxs = {};

	for(int32_t i = 0; i < 2; i++) {
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);
		s.texturemins[i] = bmins[i] * 16;
		s.extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


void lightmap_t::texcoords(int32_t face, float s, float t, float &lm_s, float &lm_t) const
{
	lm_s = (s - m_faces[face].texturemins[0]) + m_rects[face].x * 16.0f + 8.0f;
	lm_t = (t - m_faces[face].texturemins[1]) + m_rects[face].y * 16.0f + 8.0f;
	
	lm_s /= LIGHTMAP_WIDTH * 16.0f;
	lm_t /= LIGHTMAP_HEIGHT * 16.0f;
}


bool lightmap_t::init(const bsp_t &bsp)
{
	m_faces = new lmface_t[bsp.numfaces];
	if(m_faces == nullptr) {
		return false;
	}

	m_rects = new stbrp_rect[bsp.numfaces];
	if(m_rects == nullptr) {
		return false;
	}

	memset(m_rects, 0, sizeof(stbrp_rect) * bsp.numfaces);

	glActiveTexture(LIGHTMAP_TEXTURE_UNIT);
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	for(int32_t i = 0; i < bsp.numfaces; i++) {
		dface_t *face = &bsp.faces[i];
		if(face->lightmapoffset == -1) {
			continue;
		}
		faceextents(bsp, *face, m_faces[i]);
		m_rects[i].id = i;
		m_rects[i].w = (m_faces[i].extents[0] / 16) + 1;
		m_rects[i].h = (m_faces[i].extents[1] / 16) + 1;
		// HACK: GL_LINEAR will sample from its nearest neigbors.
		// to stop lightmaps interfering with each other we need to add spacing.
		m_rects[i].w += LIGHTMAP_SPACING;
		m_rects[i].h += LIGHTMAP_SPACING;
	}

	stbrp_context ctx;
	int numnodes = LIGHTMAP_WIDTH * 16;
	stbrp_node *nodes = new stbrp_node[numnodes];
	if(nodes == nullptr) {
		return false;
	}
	
	stbrp_init_target(&ctx, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, nodes, numnodes);
	stbrp_pack_rects(&ctx, m_rects, bsp.numfaces);

	delete[] nodes;

	for(int32_t i = 0; i < bsp.numfaces; i++) {
		// ^^^ 266: remove the padding for further calculations
		m_rects[i].w -= LIGHTMAP_SPACING;
		m_rects[i].h -= LIGHTMAP_SPACING;
		
		dface_t *face = &bsp.faces[i];
		if(m_rects[i].was_packed && face->lightmapoffset != -1) {
			uint8_t *data = &bsp.lighting[face->lightmapoffset];
			glTexSubImage2D(GL_TEXTURE_2D, 0,
					m_rects[i].x, m_rects[i].y,
					m_rects[i].w, m_rects[i].h,
					GL_RGB, GL_UNSIGNED_BYTE, data);
		}
	}
	return true;
}

lightmap_t::~lightmap_t()
{
	delete[] m_faces;
	delete[] m_rects;
	glActiveTexture(LIGHTMAP_TEXTURE_UNIT);
	glDeleteTextures(1, &m_texture);
}
