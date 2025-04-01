#ifndef _CAMERA_H
#define _CAMERA_H

#include <stdbool.h>
#include "bsp.h"

#define CAMERA_FOV M_PI_2

typedef struct {
	float origin[3];
	float pitch;
	float yaw;

	bool updated;
	float proj[4][4];
	float view[4][4];
	// a plane for each face of the faces of the frustum
	float frustum[6][4];
	
	uint8_t *pvs;
	// setup a pvs for each texture
	uint8_t *texturebits;
} cam_t;

bool boxinfrustum(float frustum[6][4], const int16_t mins[3], const int16_t maxs[3]);

bool cam_init(const bsp_t *bsp, cam_t *cam);
void cam_free(cam_t *cam);
void cam_rotate(cam_t *cam, float dp, float dy);
void cam_offset(const bsp_t *bsp, cam_t *cam, float delta[3]);
void cam_onresize(cam_t *cam, float w, float h);
bool cam_world2screen(const cam_t *cam, const float world[3], float w, float h, float *x, float *y);
void cam_buildbitsetformodel(const bsp_t *bsp, cam_t *cam, int32_t index);


static inline bool isbitset(const uint8_t *b, int i)
{
	return b[i >> 3] & (1 << (i & 7));
}

static inline void setbit(uint8_t *b, int i, bool value)
{
	uint8_t f = 1 << (i & 7);
	if(value) {
		b[i >> 3] |= f;
	} else {
		b[i >> 3] &= ~f;
	}
}

#endif
