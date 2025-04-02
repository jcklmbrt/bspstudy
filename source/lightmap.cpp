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

	float mins[2];
	float maxs[2];
	
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

	int32_t bmins[2], bmaxs[2];

	for(int32_t i = 0; i < 2; i++) {
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);
		s.texturemins[i] = bmins[i] * 16;
		s.extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


bool lightmap_t::init(const bsp_t &bsp)
{
	faces = new lmface_t[bsp.numfaces];
	if(faces == nullptr) {
		return false;
	}

	rects = new stbrp_rect[bsp.numfaces];
	if(rects == nullptr) {
		return false;
	}

	memset(rects, 0, sizeof(stbrp_rect) * bsp.numfaces);

	glActiveTexture(LIGHTMAP_TEXTURE_UNIT);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	for(int32_t i = 0; i < bsp.numfaces; i++) {
		dface_t *face = &bsp.faces[i];
		if(face->lightmapoffset == -1) {
			continue;
		}
		faceextents(bsp, *face, faces[i]);
		rects[i].id = i;
		rects[i].w = (faces[i].extents[0] / 16) + 1;
		rects[i].h = (faces[i].extents[1] / 16) + 1;
		// HACK: GL_LINEAR will sample from its nearest neigbors.
		// to stop lightmaps interfering with each other we need to add spacing.
		rects[i].w += LIGHTMAP_SPACING;
		rects[i].h += LIGHTMAP_SPACING;
	}

	stbrp_context ctx;
	int numnodes = LIGHTMAP_WIDTH * 16;
	stbrp_node *nodes = new stbrp_node[numnodes];
	if(nodes == nullptr) {
		return false;
	}
	
	stbrp_init_target(&ctx, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, nodes, numnodes);
	stbrp_pack_rects(&ctx, rects, bsp.numfaces);

	delete[] nodes;

	for(int32_t i = 0; i < bsp.numfaces; i++) {
		// ^^^ 266: remove the padding for further calculations
		rects[i].w -= LIGHTMAP_SPACING;
		rects[i].h -= LIGHTMAP_SPACING;
		
		dface_t *face = &bsp.faces[i];
		if(rects[i].was_packed && face->lightmapoffset != -1) {
			uint8_t *data = &bsp.lighting[face->lightmapoffset];
			glTexSubImage2D(GL_TEXTURE_2D, 0,
					rects[i].x, rects[i].y,
					rects[i].w, rects[i].h,
					GL_RGB, GL_UNSIGNED_BYTE, data);
		}
	}
	return true;
}

lightmap_t::~lightmap_t()
{
	delete[] faces;
	delete[] rects;
	glActiveTexture(LIGHTMAP_TEXTURE_UNIT);
	glDeleteTextures(1, &texture);
}
