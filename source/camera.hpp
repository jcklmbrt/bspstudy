#ifndef _CAMERA_H
#define _CAMERA_H

#include <glm/glm.hpp>
#include <stdbool.h>
#include "bsp.hpp"

struct cam_t 
{
public:
	bool init(const bsp_t &bsp);
	~cam_t();
	void rotate(float dp, float dy);
	void offset(const bsp_t &bsp, const glm::vec3 &delta);
	void onresize(float w, float h);
	bool world2screen(const glm::vec3 &world, float w, float h, float &x, float &y) const;
	void buildbitsetformodel(const bsp_t &bsp, int32_t index);
	bool boxinfrustum(const int16_t mins[3], const int16_t maxs[3]) const;
	bool boxinfrustum(const float mins[3], const float maxs[3]) const;
//private:
	glm::vec3 m_origin;
	float m_pitch;
	float m_yaw;

	bool m_updated;
	glm::mat4 m_proj;
	glm::mat4 m_view;
	// a plane for each face of the faces of the frustum
	float m_frustum[6][4];
	
	uint8_t *m_pvs;
	// setup a pvs for each texture
	uint64_t *m_texturebits;
};

constexpr bool isbitset(const uint8_t *b, int i)
{
	return b[i >> 3] & (1 << (i & 7));
}

#endif
