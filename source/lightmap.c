#include <string.h>

#include "gl.h"
#include "bsp.h"
#include "v_math.h"
#include "lightmap.h"


// needs to be exactly the same as CalcFaceExtents.
// https://github.com/ValveSoftware/halflife/blob/master/utils/qrad/lightmap.c#L563
static void faceextents(const bsp_t *bsp, const dface_t *face, lmface_t *s)
{
	texinfo_t *texinfo = &bsp->texinfo[face->texinfo];

	float mins[2];
	float maxs[2];
	
	bool first = true;
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
		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


bool lm_init(const bsp_t *bsp, lightmap_t *lm)
{
	stbrp_node *nodes = NULL;
	memset(lm, 0, sizeof(lightmap_t));

	lm->faces = calloc(bsp->numfaces, sizeof(lmface_t));
	if(lm->faces == NULL) {
		goto bad_alloc;
	}

	lm->rects = calloc(bsp->numfaces, sizeof(stbrp_rect));
	if(lm->rects == NULL) {
		goto bad_alloc;
	}

	glActiveTexture(LIGHTMAP_TEXTURE_UNIT);
	glGenTextures(1, &lm->texture);
	glBindTexture(GL_TEXTURE_2D, lm->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	for(int32_t i = 0; i < bsp->numfaces; i++) {
		dface_t *face = &bsp->faces[i];
		if(face->lightmapoffset == -1) {
			continue;
		}
		faceextents(bsp, face, &lm->faces[i]);
		lm->rects[i].id = i;
		lm->rects[i].w = (lm->faces[i].extents[0] / 16) + 1;
		lm->rects[i].h = (lm->faces[i].extents[1] / 16) + 1;
		// HACK: GL_LINEAR will sample from its nearest neigbors.
		// to stop lightmaps interfering with each other we need to add spacing.
		lm->rects[i].w += LIGHTMAP_SPACING;
		lm->rects[i].h += LIGHTMAP_SPACING;
	}

	stbrp_context ctx;
	int numnodes = LIGHTMAP_WIDTH * 2;
	nodes = malloc(numnodes * sizeof(stbrp_node));
	if(nodes == NULL) {
		goto bad_alloc;
	}
	
	stbrp_init_target(&ctx, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, nodes, numnodes);
	stbrp_pack_rects(&ctx, lm->rects, bsp->numfaces);

	for(int32_t i = 0; i < bsp->numfaces; i++) {
		// ^^^ 266: remove the padding for further calculations
		lm->rects[i].w -= LIGHTMAP_SPACING;
		lm->rects[i].h -= LIGHTMAP_SPACING;
		
		dface_t *face = &bsp->faces[i];
		if(lm->rects[i].was_packed && face->lightmapoffset != -1) {
			uint8_t *data = &bsp->lighting[face->lightmapoffset];
			glTexSubImage2D(GL_TEXTURE_2D, 0,
					lm->rects[i].x, lm->rects[i].y,
					lm->rects[i].w, lm->rects[i].h,
					GL_RGB, GL_UNSIGNED_BYTE, data);
		}
	}
	free(nodes);
	return true;
bad_alloc:
	fprintf(stderr, "lightmap: bad alloc");
	/* fallthrough */
//bad:
	free(nodes);
	lm_free(lm);
	return false;
}

void lm_free(lightmap_t *lm)
{
	if(lm == NULL) {
		return;
	}
	free(lm->faces);
	free(lm->rects);
	glDeleteTextures(1, &lm->texture);
}
