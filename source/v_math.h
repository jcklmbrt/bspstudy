#ifndef _V_MATH_H
#define _V_MATH_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>


/* we need to use keyword soup
   to disable warnings */
#define M_INL static inline


M_INL float rad2deg(float x)
{
	return x * (180.0f / M_PI);
}


M_INL float deg2rad(float x)
{
	return x * (M_PI / 180.0f);
}


M_INL void v3add(const float lhs[3], const float rhs[3], float out[3])
{
	out[0] = lhs[0] + rhs[0];
	out[1] = lhs[1] + rhs[1];
	out[2] = lhs[2] + rhs[2];
}


M_INL void v3sub(const float lhs[3], const float rhs[3], float out[3])
{
	out[0] = lhs[0] - rhs[0];
	out[1] = lhs[1] - rhs[1];
	out[2] = lhs[2] - rhs[2];
}


M_INL void v3scale(const float v[3], float f, float out[3])
{
	out[0] = v[0] * f;
	out[1] = v[1] * f;
	out[2] = v[2] * f;
}


M_INL float v3dot(const float lhs[3], const float rhs[3])
{
	return lhs[0] * rhs[0] +
	       lhs[1] * rhs[1] +
	       lhs[2] * rhs[2];
}


M_INL void v3cross(const float a[3], const float b[3], float out[3])
{
	out[0] = a[1] * b[2] - b[1] * a[2];
	out[1] = a[2] * b[0] - b[2] * a[0];
	out[2] = a[0] * b[1] - b[0] * a[1];
}

M_INL void v3angles(float pitch, float yaw, float forward[3], float side[3], float up[3])
{
	float cp = cosf(pitch);
	float sp = sinf(pitch);
	float cy = cosf(yaw);
	float sy = sinf(yaw);

	if(forward) {
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if(side) {
		side[0] = sy;
		side[1] = -cy;
		side[2] = 0.0f;
	}
	if(up) {
		up[0] = 0.0f;
		up[1] = 0.0f;
		up[2] = 1.0f;
	}

}

M_INL float v3len(const float in[3])
{
	return sqrtf(v3dot(in, in));
}


M_INL bool v3norm(const float in[3], float out[3])
{
	float len = v3len(in);
	if(len != 0.0) {
		out[0] = in[0] / len;
		out[1] = in[1] / len;
		out[2] = in[2] / len;
		return true;
	}
	return false;
}

M_INL void m4perspective(float fovy, float aspect, float znear, float zfar, float m[4][4])
{
	float halffovy = fovy / 2.0f;
	float f = cosf(halffovy) / sinf(halffovy);
	float d = znear - zfar;

	m[0][0] = f / aspect;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	
	m[1][0] = 0.0f;
	m[1][1] = f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = (zfar + znear) / d;
	m[2][3] = -1.0f;
	
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = (2.0 * zfar * znear) / d;
	m[3][3] = 0.0f;
}

M_INL void m4lookat(const float origin[3], float pitch, float yaw, float m[4][4])
{

	float cp = cosf(pitch);
	float sp = sinf(pitch);
	float cy = cosf(yaw);
	float sy = sinf(yaw);

	float forward[3] = { cp * cy, cp * sy, -sp };
	float side[3] = { sy, -cy, 0.0f };
	float up[3] = { 0.0f, 0.0f, 1.0f };

	v3norm(forward, forward);
	v3cross(forward, up, side);
	v3norm(side, side);
	v3cross(side, forward, up);
	
	for(int i = 0; i < 3; i++) {
		m[i][0] = side[i];
		m[i][1] = up[i];
		m[i][2] = -forward[i];
		m[i][3] = 0.0f;
	}
	
	m[3][0] = -v3dot(origin, side);
	m[3][1] = -v3dot(origin, up);
	m[3][2] = v3dot(origin, forward);
	m[3][3] = 1.0f;
}


M_INL void m4translate(float pos[3], float m[4][4])
{
	m[0][0] = 1.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;

	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;

	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 1.0f;
	m[2][3] = 0.0f;

	m[3][0] = -pos[0];
	m[3][1] = -pos[1];
	m[3][2] = -pos[2];
	m[3][3] = 1.0f;
}

M_INL void m4ortho2d(float left, float right, float bottom, float top, float m[4][4])
{
	m[0][0] = 2.0f / (right - left);
	m[1][0] = 0.0f;
	m[2][0] = 0.0f;
	m[3][0] = -(right + left) / (right - left);

	m[0][1] = 0.0f;
	m[1][1] = 2.0f / (top - bottom);
	m[2][1] = 0.0f;
	m[3][1] = -(top + bottom) / (top - bottom);

	m[0][2] = 0.0f;
	m[1][2] = 0.0f;
	m[2][2] = -1.0f;
	m[3][2] = 0.0f;

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;
}

M_INL void m4mult(const float a[4][4], const float b[4][4], float out[4][4])
{
	float tmp[4][4];
	
	for(int i = 0; i < 4; i++) {
		tmp[0][i] = a[0][i] * b[0][0] + a[1][i] * b[0][1] + a[2][i] * b[0][2] + a[3][i] * b[0][3];
		tmp[1][i] = a[0][i] * b[1][0] + a[1][i] * b[1][1] + a[2][i] * b[1][2] + a[3][i] * b[1][3];
		tmp[2][i] = a[0][i] * b[2][0] + a[1][i] * b[2][1] + a[2][i] * b[2][2] + a[3][i] * b[2][3];
		tmp[3][i] = a[0][i] * b[3][0] + a[1][i] * b[3][1] + a[2][i] * b[3][2] + a[3][i] * b[3][3];
	}

	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			out[i][j] = tmp[i][j];
		}
	}
}

#undef M_INL

#endif
