#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
	#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <goldsrc_bspfile.h>

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT  720

#define RAD2DEG(x) (x * (180.0 / M_PI))
#define DEG2RAD(x) (x * ( M_PI / 180.0))


static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLdouble)width / (GLdouble)height, 0.1, 100.0);
	gluLookAt(1.0, 2.5, 5.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0);
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	//noop
}

struct entity {
	size_t size;
	char  *epairs; /*
	writing a "bump allocator" w/ strict aliasing.
	epairs[0] = key length                    (max size 32 < 127)
	epairs[1] << 8 | epairs[2] = value length (max size 1024 < 32767)
	epairs[3...] = data
	*/
};

static unsigned char  getkeylen(char *ep) { return ep[0]; }
static unsigned short getvallen(char *ep) { return ep[1] << 8 | ep[2]; }
static char *getkey(char *ep)  { return ep + 3; }
static char *getval(char *ep)  { return ep + 3 + getkeylen(ep); }
static char *getnext(char *ep) { return getval(ep) + getvallen(ep); }


const char *entputs(struct entity *e)
{
	const  char *classname = NULL;
	static char lines[MAX_VALUE][128];
	int i = 0;
	for(char *ep = e->epairs; ep - e->epairs < e->size; ep = getnext(ep)) {
		// assuming 1st key will be classname
		if(!strncmp(getkey(ep), "classname", getkeylen(ep))) {
			classname = getval(ep);
		} else {
			snprintf(lines[i++], MAX_VALUE, "%s: %s\n", getkey(ep), getval(ep));
		}
	}

	printf("%s:\n", classname);
	while(i--) {
		printf("\t%s", lines[i]);
	}

	return NULL;
}


const char *entgets(struct entity *e, const char *key)
{
	for(char *ep = e->epairs; ep - e->epairs < e->size; ep = getnext(ep)) {
		if(!strncmp(getkey(ep), key, getkeylen(ep))) {
			return getval(ep);
		}
	}
	return NULL;
}


const char *entgetf(struct entity *e, const char *key, float *out)
{
	const char *value = entgets(e, key);
	if(value != NULL) {
		*out = atof(value);
	}
	return value;
}


const char *entgetv3(struct entity *e, const char *key, float out[3])
{
	const char *value = entgets(e, key);
	if(value != NULL) {
		if(sscanf(value, "%f %f %f", &out[0], &out[1], &out[2]) != 3) {
			return NULL;
		};
	}
	return value;
}


enum tokid {
	ENTLEX_END   = 0,
	ENTLEX_IDENT = 1,
	ENTLEX_LBRACE = '{',
	ENTLEX_RBRACE = '}',
	ENTLEX_QUOTE  = '\"'
};


int bsp_entlex(const char **p_start, const char *end, char token[MAX_VALUE])
{
	const char *start = *p_start;
	char       *ptok  = token;

	while(start != end) {
		switch(*start) {
			/* symbols */
		case ENTLEX_LBRACE:
		case ENTLEX_RBRACE:
			*p_start = start + 1;
			return *start;
		case ENTLEX_QUOTE:
			start++;
			while(*start != '\"') {
				*ptok++ = *start++;
				if(ptok > &token[MAX_VALUE]) {
					return ENTLEX_END;
				}
			}
			*ptok = '\0';
			*p_start = start + 1;
			return ENTLEX_IDENT;
		/* skip whitespace*/
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case  ' ':
			start++;
			break;
		case '/':
			/* comments */
			start++;
			if(*start != '/') {
				break;
			}
			/* fallthrough */
		case '#':
		case ';':
			while(*start != '\n') {
				start++;
			}
			break;
			/* EOF */
		case '\0':
		case   -1:
			return ENTLEX_END;
		default:
			fprintf(stderr, "entlex: unexpected char: %c\n", *start);
			return ENTLEX_END;
		}
	}
	
	return ENTLEX_END;
}


int bsp_entparse(const char *entdata, int32_t entdatasize, 
                 struct entity *entities)
{
	/* based upon the sample bsp file's entity data,
	   let's construct a formal grammar.
	<entitylist> -> <entitylist>\n?<entity>
	<entitylist> -> <entity>
	<entity>     -> {<epairlist>}
	<epairlist>  -> <epairlist>\n?<epair>
	<epairlist>  -> <epair>
	<epair>      -> <quote> <quote>
	<quote>      -> "<value>"
	<value>      -> .*
	*/

	/* ...or you can just use a regular expression */
	/* {\s(".*" ".*"\s)+} */
	char token[MAX_VALUE];
	const char *end = entdata + entdatasize;
	size_t toklen   = 0;
	int numentities = 0;
	int epairsize   = 0;
	int old_epairsize;
	int tokid;

	char key[MAX_KEY];
	char value[MAX_VALUE];

	unsigned char  keylen = 0;
	unsigned short vallen = 0;

