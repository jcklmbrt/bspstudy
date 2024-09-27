
#ifndef _GOLDSRC_BSPFILE_H
#define _GOLDSRC_BSPFILE_H

/* reimplementation of valve's bspfile.h header
   https://developer.valvesoftware.com/wiki/BSP_(GoldSrc) 
   https://github.com/ValveSoftware/halflife/blob/master/utils/common/bsplib.h
   figure it out buddy boyo
*/


#include <stdint.h>


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

struct bsp_lump {
	int32_t offset;
	int32_t length;
};


struct bsp_header {
	int32_t         version;
	struct bsp_lump lump[HEADER_LUMPS];
};


enum bsp_maxs {
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
	MAX_KEY   = 32,
	MAX_VALUE = 1024
};


enum bsp_version {
	BSPVERSION = 30,
	TOOLVERSION = 2,
};


struct bsp_model {
	float   mins[3];
	float   maxs[3];
	float   origin[3];
	int32_t headnode[MAX_MAP_HULLS];
	int32_t visleafs;
	int32_t firstface;
	int32_t numfaces;
};


enum bsp_plane_type {
	PLANE_X = 0,
	PLANE_Y = 1,
	PLANE_Z = 2,
	PLANE_ANYX = 3,
	PLANE_ANYY = 4,
	PLANE_ANYZ = 4
};


struct bsp_plane {
	float   normal[3];
	float   dist;
	int32_t type;
};

/* signed/unsigned ambiguity in documentation :/ */
#define MAXLIGHTMAPS 4
struct bsp_face {
	int16_t plane;
	int16_t side;
	int32_t firstedge;
	int16_t numedges;
	int16_t texinfo;
	char    styles[MAXLIGHTMAPS];
	int32_t lightmapoffset;
};


#define MAXTEXTURENAME 16
#define MIPLEVELS       4
struct bsp_miptex {
	char name[MAXTEXTURENAME];
	int32_t width;
	int32_t height;
	int32_t offsets[MIPLEVELS];
};


struct bsp_texinfo {
	float vecs[2][4]; /* [s/t][xyz] */
	int32_t miptex;
	int32_t flags;
};


struct bsp_node {
	uint32_t plane;
	int16_t children[2];
	int16_t mins[3];
	int16_t maxs[3];
	uint32_t firstface;
	uint32_t numfaces;
};


struct bsp_leaf {
	int32_t contents;
	int32_t vis;
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstmarksurface;
	uint16_t nummarksurfaces;
	uint8_t ambientlevels[4];
};


struct bsp_clipnode {
	int32_t plane;
	int16_t children[2];
};

struct bsp_vertex {
	float x;
	float y;
	float z;
};

struct bsp_edge {
	uint16_t v0, v1;
};

void *bsp_readlump(FILE *fp, struct bsp_header *hdr, int lumpid, int32_t *elemcount);

#endif