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


void cam_onresize(cam_t *cam, float w, float h)
{
	m4perspective(90.0f, w / h, 0.1f, 10000.0f, cam->proj);
}


void cam_buildbitsetformodel(const bsp_t *bsp, const cam_t *cam, int32_t index)
{	
	dmodel_t *mdl = &bsp->models[index];
	
	memset(cam->texturebits, 0, bsp->miphdr->nummiptex * ((bsp->numfaces + 7) / 8));
	
	int16_t children[128];
	// hull 0 is used for rendering, the others are for collision
	children[0] = mdl->headnode[0];
	int nc = 1;

	while(nc > 0) {
		int16_t child = children[--nc];
		if(child < 0) {
			dleaf_t *leaf = &bsp->leaves[~child];
			int16_t b = (~child) - 1;
			if(isbitset(cam->pvs, b) == false) {
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
	// pvs set for our current position
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
}
