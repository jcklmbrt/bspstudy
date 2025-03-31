#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gl.h"
#include "bsp.h"
#include "hud.h"
#include "v_math.h"
#include "entity.h"
#include "lightmap.h"

#include <GLFW/glfw3.h>

void updatepos(float dt);

static int s_width = 640;
static int s_height = 480;
static GLFWwindow *s_window = NULL;

static float s_origin[3] = { 0.0f, 0.0f, 0.0f };
static float s_yaw = 0.0f;
static float s_pitch = 0.0f;

enum keyflags {
	INPUT_LEFT = (1 << 0),
	INPUT_RIGHT = (1 << 1),
	INPUT_UP = (1 << 2),
	INPUT_DOWN = (1 << 3)
};

static unsigned s_keyflags = 0;

static hud_t hud;

extern void updatesize(float w, float h);
static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	(void)window;
	
	glViewport(0, 0, width, height);
	
	s_width = width;
	s_height = height;

	float w = (float)width;
	float h = (float)height;
	// setup 3d perspective projection
	gl_onresize(w, h);
	// setup 2d ortho projection
	hud_onresize(&hud, w, h);
}


void updatepos(float dt)
{
	float forward[3];
	float side[3];

	float cp = cosf(s_pitch);
	float sp = sinf(s_pitch);
	float cy = cosf(s_yaw);
	float sy = sinf(s_yaw);
	
	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	side[0] = sy;
	side[1] = -cy;
	side[2] = 0.0f;

	const float velocity = 1000.0f;
	dt *= velocity;

	float delta[3] = { 0.0f, 0.0f, 0.0f };
	if(s_keyflags & INPUT_UP) v3add(delta, forward, delta);
	if(s_keyflags & INPUT_DOWN) v3sub(delta, forward, delta);
	if(s_keyflags & INPUT_LEFT) v3sub(delta, side, delta);
	if(s_keyflags & INPUT_RIGHT) v3add(delta, side, delta);

	if(v3norm(delta, delta)) {
		v3scale(delta, dt, delta);
		v3add(s_origin, delta, s_origin);
	}

	gl_lookat(s_origin, s_pitch, s_yaw);
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

	s_yaw += xoffset;
	s_pitch += yoffset;

	s_pitch = fmin(s_pitch,  M_PI_2 * 0.99f);
	s_pitch = fmax(s_pitch, -M_PI_2 * 0.99f);
	
	s_yaw = remainderf(s_yaw, M_PI * 2.0);

	gl_lookat(s_origin, s_pitch, s_yaw);
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

	s_window = glfwCreateWindow(s_width, s_height, "BSP Study", NULL, NULL);
	if(s_window == NULL) {
		goto bad;
	}

	glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	if(glfwRawMouseMotionSupported()) {
		glfwSetInputMode(s_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	glfwSwapInterval(0);

	glfwMakeContextCurrent(s_window);
	glfwSetFramebufferSizeCallback(s_window, framebuffer_size_callback);
	glfwSetKeyCallback(s_window, key_callback);
	glfwSetCursorPosCallback(s_window, mouse_callback);

	int version = gladLoaderLoadGL();
	if(version == 0) {
		goto bad;
	}

	if(gl_init() != EXIT_SUCCESS) {
		goto bad;
	}

	GLuint *textures = gl_loadtextures(bsp);
	if(textures == NULL) {
		printf("Failed to load textures\n");
		goto bad;
	}

	lightmap_t lightmap;
	lm_init(bsp, &lightmap);
;
	int32_t *brushmodels = malloc(bsp->nummodels * sizeof(int32_t));
	int32_t nummodels = 0;

	float *origins = malloc(sizeof(float) * bsp->numentities * 3);

	bool found_spawn = false;
	for(int32_t i = 0; i < bsp->numentities; i++) {
		const char *classname = entgets(&bsp->entities[i], "classname");
		// get spawn point
		if(!strncmp(classname, "info_player_start", EPAIR_MAX_KEY)) {
			entgetv3(&bsp->entities[i], "origin", s_origin);
			entgetf(&bsp->entities[i], "angle", &s_yaw);

			s_yaw = deg2rad(s_yaw);
			found_spawn = true;
		}

		entgetv3(&bsp->entities[i], "origin", origins + i * 3);

		const char *model = entgets(&bsp->entities[i], "model");
		if(model != NULL) {
			if(!strncmp(classname, "func_", 5) && model[0] == '*') {
				brushmodels[nummodels++] = atoi(&model[1]);
			}
		}
	}

	hud_init(&hud);

	framebuffer_size_callback(s_window, s_width, s_height);

	if(!found_spawn) {
		fprintf(stderr, "failed to find spawn point, spawning at {0, 0, 0}");
	}
	
	gl_lookat(s_origin, s_pitch, s_yaw);

	gl_setupvertices(bsp, &lightmap);
	
	double time = glfwGetTime();
	double dt;

	size_t len;
	char buf[2048];
	// main loop
	for(;;) {
		dt = glfwGetTime() - time;
		time = glfwGetTime();
		updatepos(dt);
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		gl_3dmode();
		
		gl_rmodel(bsp, s_origin, 0);

		for(int32_t i = 0; i < nummodels; i++) {
			gl_rmodel(bsp, s_origin, brushmodels[i]);
		}

		gl_end(bsp, textures);

		hud_clear(&hud);

		len = snprintf(buf, sizeof(buf),
			 "%.01f Frames Per Second\n"
			 "BSP File: %s",
			 1.0f / dt, filename);

		hud_puts(&hud, 24.0f, 24.0f, buf, len);

		for(int32_t i = 0; i < bsp->numentities; i++) {
			if(entgets(&bsp->entities[i], "origin")) {
				
				const char *classname = entgets(&bsp->entities[i], "classname");

				float x, y;
				len = strlen(classname);

				float w, h;
				hud_strsize(&hud, &w, &h, classname, len);

				w /= 2.0f;
				h /= 2.0f;
				
				if(gl_world2screen(&origins[i*3], s_width, s_height, &x, &y)) {
					hud_puts(&hud, x - w, y - h, classname, len);
				}
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
	lm_free(&lightmap);
	bsp_free(bsp);

	if(s_window != NULL) {
		glfwDestroyWindow(s_window);
	}

	glfwTerminate();
	
	return status;
}
