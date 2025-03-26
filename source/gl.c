#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "bsp.h"
#include "wad.h"
#include "gl.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static GLFWwindow *s_window;
static int s_width = 640;
static int s_height = 480;

#define FONTATLAS_WIDTH 512
#define FONTATLAS_HEIGHT 512
static GLuint s_glfont = 0;
static stbtt_packedchar s_pc[128];

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	s_width = width;
	s_height = height;
	
	gluPerspective(90.0, (GLdouble)width / (GLdouble)height, 0.1, 10000.0);
}

static float s_origin[3];
static float s_yaw;
static float s_pitch;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	float forward[3];
	float side[3];

	float cp = cosf(s_pitch);
	float sp = sinf(s_pitch);
	float cy = cosf(s_yaw);
	float sy = sinf(s_yaw);
	float sr = sinf(0.0f);
	float cr = cosf(0.0f);
	
	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	side[0] = sy;
	side[1] = -cy;
	side[2] = 0.0f;

	for(int i = 0; i < 3; i++) {
		forward[i] *= 10.0f;
		side[i] *= 10.0f;
		
		switch(key) {
		case GLFW_KEY_UP: case GLFW_KEY_W: s_origin[i] += forward[i]; break;
		case GLFW_KEY_DOWN: case GLFW_KEY_S: s_origin[i] -= forward[i]; break;
		case GLFW_KEY_LEFT: case GLFW_KEY_A: s_origin[i] -= side[i]; break;
		case GLFW_KEY_RIGHT: case GLFW_KEY_D: s_origin[i] += side[i]; break;
		}
	}
}

static void mouse_callback(GLFWwindow *window, double x, double y)
{
	static float last_x = 0.0f;
	static float last_y = 0.0f;
	
	float xoffset = last_x - x;
	float yoffset = y - last_y;
	last_x = x;
	last_y = y;

	const float sensitivity = 0.01f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	s_yaw += xoffset;
	s_pitch += yoffset;

	const float min_pitch = -M_PI_2 * 0.99f;
	const float max_pitch =  M_PI_2 * 0.99f;
	
	s_pitch = s_pitch > max_pitch ? max_pitch : s_pitch;
	s_pitch = s_pitch < min_pitch ? min_pitch : s_pitch; 

	s_yaw = remainderf(s_yaw, M_PI * 2.0);
}

void gl_swapbuffers(void)
{
	glfwSwapBuffers(s_window);
}

void gl_pollevents(void)
{
	glfwPollEvents();
}

int gl_shouldclose(void)
{
	return glfwWindowShouldClose(s_window);
}


GLuint gl_loadmiptex(struct miptex *miptex)
{
	uint8_t *miptex_p = (uint8_t *)miptex;
	int32_t width = miptex->width;
	int32_t height = miptex->height;
	
	uint8_t *palette = miptex_p + miptex->offsets[3] + (width / 8) * (height / 8) + 2;
	uint8_t *mip0 = miptex_p + miptex->offsets[0];

	uint8_t *rgba = malloc(width * height * 4);
	for(int i = 0; i < height * width; i++) {
		int32_t p = mip0[i] * 3;
		rgba[i * 4 + 0] = palette[p + 0];
		rgba[i * 4 + 1] = palette[p + 1];
		rgba[i * 4 + 2] = palette[p + 2];
		// https://developer.valvesoftware.com/wiki/Texture_prefixes
		if(mip0[i] == 255 && miptex->name[0] == '{') {
			rgba[i * 4 + 3] = 0x00;
		} else {
			rgba[i * 4 + 3] = 0xFF;
		}
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glBindTexture(GL_TEXTURE_2D, 0);
	free(rgba);

	return tex;
}


void gl_rmodel(struct bsp *bsp, GLuint *gltex, int32_t index)
{
	if(index < 0 || index > bsp->nummodels) {
		fprintf(stderr, "gl_rmodel: model index %d out of bounds (0-%d)",
			index, bsp->nummodels);
		return;
	}
	
	struct bsp_model *mdl = &bsp->models[index];
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(mdl->origin[0], mdl->origin[1], mdl->origin[2]);
	glMatrixMode(GL_PROJECTION);

	int16_t children[128];
	
	// hull 0 is used for rendering, the others are for collision
	children[0] = mdl->headnode[0];
	int nc = 1;

	while(nc > 0) {
		int16_t child = children[--nc];

		if(child < 0) {
			struct bsp_leaf *leaf = &bsp->leaves[~child];
			
			for(int32_t j = 0; j < leaf->nummarksurfaces; j++) {
				int32_t face = bsp->marksurfaces[leaf->firstmarksurface + j];
				gl_rface(bsp, gltex, &bsp->faces[face]);
			}
		} else {
			struct bsp_node *node = &bsp->nodes[child];
			struct bsp_plane *plane = &bsp->planes[node->plane];

			float dist;
			switch(plane->type) {
			case PLANE_X: dist = s_origin[0] - plane->dist; break;
			case PLANE_Y: dist = s_origin[1] - plane->dist; break;
			case PLANE_Z: dist = s_origin[2] - plane->dist; break;
			default: dist = plane->normal[0] * s_origin[0] +
					plane->normal[1] * s_origin[1] +
					plane->normal[2] * s_origin[2];
				dist -= plane->dist;
				break;
			}

			if(nc > 128) {
				fprintf(stderr, "gl_rmodel: stack overflow, skipping node %d\n", child);
			}
			
			children[nc++] = node->children[dist > 0 ? 0 : 1];
			children[nc++] = node->children[dist > 0 ? 1 : 0];
		}
	}
	glPopMatrix();
}

void gl_rface(struct bsp *bsp, GLuint *gltex, struct bsp_face *face)
{
	struct bsp_texinfo *texinfo = &bsp->texinfo[face->texinfo];
	glBindTexture(GL_TEXTURE_2D, gltex[texinfo->miptex]);

	struct miptex *miptex = bsp->textures + bsp->miphdr->dataofs[texinfo->miptex];
	
	glBegin(GL_POLYGON);
	
	for(int32_t i = 0; i < face->numedges; i++) {
		
		int32_t edge = bsp->surfedges[face->firstedge + i];

		int32_t v;
		if(edge >= 0) {
			v = bsp->edges[edge].v0;
		} else {
			v = bsp->edges[-edge].v1;
		}

		struct bsp_vertex vtx = bsp->vertices[v];
		
		float s = vtx.x * texinfo->vecs[0][0] +
			vtx.y * texinfo->vecs[0][1] +
			vtx.z * texinfo->vecs[0][2] + texinfo->vecs[0][3];

		float t = vtx.x * texinfo->vecs[1][0] +
			vtx.y * texinfo->vecs[1][1] +
			vtx.z * texinfo->vecs[1][2] + texinfo->vecs[1][3];

		s /= (float)miptex->width;
		t /= (float)miptex->height;
		
		glTexCoord2f(s, t);
		glVertex3d(vtx.x, vtx.y, vtx.z);
	}
	glEnd();
}

void gl_setupview(void)
{
	float center[3];
	center[0] = s_origin[0] + cosf(s_pitch) * cosf(s_yaw);
	center[1] = s_origin[1] + cosf(s_pitch) * sinf(s_yaw);
	center[2] = s_origin[2] - sinf(s_pitch);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/* z is up */
	gluLookAt(s_origin[0], s_origin[1], s_origin[2], 
		  center[0], center[1], center[2],
		  0.0, 0.0, 1.0);

	glMatrixMode(GL_PROJECTION);
}

void gl_lookat(const float origin[3], float pitch, float yaw)
{
	s_origin[0] = origin[0];
	s_origin[1] = origin[1];
	s_origin[2] = origin[2];

	s_pitch = pitch;
	s_yaw = yaw;
	
}

int gl_printf(float x, float y, const char *fmt, ...)
{
	char buf[2048];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, 2048, fmt, args);
	va_end(args);

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, s_width, s_height, 0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
		
	glBindTexture(GL_TEXTURE_2D, s_glfont);

	glBegin(GL_QUADS);
	for(int i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(s_pc, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, buf[i], &x, &y, &q, 0);
		glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
		glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
		glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
		glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
	}
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);

	return len;
}


