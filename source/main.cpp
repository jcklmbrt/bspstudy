#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "render.hpp"
#include "bsp.hpp"
#include "glm/geometric.hpp"
#include "hud.hpp"
#include "entity.hpp"
#include "camera.hpp"
#include "lightmap.hpp"

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
	cam.onresize(w, h);
	// setup 2d ortho projection
	hud.onresize(w, h);
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

	cam.rotate(xoffset, yoffset);
}

int main(int argc, char **argv)
{
	int status = EXIT_SUCCESS;

	if(argc != 2) {
		fputs("usage: bspstudy [FILENAME]\n", stderr);
		return EXIT_FAILURE;
	}
	
	const char *filename = argv[1];
	
	printf("reading %s:\n", filename);

	bsp_t bsp;
	render_t r;
	lightmap_t lightmap;
	
	if(!bsp.open(filename)) {
		return EXIT_FAILURE;
	}

	fflush(stdout);

	if(glfwInit() != GLFW_TRUE) {
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	s_window = glfwCreateWindow(s_width, s_height, argv[0], NULL, NULL);
	if(s_window == NULL) {
		return EXIT_FAILURE;
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
		return EXIT_FAILURE;
	}

	if(!lightmap.init(bsp)) {
		return EXIT_FAILURE;
	}

	if(!cam.init(bsp)) {
		return EXIT_FAILURE;
	}
	
	if(!hud.init()) {
		return EXIT_FAILURE;
	}

	if(!r.init(bsp, lightmap)) {
		return EXIT_FAILURE;
	}

	int32_t *brushmodels = new int32_t[bsp.nummodels];
	int32_t nummodels = 0;

	glm::vec3 *origins = new glm::vec3[bsp.numentities];
	for(int32_t i = 0; i < bsp.numentities; i++) {
		bsp.entities[i].get("origin", origins[i]);
		const char *model = bsp.entities[i].get("model");
		if(model != NULL) {
			const char *classname = bsp.entities[i].get("classname");
			if(!strncmp(classname, "func_", 5) && model[0] == '*') {
				int32_t index = atoi(&model[1]);
				if(index > 0 && index < bsp.nummodels) {
					brushmodels[nummodels++] = index;
					dmodel_t *mdl = &bsp.models[index];
					for(int j = 0; j < 3; j++) {
						mdl->origin[j] = origins[i][j];
					}
				}
			}
		}
	}
	
	framebuffer_size_callback(s_window, s_width, s_height);
	
	double time = glfwGetTime();
	double dt;

	size_t len;
	char buf[2048];

	// main loop
	while(!glfwWindowShouldClose(s_window)) {
		
		dt = glfwGetTime() - time;
		time = glfwGetTime();

		float cp = cos(cam.m_pitch);
		float sp = sin(cam.m_pitch);
		float cy = cos(cam.m_yaw);
		float sy = sin(cam.m_yaw);

		glm::vec3 forward = { cp * cy, cp * sy, -sp };
		glm::vec3 side = { sy, -cy, 0.0f };

		const float velocity = 1000.0f;

		glm::vec3 delta = { 0.0f, 0.0f, 0.0f };
		if(s_keyflags & INPUT_UP) delta += forward;
		if(s_keyflags & INPUT_DOWN) delta -= forward;
		if(s_keyflags & INPUT_LEFT) delta -= side;
		if(s_keyflags & INPUT_RIGHT) delta += side;

		if(s_keyflags != 0) {
			delta = normalize(delta);
			delta *= dt * velocity;
			cam.offset(bsp, delta);
		}
		
		if(cam.m_updated) {
			cam.buildbitsetformodel(bsp, 0);
		}
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int32_t nv = r.drawworld(bsp, cam);

		for(int i = 0; i < nummodels; i++) {
			r.drawmodel(bsp, cam, brushmodels[i]);
		}

		hud.clear();

		len = snprintf(buf, sizeof(buf),
			 "%.01f Frames Per Second\n"
			 "BSP File: %s\n"
			       "Faces drawn: %d",
			       1.0f / dt, filename, nv);

		hud.puts(24.0f, 24.0f, buf, len);

		for(int32_t i = 0; i < bsp.numentities; i++) {
			if(!bsp.entities[i].get("origin")) {
				continue;
			}

			int32_t leaf = bsp.pointinleaf(origins[i]);
			if(!isbitset(cam.m_pvs, leaf - 1)) {
				continue;
			}
				
			const char *classname = bsp.entities[i].get("classname");

			float x, y;
			len = strlen(classname);

			float w, h;
			hud.strsize(w, h, classname, len);

			w /= 2.0f;
			h /= 2.0f;

			if(cam.world2screen(origins[i], s_width, s_height, x, y)) {
				hud.puts(x - w, y - h, classname, len);
			}
		}
		
		hud.drawelems();
		
		glfwSwapBuffers(s_window);
		glfwPollEvents();
	}

	delete[] origins;
	delete[] brushmodels;
	if(s_window != NULL) {
		glfwDestroyWindow(s_window);
	}
	glfwTerminate();
	return status;
}
