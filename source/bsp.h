
#ifndef _GOLDSRC_BSPFILE_H
#define _GOLDSRC_BSPFILE_H

#include <stdint.h>

#include "wad.h"
#include "entity.h"

enum {
	LUMP_ENTITIES     = 0,
	LUMP_PLANES       = 1,
	LUMP_TEXTURES     = 2,
	LUMP_VERTICES     = 3,
	LUMP_VISIBILITY   = 4,
	LUMP_NODES        = 5,
	LUMP_TEXINFO      = 6,
	LUMP_FACES        = 7,
	LUMP_LIGHTING     = 8,
	LUMP_CLIPNODES    = 9,
	LUMP_LEAVES       = 10,
	LUMP_MARKSURFACES = 11,
	LUMP_EDGES        = 12,
	LUMP_SURFEDGES    = 13,
	LUMP_MODELS       = 14,
	HEADER_LUMPS      = 15
};

typedef struct {
	int32_t offset;
	int32_t length;
} lump_t;


typedef struct {
	int32_t version;
	lump_t lump[HEADER_LUMPS];
} dheader_t;


enum {
	MAX_MAP_HULLS        = 4,
	MAX_MAP_MODELS       = 400,
	MAX_MAP_BRUSHES      = 4096,
	MAX_MAP_ENTITIES     = 1024,
	MAX_MAP_ENTSTRING    = (128*1024),
	MAX_MAP_PLANES       = 32767,
	MAX_MAP_NODES        = 32767,
	MAX_MAP_CLIPNODES    = 32767,
	MAX_MAP_LEAVES       = 8192,
	MAX_MAP_VERTS        = 65535,
	MAX_MAP_FACES        = 65535,
	MAX_MAP_MARKSURFACES = 65535,
	MAX_MAP_TEXINFO      = 8192,
	MAX_MAP_EDGES        = 256000,
	MAX_MAP_SURFEDGES    = 512000,
	MAX_MAP_TEXTURES     = 512,
	MAX_MAP_MIPTEX       = 0x200000,
	MAX_MAP_LIGHTING     = 0x200000,
	MAX_MAP_VISIBILITY   = 0x200000,
	MAX_MAP_PORTALS      = 65536,
	/* key value */
	EPAIR_MAX_KEY   = 32,
	EPAIR_MAX_VALUE = 1024,
	/* versions */
	BSPVERSION = 30,
	TOOLVERSION = 2
};


typedef struct {
	float mins[3];
	float maxs[3];
	float origin[3];
	int32_t headnode[MAX_MAP_HULLS];
	int32_t visleafs;
	int32_t firstface;
	int32_t numfaces;
} dmodel_t;


enum {
	PLANE_X = 0,
	PLANE_Y = 1,
	PLANE_Z = 2,
	PLANE_ANYX = 3,
	PLANE_ANYY = 4,
	PLANE_ANYZ = 4
};


typedef struct {
	float normal[3];
	float dist;
	int32_t type;
} dplane_t;


#define MAXLIGHTMAPS 4
typedef struct {
	int16_t plane;
	int16_t side;
	int32_t firstedge;
	int16_t numedges;
	int16_t texinfo;
	uint8_t styles[MAXLIGHTMAPS];
	int32_t lightmapoffset;
} dface_t;


typedef struct texinfo_s {
	float vecs[2][4]; /* [s/t][xyz offset] */
	int32_t miptex;
	int32_t flags;
} texinfo_t;

typedef struct {
	uint32_t plane;
	int16_t children[2];
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstface;
	uint16_t numfaces;
} dnode_t;

enum {
	CONTENTS_EMPTY = -1,
	CONTENTS_SOLID = -2,
	CONTENTS_WATER = -3,
	CONTENTS_SLIME = -4,
	CONTENTS_LAVA = -5,
	CONTENTS_SKY = -6,
	CONTENTS_ORIGIN = -7,
	CONTENTS_CLIP = -8,
	CONTENTS_CURRENT_0 = -9,
	CONTENTS_CURRENT_90 = -10,
	CONTENTS_CURRENT_180 = -11,
	CONTENTS_CURRENT_270 = -12,
	CONTENTS_CURRENT_UP = -13,
	CONTENTS_CURRENT_DOWN = -14,
	CONTENTS_TRANSLUCENT = -15
};

typedef struct {
	int32_t contents;
	int32_t vis;
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstmarksurface;
	uint16_t nummarksurfaces;
	uint8_t ambientlevels[4];
} dleaf_t;

typedef struct {
	int32_t plane;
	int16_t children[2];
} dclipnode_t;

typedef struct bsp_vertex {
	float point[3];
} dvertex_t;

typedef struct {
	uint16_t v[2];
} dedge_t;

typedef struct
{
	union {
		struct {
			union {
				char *entdata;
				entity_t *entities;
			};
			dplane_t *planes;
			union {
				uint8_t *textures;
				dmiptexlump_t *miphdr;
			};
			dvertex_t *vertices;
			uint8_t *vis;
			dnode_t *nodes;
			texinfo_t *texinfo;
			dface_t *faces;
			uint8_t *lighting;
			dclipnode_t *clipnodes;
			dleaf_t *leaves;
			uint16_t *marksurfaces;
			dedge_t *edges;
			int32_t *surfedges;
			dmodel_t *models;
		};
		void *lumps[HEADER_LUMPS];
	};

	union {
		struct {
			union {
				int32_t entdatasize;
				int32_t numentities;
			};
			int32_t numplanes;
			int32_t miptexsize;
			int32_t numvertices;
			int32_t vissize;
			int32_t numnodes;
			int32_t numtexinfos;
			int32_t numfaces; 
			int32_t lightingsize;
			int32_t numclipnodes;
			int32_t numleaves;
			int32_t nummarksurfaces;
			int32_t numedges;
			int32_t numsurfedges;
			int32_t nummodels;
		};
		int32_t lumpsize[HEADER_LUMPS];
	};
} bsp_t;

bsp_t *bsp_open(const char *filename);
const char *bsp_wadname(const bsp_t *bsp);
void bsp_free(bsp_t *bsp);
int32_t bsp_pointinleaf(const bsp_t *bsp, const float origin[3]);
void bsp_pvsfororigin(const bsp_t *bsp, const float origin[3], uint8_t *pvs);


#endif
