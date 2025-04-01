#include <string.h>
#include <stdbool.h>

#include "bsp.h"
#include "v_math.h"
#include "entity.h"
#include "camera.h"

static bool getspawn(const bsp_t *bsp, float origin[3], float *yaw)
{
	*yaw = 0;
	origin[0] = origin[1] = origin[2] = 0.0f;
	
	for(int32_t i = 0; i < bsp->numentities; i++) {
		const char *classname = entgets(&bsp->entities[i], "classname");
		if(classname == NULL) {
			continue;
		}
		// get spawn point
		if(!strncmp(classname, "info_player_start", EPAIR_MAX_KEY)) {
			entgetv3(&bsp->entities[i], "origin", origin);
			entgetf(&bsp->entities[i], "angle", yaw);

			*yaw = deg2rad(*yaw);
			return true;
		}
	}
	return false;
}


static void setupfrustum(const float proj[4][4], const float view[4][4], float frustum[6][4])
{
	float clip[4][4];
	m4mult(proj, view, clip);

	frustum[0][0] = clip[0][3] - clip[0][0];
	frustum[0][1] = clip[1][3] - clip[1][0];
	frustum[0][2] = clip[2][3] - clip[2][0];
	frustum[0][3] = clip[3][3] - clip[3][0];

	frustum[1][0] = clip[0][3] + clip[0][0];
	frustum[1][1] = clip[1][3] + clip[1][0];
	frustum[1][2] = clip[2][3] + clip[2][0];
	frustum[1][3] = clip[3][3] + clip[3][0];

	frustum[2][0] = clip[0][3] + clip[0][1];
	frustum[2][1] = clip[1][3] + clip[1][1];
	frustum[2][2] = clip[2][3] + clip[2][1];
	frustum[2][3] = clip[3][3] + clip[3][1];

	frustum[3][0] = clip[0][3] - clip[0][1];
	frustum[3][1] = clip[1][3] - clip[1][1];
	frustum[3][2] = clip[2][3] - clip[2][1];
	frustum[3][3] = clip[3][3] - clip[3][1];

	frustum[4][0] = clip[0][3] - clip[0][2];
	frustum[4][1] = clip[1][3] - clip[1][2];
	frustum[4][2] = clip[2][3] - clip[2][2];
	frustum[4][3] = clip[3][3] - clip[3][2];

	frustum[5][0] = clip[0][3] + clip[0][2];
	frustum[5][1] = clip[1][3] + clip[1][2];
	frustum[5][2] = clip[2][3] + clip[2][2];
	frustum[5][3] = clip[3][3] + clip[3][2];

	for(int32_t i = 0; i < 6; i++) {
		float len = v3len(frustum[i]);
		for(int32_t j = 0; j < 4; j++) {
			frustum[i][j] /= len;
		}
	}
}

void cam_onresize(cam_t *cam, float w, float h)
{
	m4perspective(M_PI_2, w / h, 0.1f, 100000.0f, cam->proj);
	setupfrustum((const float (*)[])cam->proj, (const float (*)[])cam->view, cam->frustum);
	cam->updated = true;
}


bool boxinfrustum(float frustum[6][4], const int16_t mins[3], const int16_t maxs[3])
{
	float pts[8][3] = {
		{ maxs[0], maxs[1], maxs[2] },
		{ mins[0], maxs[1], maxs[2] },
		{ maxs[0], mins[1], maxs[2] },
		{ mins[0], mins[1], maxs[2] },
		{ maxs[0], maxs[1], mins[2] },
		{ mins[0], maxs[1], mins[2] },
		{ maxs[0], mins[1], mins[2] },
		{ mins[0], mins[1], mins[2] }
	};
	
	for(int32_t p = 0; p < 6; p++)
	{
		if(v3dot(frustum[p], pts[0]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[1]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[2]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[3]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[4]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[5]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[6]) + frustum[p][3] > 0) continue;
		if(v3dot(frustum[p], pts[7]) + frustum[p][3] > 0) continue;
		return false;
	}
	
	return true;
}


static bool m4mulv3(float m[4][4], const float in[3], float out[3])
{
	float x = in[0] * m[0][0] + in[1] * m[1][0] + in[2] * m[2][0] + m[3][0];
	float y = in[0] * m[0][1] + in[1] * m[1][1] + in[2] * m[2][1] + m[3][1];
	float z = in[0] * m[0][2] + in[1] * m[1][2] + in[2] * m[2][2] + m[3][2];
	float w = in[0] * m[0][3] + in[1] * m[1][3] + in[2] * m[2][3] + m[3][3];
	
	if(w < 0.001f) {
		return false;
	}

	out[0] = x / w;
	out[1] = y / w;
	out[2] = z / w;

	return true;
}