double gl_time(void)
{
	return glfwGetTime();
}

void gl_clear(float r, float g, float b, float a)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int gl_init(void)
{
	const char *msg = NULL;
	if(glfwInit() != GLFW_TRUE) {
		goto bad;
	}
	   
	/* Yes, I'm using an antiquated version of OpenGL.
	   I don't want to bother with extension loaders,
	   this is a study of the BSP file format, not modern OpenGL. */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	s_window = glfwCreateWindow(s_width, s_height, "BSP Study", NULL, NULL);
	if(s_window == NULL) {
		goto bad;
	}

	glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	if(glfwRawMouseMotionSupported()) {
		glfwSetInputMode(s_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	glfwMakeContextCurrent(s_window);
	glfwSetFramebufferSizeCallback(s_window, framebuffer_size_callback);
	glfwSetKeyCallback(s_window, key_callback);
	glfwSetCursorPosCallback(s_window, mouse_callback);

	framebuffer_size_callback(s_window, s_width, s_height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// TODO: implement proper back-to-front rendering
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

#ifndef _WIN32
	const char *font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#else
	const char *font = "C:\\Windows\\Fonts\\DejavuSans.ttf";
#endif
	glfwSwapInterval(0);
	
	FILE *fp = fopen(font, "rb");
	if(fp == NULL) {
		fprintf(stderr, "Failed to open font: %s\n", font);
		goto bad;
	}
	
	fseek(fp, 0, SEEK_END);
	size_t ttfsize = ftell(fp);
	uint8_t *rawttf = malloc(ttfsize);
	
	fseek(fp, 0, SEEK_SET);
	fread(rawttf, 1, ttfsize, fp);
	fclose(fp);

	stbtt_pack_context ctx;
	const float fontsize =  24.0f;
	uint8_t *alpha = malloc(FONTATLAS_WIDTH * FONTATLAS_HEIGHT);
	uint8_t *rgba = malloc(FONTATLAS_WIDTH * FONTATLAS_HEIGHT * 4);
	stbtt_PackBegin(&ctx, alpha, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, 1, NULL);
	stbtt_PackFontRange(&ctx, rawttf, 0, fontsize, 0, 128, s_pc);
	stbtt_PackEnd(&ctx);

	for(int i = 0; i < FONTATLAS_WIDTH * FONTATLAS_HEIGHT; i++) {
		rgba[i * 4 + 0] = 0xFF;
		rgba[i * 4 + 1] = 0xFF;
		rgba[i * 4 + 2] = 0xFF;
		rgba[i * 4 + 3] = alpha[i];
	}

	glGenTextures(1, &s_glfont);
	glBindTexture(GL_TEXTURE_2D, s_glfont);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONTATLAS_WIDTH, FONTATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	free(rawttf);
	free(alpha);
	free(rgba);
	
	return EXIT_SUCCESS;
bad:
	glfwGetError(&msg);
	if(msg != NULL) {
		fprintf(stderr, "%s", msg);
	}

	gl_free();
	return EXIT_FAILURE;
}

void gl_free()
{
	if(s_window != NULL) {
		glfwDestroyWindow(s_window);
	}
	glfwTerminate();
}
