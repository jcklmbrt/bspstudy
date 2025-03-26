#ifndef _GL_H
#define _GL_H


#include <GL/gl.h>

struct bsp;
struct bsp_face;
struct miptex;

int gl_init(void);
void gl_free(void);
double gl_time(void);

void gl_setupview(void);
void gl_lookat(const float origin[3], float pitch, float yaw);
void gl_rface(struct bsp *bsp, GLuint *gltex, struct bsp_face *face);
void gl_rmodel(struct bsp *bsp, GLuint *gltex, int32_t index);
int gl_printf(float x, float y, const char *fmt, ...);

void gl_clear(float r, float g, float b, float a);
void gl_swapbuffers(void);
void gl_pollevents(void);
int gl_shouldclose(void);

GLuint gl_loadmiptex(struct miptex *miptex);

#endif