bool cam_world2screen(const cam_t *cam, const float world[3], float w, float h, float *x, float *y)
{
	float screen[3];
	float mvp[4][4];
	m4mult(cam->proj, cam->view, mvp);

	if(!m4mulv3(mvp, world, screen)) {
		return false;
	}

	*x = (screen[0] + 1.0f) * (0.5f * w);
	*y = (screen[1] + 1.0f) * (0.5f * h);
	*y = h - *y;

	return true;
}


void cam_buildbitsetformodel(const bsp_t *bsp, cam_t *cam, int32_t index)
{	
	dmodel_t *mdl = &bsp->models[index];
	
	memset(cam->texturebits, 0, bsp->miphdr->nummiptex * ((bsp->numfaces + 7) / 8));

	int32_t nc = 0;
	int16_t children[128];
	// hull 0 is used for rendering, the others are for collision
	children[nc++] = mdl->headnode[0];

	while(nc > 0) {
		int16_t child = children[--nc];
		if(child < 0) {
			dleaf_t *leaf = &bsp->leaves[~child];
			int16_t b = (~child) - 1;
			if(isbitset(cam->pvs, b) == false) {
				continue;
			}
			if(!boxinfrustum(cam->frustum, leaf->mins, leaf->maxs)) {
				continue;
			}
			for(int32_t j = 0; j < leaf->nummarksurfaces; j++) {
				int32_t f = bsp->marksurfaces[leaf->firstmarksurface + j];
				dface_t *face = &bsp->faces[f];
				texinfo_t *texinfo = &bsp->texinfo[face->texinfo];
				setbit(&cam->texturebits[texinfo->miptex * ((bsp->numfaces + 7) / 8)], f, true);
			}
		} else {
			dnode_t *node = &bsp->nodes[child];
			dplane_t *plane = &bsp->planes[node->plane];

			if(!boxinfrustum(cam->frustum, node->mins, node->maxs)) {
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
				fprintf(stderr, "gl_rmodel: stack overflow, skipping node %d\n", child);
				continue;
			}
			children[nc++] = node->children[dist > 0 ? 0 : 1];
			children[nc++] = node->children[dist > 0 ? 1 : 0];
		}
	}
}

bool cam_init(const bsp_t *bsp, cam_t *cam)
{
	cam->pvs = NULL;
	cam->texturebits = NULL;
	
	if(!getspawn(bsp, cam->origin, &cam->yaw)) {
		fprintf(stderr, "failed to find spawn point, spawning at {0, 0, 0}");
	}
	// pvs for our current position
	cam->pvs = malloc((bsp->numleaves + 7) / 8);
	if(cam->pvs == NULL) {
		goto bad_alloc;
	}
	// bitset of faces to render for each texture
	cam->texturebits = calloc((bsp->numfaces + 7) / 8, bsp->miphdr->nummiptex);
	if(cam->texturebits == NULL) {
		goto bad_alloc;
	}

	memset(cam->pvs, 0, (bsp->numleaves + 7) / 8);
	m4lookat(cam->origin, cam->pitch, cam->yaw, cam->view);
	bsp_pvsfororigin(bsp, cam->origin, cam->pvs);
	cam->updated = true;
	return true;
bad_alloc:
	fprintf(stderr, "cam_init: bad alloc\n");
	cam_free(cam);
	return false;
}

void cam_free(cam_t *cam)
{
	if(cam != NULL) {
		free(cam->texturebits);
		free(cam->pvs);
	}
}


void cam_offset(const bsp_t *bsp, cam_t *cam, float delta[3])
{
	v3add(cam->origin, delta, cam->origin);
	m4lookat(cam->origin, cam->pitch, cam->yaw, cam->view);

	setupfrustum((const float(*)[])cam->proj, (const float(*)[])cam->view, cam->frustum);
	
	bsp_pvsfororigin(bsp, cam->origin, cam->pvs);
	cam->updated = true;
}

void cam_rotate(cam_t *cam, float dy, float dp)
{
	cam->yaw += dy;
	cam->pitch += dp;

	cam->pitch = fmin(cam->pitch, M_PI_2 * 0.999f);
	cam->pitch = fmax(cam->pitch, -M_PI_2 * 0.999f);

	cam->yaw = remainderf(cam->yaw, M_PI * 2.0f);

	m4lookat(cam->origin, cam->pitch, cam->yaw, cam->view);
	setupfrustum((const float(*)[])cam->proj, (const float(*)[])cam->view, cam->frustum);
	cam->updated = true;
}