	for(;;) {
		tokid = bsp_entlex(&entdata, end, token);

		if(tokid == ENTLEX_END) {
			break;
		} else if(tokid != ENTLEX_LBRACE) {
			fprintf(stderr, "entparse: expected \'{\'\n");
			break;
		}

		for(;;) {
			tokid = bsp_entlex(&entdata, end, token);

			if(tokid == ENTLEX_RBRACE) {
				/* ensure entity has mandatory key "classname" */
				if(entgets(&entities[numentities], "classname") == NULL) {
					fprintf(stderr, "entparse: bad entity, no classname\n");
				} else {
					printf("%s\n", entgets(&entities[numentities], "classname"));
				}
				if(numentities++ > MAX_MAP_ENTITIES) {
					fprintf(stderr, "entparse: too many entities\n");
					return numentities;
				}
				entities[numentities].epairs = NULL;
				epairsize = 0;
				break;
			} else if(tokid != ENTLEX_IDENT) {
				fprintf(stderr, "entparse: %d expected identifier or }\n", tokid);
				return numentities;
			}

			keylen = snprintf(key, MAX_KEY, "%s", token) + 1;

			if(bsp_entlex(&entdata, end, token) != ENTLEX_IDENT) {
				fprintf(stderr, "entparse: expected identifier\n");
				return numentities;
			}

			vallen = snprintf(value, MAX_VALUE, "%s", token) + 1;

			char *epairs = entities[numentities].epairs;

			old_epairsize = epairsize;
			epairsize += keylen + vallen + 3;

			if(epairs == NULL) {
				epairs = malloc(epairsize);
			} else {
				epairs = realloc(epairs, epairsize);
			}

			if(epairs == NULL) {
				fprintf(stderr, "entparse: bad realloc: epairsize: %d\n", epairsize);
				return numentities;
			}

			char *newepair = epairs + old_epairsize;

			newepair[0] = keylen;
			newepair[1] = (vallen >> 8) & 0xFF;
			newepair[2] = (vallen >> 0) & 0xFF;

			memcpy(newepair + 3,          key,   keylen);
			memcpy(newepair + 3 + keylen, value, vallen);

			entities[numentities].epairs = epairs;
			entities[numentities].size   = epairsize;
		}
	}

	return numentities;
}


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

	struct bsp_header hdr;
	if(fread(&hdr, 1, sizeof(struct bsp_header), fp) != sizeof(struct bsp_header)) {
		fprintf(stderr, "%s: failed to read header\n", filename);
		goto bad;
	};

	/* local copy of lumps */
	void   *lumps[HEADER_LUMPS];
	int32_t lumpsize[HEADER_LUMPS];

	/* copy lumps from file to our local copy */
	for(int i = 0; i < HEADER_LUMPS; i++) {
		lumps[i] = bsp_readlump(fp, &hdr, i, &lumpsize[i]);
		if(lumps[i] == NULL) {
			goto bad;
		}
	}

	/* all data copied from file */
	fclose(fp);

	char    *entdata     = lumps[LUMP_ENTITIES];
	uint32_t entdatasize = lumpsize[LUMP_ENTITIES];

	/* we want all ptrs to be NULL */
	struct entity *entities = calloc(MAX_MAP_ENTITIES, sizeof(struct entity));

	int numentities = bsp_entparse(entdata, entdatasize, entities);

	printf("num entities: %d\n", numentities);
	for(int i = 0; i < numentities; i++) {
		entputs(&entities[i]);
		//char *classname = entgets(&entities[i], "classname");
		//if(strncmp(classname, "func_door", MAX_VALUE)) {
		//	printf("%s\n", classname);
		//}
	}

	/* entdata is not null terminated. using printf would be a bad idea. :P */
	//for(int i = 0; i < entdatasize; i++) {
	//	fputc(entdata[i], stdout);
	//}

	fflush(stdout);

	/* skip GL shit */
	goto end;

	GLFWwindow *window = NULL;

	if(glfwInit() != GLFW_TRUE) {
		goto bad_glfw;
	}
	/* Yes, I'm using an antiquated version of OpenGL.
	   I don't want to bother with extension loaders,
	   this is a study of the BSP file format, not modern OpenGL. */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "BSP Study", NULL, NULL);
	if(window == NULL) {
		goto bad_glfw;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	framebuffer_size_callback(window, WINDOW_WIDTH, WINDOW_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, WINDOW_WIDTH / WINDOW_HEIGHT, 0.1, 100.0);
	gluLookAt(1.0, 2.5, 100.0, 
	          0.0, 0.0, 0.0, 
	          0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// main loop
	for(;;) {
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(glfwGetTime()*90, 0.5, 1.0, 0.0);

		glfwSwapBuffers(window);
		glfwPollEvents();

		if(glfwWindowShouldClose(window)) {
			goto end;
		}
	}
bad_glfw: {
		const char *errmsg = NULL;
		glfwGetError(&errmsg);
		fprintf(stderr, "%s", errmsg);
	}
	/* fallthrough */
bad:
	status = EXIT_FAILURE;
	/* fallthrough */
end:
	for(int i = 0; i < HEADER_LUMPS; i++) {
		free(lumps[i]);
	}
	//glfwDestroyWindow(window);
	//glfwTerminate();
	return status;
}
