#include <iostream>

#include <string.h>
#include <stdbool.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include "bsp.hpp"
#include "camera.hpp"

static void setupfrustum(const glm::mat4 &proj, const glm::mat4 &view, float frustum[6][4])
{
	glm::mat4 clip = proj * view;

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
		float len = std::sqrt(frustum[i][0] * frustum[i][0] +
				      frustum[i][1] * frustum[i][1] +
				      frustum[i][2] * frustum[i][2]);
		
		for(int32_t j = 0; j < 4; j++) {
			frustum[i][j] /= len;
		}
	}
}


static glm::mat4 lookat(const glm::vec3 &origin, float pitch, float yaw)
{
	float cp = cos(pitch);
	float sp = sin(pitch);
	float cy = cos(yaw);
	float sy = sin(yaw);

	glm::vec3 forward = { cp * cy, cp * sy, -sp };
	glm::vec3 up = { 0.0f, 0.0f, 1.0f };

	return glm::lookAt(origin, origin + forward, up);
}


void cam_t::onresize(float w, float h)
{
	m_proj = glm::perspective(90.0f, w / h, 0.1f, 10000.0f);
	setupfrustum(m_proj, m_view, m_frustum);
	m_updated = true;
}


bool cam_t::boxinfrustum(const int16_t mins[3], const int16_t maxs[3]) const
{
	for(int32_t p = 0; p < 6; p++) {
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		return false;
	};
	
	return true;
}


bool cam_t::boxinfrustum(const float mins[3], const float maxs[3]) const
{
	for(int32_t p = 0; p < 6; p++) {
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * maxs[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * maxs[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * maxs[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		if(m_frustum[p][0] * mins[0] + m_frustum[p][1] * mins[1] + m_frustum[p][2] * mins[2] + m_frustum[p][3] > 0) continue;
		return false;
	};
	
	return true;
}


bool cam_t::world2screen(const glm::vec3 &world, float w, float h, float &x, float &y) const
{
	glm::mat4 mvp = m_proj * m_view;
	glm::vec4 screen = mvp * glm::vec4(world, 1.0f);

	if(screen.w <= 0.001f) {
		return false;
	}
	
	screen /= screen.w;
	
	x = (screen[0] + 1.0f) * (0.5f * w);
	y = (screen[1] + 1.0f) * (0.5f * h);
	y = h - y;

	return true;
}


void cam_t::buildbitsetformodel(const bsp_t &bsp, int32_t index)
{	
	dmodel_t *mdl = &bsp.models[index];
	
	for(int32_t i = 0; i < bsp.miphdr->nummiptex; i++) {
		for(int32_t j = 0; j < ((bsp.numfaces + 63) / 64); j++) {
			m_texturebits[i * ((bsp.numfaces + 63) / 64) + j] = 0ull;
		}
	}

	int32_t nc = 0;
	int16_t children[128];
	// hull 0 is used for rendering, the others are for collision
	children[nc++] = mdl->headnode[0];

	while(nc > 0) {
		int16_t child = children[--nc];
		if(child < 0) {
			dleaf_t *leaf = &bsp.leaves[~child];
			if(!isbitset(m_pvs, (~child) - 1)) {
				continue;
			}
			if(!boxinfrustum(leaf->mins, leaf->maxs)) {
				continue;
			}
			for(int32_t j = 0; j < leaf->nummarksurfaces; j++) {
				int32_t f = bsp.marksurfaces[leaf->firstmarksurface + j];
				dface_t *face = &bsp.faces[f];
				texinfo_t *texinfo = &bsp.texinfo[face->texinfo];

				uint64_t *bits = &m_texturebits[texinfo->miptex * ((bsp.numfaces + 63) / 64)];
				bits[f / 64] |= (1ull << ((uint64_t)f % 64));
			}
		} else {
			dnode_t *node = &bsp.nodes[child];
			dplane_t *plane = &bsp.planes[node->plane];

			if(!boxinfrustum(node->mins, node->maxs)) {
				continue;
			}
			
			float dist = 0;
			switch(plane->type) {
			case PLANE_X: dist = m_origin[0] - plane->dist; break;
			case PLANE_Y: dist = m_origin[1] - plane->dist; break;
			case PLANE_Z: dist = m_origin[2] - plane->dist; break;
			default:
				dist = dot(plane->normal, m_origin) - plane->dist;
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

bool cam_t::init(const bsp_t &bsp)
{
	m_pvs = NULL;
	m_texturebits = NULL;
	
	if(!bsp.getspawn(m_origin, m_yaw)) {
		std::cerr << "failed to find spawn point, spawning at { 0, 0, 0 }" << std::endl;
	}
	// pvs for our current position
	m_pvs = new uint8_t[(bsp.numleaves + 7) / 8];
	if(m_pvs == NULL) {
		return false;
	}
	// bitset of faces to render for each texture
	m_texturebits = new uint64_t[((bsp.numfaces + 63) / 64) * bsp.miphdr->nummiptex];
	if(m_texturebits == NULL) {
		return false;
	}

	memset(m_pvs, 0, (bsp.numleaves + 7) / 8);
	m_view = lookat(m_origin, m_pitch, m_yaw);
	bsp.pvsfororigin(m_origin, m_pvs);
	m_updated = true;
	return true;
}

cam_t::~cam_t()
{
	delete[] m_texturebits;
	delete[] m_pvs;
}


void cam_t::offset(const bsp_t &bsp, const glm::vec3 &delta)
{
	m_origin += delta;

	m_view = lookat(m_origin, m_pitch, m_yaw);

	setupfrustum(m_proj, m_view, m_frustum);
	
	bsp.pvsfororigin(m_origin, m_pvs);
	m_updated = true;
}

void cam_t::rotate(float dy, float dp)
{
	m_yaw += dy;
	m_pitch += dp;

	m_pitch = fmin(m_pitch, M_PI_2 * 0.999f);
	m_pitch = fmax(m_pitch, -M_PI_2 * 0.999f);

	m_yaw = remainderf(m_yaw, M_PI * 2.0f);

	m_view = lookat(m_origin, m_pitch, m_yaw);

	setupfrustum(m_proj, m_view, m_frustum);
	m_updated = true;
}
