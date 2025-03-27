#define _USE_MATH_DEFINES
#include <math.h>

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gl.h"
#include "wad.h"
#include "bsp.h"
#include "entity.h"

#define RAD2DEG(x) (x * (180.0 / M_PI))
#define DEG2RAD(x) (x * ( M_PI / 180.0))

void updatepos(float dt);

int main(int argc, char **argv)
{
	int status = EXIT_SUCCESS;

	if(argc != 2) {
		fputs("Usage: bspstudy [FILENAME]\n", stderr);
		return EXIT_FAILURE;
	}
	
	const char *filename = argv[1];
	FILE *fp = NULL;
	fp = fopen(filename, "rb");
	if(fp == NULL) {
		fprintf(stderr, "%s: failed to open file\n", filename);
		return EXIT_FAILURE;
	}

	printf("Reading %s:\n", filename);

	bsp_t *bsp = bsp_open(filename);
	if(bsp == NULL) {
		goto bad;
	}
	
	const char *wadname = bsp_wadname(bsp);

	printf("WAD File: %s\n", wadname);

	wad_t *wad = NULL;
	wad = wad_open(wadname);
	if(wad == NULL) {
		fprintf(stderr, "Failed to open wad file %s\n", wadname);

		wad = wad_open("halflife.wad");
		if(wad == NULL) {
			goto bad;
		}
	}

	if(gl_init() != 0) {
		goto bad;
	}
	
	GLuint *gltex = calloc(bsp->miphdr->nummiptex, sizeof(GLuint));
	
	for(int32_t i = 0; i < bsp->miphdr->nummiptex; i++) {

		if(bsp->miphdr->dataofs[i] == 0 || bsp->miphdr->dataofs[i] == -1) {
			continue;
		}

		miptex_t *miptex = (miptex_t *)(bsp->textures + bsp->miphdr->dataofs[i]);

		if(miptex->offsets[0] != 0) {
			/* texture is in BSP file */
			gltex[i] = gl_loadmiptex(miptex);
		} else {
			const char *name = miptex->name;
			/* texture is in WAD file */
			miptex = wad_getmiptex(wad, name);
			if(miptex != NULL) {
				gltex[i] = gl_loadmiptex(miptex);
				free(miptex);
			} else {
				fprintf(stderr, "%s: Failed to find %s\n", wadname, name);
			}
		}
	}

	wad_close(wad);

	fflush(stdout);

	float pitch = 0.0f;
	float yaw = 0.0f;
	float origin[3];

	int32_t *brushmodels = malloc(bsp->nummodels * sizeof(int32_t));
	int32_t nummodels = 0;

	bool found_spawn = false;
	for(int32_t i = 0; i < bsp->numentities; i++) {
		const char *classname = entgets(&bsp->entities[i], "classname");
		// get spawn point
		if(!strncmp(classname, "info_player_start", EPAIR_MAX_KEY)) {
			entgetv3(&bsp->entities[i], "origin", origin);
			entgetf(&bsp->entities[i], "angle", &yaw);

			yaw = DEG2RAD(yaw);
			found_spawn = true;
		}

		const char *model = entgets(&bsp->entities[i], "model");
		if(model != NULL) {
			if(!strncmp(classname, "func_", 5) && model[0] == '*') {
				brushmodels[nummodels++] = atoi(&model[1]);
			}
		}
	}

	if(!found_spawn) {
		fprintf(stderr, "failed to find spawn point, spawning at {0, 0, 0}");
	}
	
	gl_lookat(origin, pitch, yaw);

	double time = gl_time();
	double dt;
	// main loop
	for(;;) {
		dt = gl_time() - time;
		time = gl_time();
		updatepos(dt*100.0f);
		
		gl_clear(0.0f, 0.0f, 0.0f, 1.0f);
		
		gl_3dmode();
		gl_rmodel(bsp, gltex, 0);
		
		for(int32_t i = 0; i < nummodels; i++) {
			gl_rmodel(bsp, gltex, brushmodels[i]);
		}

		gl_2dmode();
		gl_printf(24.0f, 24.0f, "%.0lf Frames Per Second", 1.0f / dt, filename);
		gl_printf(24.0f, 48.0f, "BSP File: %s", filename);
		gl_printf(24.0f, 72.0f, "WAD File: %s", wadname);

		gl_swapbuffers();
		gl_pollevents();

		if(gl_shouldclose()) {
			goto end;
		}
	}
bad:
	status = EXIT_FAILURE;
	/* fallthrough */
end:
	bsp_free(bsp);
	return status;
}
