#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gl.h"
#include "bsp.h"
#include "hud.h"
#include "v_math.h"
#include "entity.h"
#include "camera.h"
#include "lightmap.h"

#include <GLFW/glfw3.h>

static int s_width = 640;
static int s_height = 480;
static GLFWwindow *s_window = NULL;

enum keyflags {
	INPUT_LEFT = (1 << 0),
	INPUT_RIGHT = (1 << 1),
	INPUT_UP = (1 << 2),
	INPUT_DOWN = (1 << 3)
};

static unsigned s_keyflags = 0;
static cam_t cam;
static hud_t hud;

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	(void)window;
	
	glViewport(0, 0, width, height);
	
	s_width = width;
	s_height = height;

	float w = (float)width;
	float h = (float)height;
	// setup 3d perspective projection
	cam_onresize(&cam, w, h);
	// setup 2d ortho projection
	hud_onresize(&hud, w, h);
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	(void)window;
	(void)scancode;
	(void)mods;
	
	unsigned flag = 0;
	
	switch(key) {
	case GLFW_KEY_UP: case GLFW_KEY_W: flag = INPUT_UP; break;
	case GLFW_KEY_DOWN: case GLFW_KEY_S: flag =  INPUT_DOWN; break;
	case GLFW_KEY_LEFT: case GLFW_KEY_A: flag = INPUT_LEFT; break;
	case GLFW_KEY_RIGHT: case GLFW_KEY_D: flag = INPUT_RIGHT; break;
	}

	if(action == GLFW_PRESS) {
		s_keyflags |= flag;
	} else if(action == GLFW_RELEASE) {
		s_keyflags &= ~flag;
	}
}

static void mouse_callback(GLFWwindow *window, double x, double y)
{
	(void)window;
	
	static float last_x = 0.0f;
	static float last_y = 0.0f;
	
	float xoffset = last_x - x;
	float yoffset = y - last_y;
	last_x = x;
	last_y = y;

	const float sensitivity = 0.001f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	cam_rotate(&cam, xoffset, yoffset);
}

int main(int argc, char **argv)
{
	int status = EXIT_SUCCESS;

	if(argc != 2) {
		fputs("Usage: bspstudy [FILENAME]\n", stderr);
		return EXIT_FAILURE;
	}
	
	const char *filename = argv[1];

	printf("Reading %s:\n", filename);

	bsp_t *bsp = bsp_open(filename);
	if(bsp == NULL) {
		goto bad;
	}

	fflush(stdout);

	if(glfwInit() != GLFW_TRUE) {
		goto bad;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	int version = gladLoaderLoadGL();

	glfwSwapInterval(0);
	
	if(version == 0) {
		goto bad;
	}

	lightmap_t lightmap;
	if(!lm_init(bsp, &lightmap)) {
		goto bad;
	}

	if(!cam_init(bsp, &cam)) {
		goto bad;
	}
	
	if(!hud_init(&hud)) {
		goto bad;
	}
	
	if(!gl_init(bsp, &lightmap)) {
		goto bad;
	}

	int32_t *brushmodels = malloc(bsp->nummodels * sizeof(int32_t));
	int32_t nummodels = 0;

	float *origins = malloc(sizeof(float) * bsp->numentities * 3);

	for(int32_t i = 0; i < bsp->numentities; i++) {
		entgetv3(&bsp->entities[i], "origin", origins + i * 3);
		const char *model = entgets(&bsp->entities[i], "model");
		if(model != NULL) {
			const char *classname = entgets(&bsp->entities[i], "classname");
			if(!strncmp(classname, "func_", 5) && model[0] == '*') {
				brushmodels[nummodels++] = atoi(&model[1]);
			}
		}
	}
	
	framebuffer_size_callback(s_window, s_width, s_height);
	
	double time = glfwGetTime();
	double dt;

	size_t len;
	char buf[2048];
	// main loop
	for(;;) {
		dt = glfwGetTime() - time;
		time = glfwGetTime();

		float forward[3];
		float side[3];

		v3angles(cam.pitch, cam.yaw, forward, side, NULL);

		const float velocity = 1000.0f;

		float delta[3] = { 0.0f, 0.0f, 0.0f };
		if(s_keyflags & INPUT_UP) v3add(delta, forward, delta);
		if(s_keyflags & INPUT_DOWN) v3sub(delta, forward, delta);
		if(s_keyflags & INPUT_LEFT) v3sub(delta, side, delta);
		if(s_keyflags & INPUT_RIGHT) v3add(delta, side, delta);

		if(v3norm(delta, delta)) {
			v3scale(delta, dt * velocity, delta);
			cam_offset(bsp, &cam, delta);
		}
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gl_renderfaces(bsp, &cam);

		hud_clear(&hud);

		len = snprintf(buf, sizeof(buf),
			 "%.01f Frames Per Second\n"
			 "BSP File: %s",
			 1.0f / dt, filename);

		hud_puts(&hud, 24.0f, 24.0f, buf, len);

		for(int32_t i = 0; i < bsp->numentities; i++) {
			if(!entgets(&bsp->entities[i], "origin")) {
				continue;
			}

			int32_t leaf = bsp_pointinleaf(bsp, &origins[i * 3]);
			if(!isbitset(cam.pvs, leaf - 1)) {
				continue;
			}
				
			const char *classname = entgets(&bsp->entities[i], "classname");

			float x, y;
			len = strlen(classname);

			float w, h;
			hud_strsize(&hud, &w, &h, classname, len);

			w /= 2.0f;
			h /= 2.0f;

			if(gl_world2screen(&cam, &origins[i*3], s_width, s_height, &x, &y)) {
				hud_puts(&hud, x - w, y - h, classname, len);
			}
		}
		
		hud_drawelems(&hud);
		
		glfwSwapBuffers(s_window);
		glfwPollEvents();

		if(glfwWindowShouldClose(s_window)) {
			goto end;
		}
	}
bad:
	status = EXIT_FAILURE;
	const char *msg = NULL;
	glfwGetError(&msg);
	if(msg != NULL) {
		fprintf(stderr, "%s", msg);
	}
	/* fallthrough */
end:
	cam_free(&cam);
	hud_free(&hud);
	gl_free(bsp);
	lm_free(&lightmap);
	bsp_free(bsp);
	if(s_window != NULL) {
		glfwDestroyWindow(s_window);
	}
	glfwTerminate();
	return status;
}
