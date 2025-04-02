
#ifndef _GOLDSRC_BSPFILE_H
#define _GOLDSRC_BSPFILE_H

#include <stdbool.h>
#include <stdint.h>

#include "wad.hpp"
#include "entity.hpp"

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

struct lump_t {
	int32_t offset;
	int32_t length;
};


struct dheader_t {
	int32_t version;
	lump_t lump[HEADER_LUMPS];
};


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


struct dmodel_t {
	float mins[3];
	float maxs[3];
	float origin[3];
	int32_t headnode[MAX_MAP_HULLS];
	int32_t visleafs;
	int32_t firstface;
	int32_t numfaces;
};


enum {
	PLANE_X = 0,
	PLANE_Y = 1,
	PLANE_Z = 2,
	PLANE_ANYX = 3,
	PLANE_ANYY = 4,
	PLANE_ANYZ = 4
};


struct dplane_t {
	glm::vec3 normal;
	float dist;
	int32_t type;
};


#define MAXLIGHTMAPS 4
struct dface_t {
	int16_t plane;
	int16_t side;
	int32_t firstedge;
	int16_t numedges;
	int16_t texinfo;
	uint8_t styles[MAXLIGHTMAPS];
	int32_t lightmapoffset;
};


struct texinfo_t {
	float vecs[2][4]; /* [s/t][xyz offset] */
	int32_t miptex;
	int32_t flags;
};

struct dnode_t {
	uint32_t plane;
	int16_t children[2];
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstface;
	uint16_t numfaces;
};

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

struct dleaf_t {
	int32_t contents;
	int32_t vis;
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstmarksurface;
	uint16_t nummarksurfaces;
	uint8_t ambientlevels[4];
};

struct dclipnode_t {
	int32_t plane;
	int16_t children[2];
};

struct dvertex_t {
	float point[3];
};

struct dedge_t {
	uint16_t v[2];
};

struct bsp_t
{
public:
	bool open(const char *filename);
	~bsp_t();

	const char *wadname() const;
	int32_t pointinleaf(const glm::vec3 &point) const;
	void pvsfororigin(const glm::vec3 &point, uint8_t *pvs) const;
	bool getspawn(glm::vec3 &origin, float &yaw) const;

	/* pointers to lumps */
	union {
		void *datastart;
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

	/* lump sizes */
	union {
		int32_t sizestart;
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

#endif
