#ifndef _CAMERA_H
#define _CAMERA_H

#include <stdbool.h>
#include "bsp.h"

#define CAMERA_FOV 90

typedef struct {
	float origin[3];
	float pitch;
	float yaw;

	float proj[4][4];
	float view[4][4];

	dplane_t frustum[6];
	
	uint8_t *pvs;
	// setup a pvs for each texture
	uint8_t *texturebits;
} cam_t;


bool cam_init(const bsp_t *bsp, cam_t *cam);
void cam_free(cam_t *cam);
void cam_rotate(cam_t *cam, float dp, float dy);
void cam_offset(const bsp_t *bsp, cam_t *cam, float delta[3]);
void cam_onresize(cam_t *cam, float w, float h);


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
