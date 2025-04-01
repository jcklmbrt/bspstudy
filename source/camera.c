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


static void setupfrustum(const float *proj, const float *view, float frustum[6][4])
{
	float clip[16];
	  
	clip[ 0] = view[ 0] * proj[ 0] + view[ 1] * proj[ 4] + view[ 2] * proj[ 8] + view[ 3] * proj[12];
	clip[ 1] = view[ 0] * proj[ 1] + view[ 1] * proj[ 5] + view[ 2] * proj[ 9] + view[ 3] * proj[13];
	clip[ 2] = view[ 0] * proj[ 2] + view[ 1] * proj[ 6] + view[ 2] * proj[10] + view[ 3] * proj[14];
	clip[ 3] = view[ 0] * proj[ 3] + view[ 1] * proj[ 7] + view[ 2] * proj[11] + view[ 3] * proj[15];
	clip[ 4] = view[ 4] * proj[ 0] + view[ 5] * proj[ 4] + view[ 6] * proj[ 8] + view[ 7] * proj[12];
	clip[ 5] = view[ 4] * proj[ 1] + view[ 5] * proj[ 5] + view[ 6] * proj[ 9] + view[ 7] * proj[13];
	clip[ 6] = view[ 4] * proj[ 2] + view[ 5] * proj[ 6] + view[ 6] * proj[10] + view[ 7] * proj[14];
	clip[ 7] = view[ 4] * proj[ 3] + view[ 5] * proj[ 7] + view[ 6] * proj[11] + view[ 7] * proj[15];
	clip[ 8] = view[ 8] * proj[ 0] + view[ 9] * proj[ 4] + view[10] * proj[ 8] + view[11] * proj[12];
	clip[ 9] = view[ 8] * proj[ 1] + view[ 9] * proj[ 5] + view[10] * proj[ 9] + view[11] * proj[13];
	clip[10] = view[ 8] * proj[ 2] + view[ 9] * proj[ 6] + view[10] * proj[10] + view[11] * proj[14];
	clip[11] = view[ 8] * proj[ 3] + view[ 9] * proj[ 7] + view[10] * proj[11] + view[11] * proj[15];
	clip[12] = view[12] * proj[ 0] + view[13] * proj[ 4] + view[14] * proj[ 8] + view[15] * proj[12];
	clip[13] = view[12] * proj[ 1] + view[13] * proj[ 5] + view[14] * proj[ 9] + view[15] * proj[13];
	clip[14] = view[12] * proj[ 2] + view[13] * proj[ 6] + view[14] * proj[10] + view[15] * proj[14];
	clip[15] = view[12] * proj[ 3] + view[13] * proj[ 7] + view[14] * proj[11] + view[15] * proj[15];

	frustum[0][0] = clip[ 3] - clip[ 0];
	frustum[0][1] = clip[ 7] - clip[ 4];
	frustum[0][2] = clip[11] - clip[ 8];
	frustum[0][3] = clip[15] - clip[12];

	frustum[1][0] = clip[ 3] + clip[ 0];
	frustum[1][1] = clip[ 7] + clip[ 4];
	frustum[1][2] = clip[11] + clip[ 8];
	frustum[1][3] = clip[15] + clip[12];

	frustum[2][0] = clip[ 3] + clip[ 1];
	frustum[2][1] = clip[ 7] + clip[ 5];
	frustum[2][2] = clip[11] + clip[ 9];
	frustum[2][3] = clip[15] + clip[13];

	frustum[3][0] = clip[ 3] - clip[ 1];
	frustum[3][1] = clip[ 7] - clip[ 5];
	frustum[3][2] = clip[11] - clip[ 9];
	frustum[3][3] = clip[15] - clip[13];

	frustum[4][0] = clip[ 3] - clip[ 2];
	frustum[4][1] = clip[ 7] - clip[ 6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];

	frustum[5][0] = clip[ 3] + clip[ 2];
	frustum[5][1] = clip[ 7] + clip[ 6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];

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
	//setupfrustum(cam);
}

static bool cam_infrustum(const cam_t *cam, const float mins[3], const float maxs[3])
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
		if(v3dot(cam->frustum[p], pts[0]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[1]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[2]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[3]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[4]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[5]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[6]) + cam->frustum[p][3] > 0) continue;
		if(v3dot(cam->frustum[p], pts[7]) + cam->frustum[p][3] > 0) continue;
		return false;
	}
	
	return true;
}

void cam_buildbitsetformodel(const bsp_t *bsp, const cam_t *cam, int32_t index)
{	
	dmodel_t *mdl = &bsp->models[index];
	
	memset(cam->texturebits, 0, bsp->miphdr->nummiptex * ((bsp->numfaces + 7) / 8));

	float mins[3], maxs[3];
	
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

			for(int32_t i = 0; i < 3; i++) {
				mins[i] = leaf->mins[i];
				maxs[i] = leaf->maxs[i];
			}

			if(!cam_infrustum(cam, mins, maxs)) {
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

			for(int i = 0; i < 3; i++) {
				mins[i] = node->mins[i];
				maxs[i] = node->maxs[i];
			}

			if(!cam_infrustum(cam, mins, maxs)) {
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
		goto bad;
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
	cam_buildbitsetformodel(bsp, cam, 0);
	return true;
bad_alloc:
	fprintf(stderr, "cam_init: bad alloc\n");
	/* fallthrough */
bad:
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

	setupfrustum((const float *)cam->proj, (const float *)cam->view, cam->frustum);
	
	bsp_pvsfororigin(bsp, cam->origin, cam->pvs);
	cam_buildbitsetformodel(bsp, cam, 0);
}

void cam_rotate(cam_t *cam, float dy, float dp)
{
	cam->yaw += dy;
	cam->pitch += dp;

	cam->pitch = fmin(cam->pitch, M_PI_2 * 0.999f);
	cam->pitch = fmax(cam->pitch, -M_PI_2 * 0.999f);

	cam->yaw = remainderf(cam->yaw, M_PI * 2.0f);

	m4lookat(cam->origin, cam->pitch, cam->yaw, cam->view);

	setupfrustum((const float *)cam->proj, (const float *)cam->view, cam->frustum);

//	cam_buildbitsetformodel(bsp, cam, 0);
}
