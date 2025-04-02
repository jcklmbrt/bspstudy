#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#define _USE_MATH_DEFINES
#include <math.h>

#include "entity.hpp"
#include "bsp.hpp"
#include "wad.hpp"


static const char *lumpname(int lump_num)
{
	switch(lump_num) {
	case LUMP_ENTITIES:     return "LUMP_ENTITIES";
	case LUMP_PLANES:       return "LUMP_PLANES";
	case LUMP_TEXTURES:     return "LUMP_TEXTURES";
	case LUMP_VERTICES:     return "LUMP_VERTICES";
	case LUMP_VISIBILITY:   return "LUMP_VISIBILITY";
	case LUMP_NODES:        return "LUMP_NODES";
	case LUMP_TEXINFO:      return "LUMP_TEXINFO";
	case LUMP_FACES:        return "LUMP_FACES";
	case LUMP_LIGHTING:     return "LUMP_LIGHTING";
	case LUMP_CLIPNODES:    return "LUMP_CLIPNODES";
	case LUMP_LEAVES:       return "LUMP_LEAVES";
	case LUMP_MARKSURFACES: return "LUMP_MARKSURFACES";
	case LUMP_EDGES:        return "LUMP_EDGES";
	case LUMP_SURFEDGES:    return "LUMP_SURFEDGES";
	case LUMP_MODELS:       return "LUMP_MODELS";
	case HEADER_LUMPS:      return "HEADER_LUMPS";
	default:                return NULL;
	}
}


static void *readlump(FILE *fp, dheader_t *hdr, int lumpid, int32_t *elemcount)
{
	static int32_t maxsizes[HEADER_LUMPS] = {
		/* LUMP_ENTITIES     */ MAX_MAP_ENTSTRING,
		/* LUMP_PLANES       */ MAX_MAP_PLANES,
		/* LUMP_TEXTURES     */ MAX_MAP_MIPTEX,
		/* LUMP_VERTICES     */ MAX_MAP_VERTS,
		/* LUMP_VISIBILITY   */ MAX_MAP_VISIBILITY,
		/* LUMP_NODES        */ MAX_MAP_NODES,
		/* LUMP_TEXINFO      */ MAX_MAP_TEXINFO,
		/* LUMP_FACES        */ MAX_MAP_FACES,
		/* LUMP_LIGHTING     */ MAX_MAP_LIGHTING,
		/* LUMP_CLIPNODES    */ MAX_MAP_CLIPNODES,
		/* LUMP_LEAVES       */ MAX_MAP_LEAVES,
		/* LUMP_MARKSURFACES */ MAX_MAP_MARKSURFACES,
		/* LUMP_EDGES        */ MAX_MAP_EDGES,
		/* LUMP_SURFEDGES    */ MAX_MAP_SURFEDGES,
		/* LUMP_MODELS       */ MAX_MAP_MODELS
	};

	static int32_t elemsizes[HEADER_LUMPS] = {
		/* LUMP_ENTITIES     */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_PLANES       */ sizeof(dplane_t),
		/* LUMP_TEXTURES     */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_VERTICES     */ sizeof(dvertex_t),
		/* LUMP_VISIBILITY   */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_NODES        */ sizeof(dnode_t),
		/* LUMP_TEXINFO      */ sizeof(texinfo_t),
		/* LUMP_FACES        */ sizeof(dface_t),
		/* LUMP_LIGHTING     */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_CLIPNODES    */ sizeof(dclipnode_t),
		/* LUMP_LEAVES       */ sizeof(dleaf_t),
		/* LUMP_MARKSURFACES */ sizeof(uint16_t),
		/* LUMP_EDGES        */ sizeof(dedge_t),
		/* LUMP_SURFEDGES    */ sizeof(int32_t),
		/* LUMP_MODELS       */ sizeof(dmodel_t)
	};

	void *data = NULL;
	int32_t length = hdr->lump[lumpid].length;
	int32_t offset = hdr->lump[lumpid].offset;
	int32_t maxsize = maxsizes[lumpid];
	int32_t elemsize = elemsizes[lumpid];
	
	if(length % elemsize != 0) {
		fprintf(stderr, "loadlump %s: bad length: %d\n",
			lumpname(lumpid), length);
		return NULL;
	}

	if(length / elemsize > maxsize) {
		fprintf(stderr, "waring: lump %s too large (%d > %d)\n",
			lumpname(lumpid), length / elemsize, maxsize);
	}
	
	if((data = malloc(length)) == NULL) {
		fprintf(stderr, "loadlump %s: bad alloc\n",
			lumpname(lumpid));
		return NULL;
	}

	if(fseek(fp, offset, SEEK_SET) != 0) {
		fprintf(stderr, "loadlump %s: bad offset/seek %d\n",
			lumpname(lumpid), offset);
		free(data);
		return NULL;
	};

	if(fread(data, 1, length, fp) != (size_t)length) {
		fprintf(stderr, "loadlump %s: bad read\n",
			lumpname(lumpid));
		free(data);
		return NULL;
	};

	*elemcount = length / elemsize;
	return data;
}


bsp_t::~bsp_t()
{
	void **lumps = &datastart;

	for(int i = 0; i < HEADER_LUMPS; i++) {
		free(lumps[i]);
	}
}


const char *bsp_t::wadname() const
{
	const char *wadname = NULL;
	for(int i = 0; i < numentities; i++) {
		const char *classname = entities[i].get("classname");
		if(classname == NULL) {
			continue;
		}
		if(!strncmp(classname, "worldspawn", EPAIR_MAX_KEY)) {
			wadname = entities[i].get("wad");
		}
	}
	if(wadname == NULL) {
		return NULL;
	}
	const char *end = wadname;
	while(*end != '\0') {
		end++;
	}
	for(; end != wadname; end--) {
		if(*end == '\\' || *end =='/') {
			wadname = end + 1;
			break;
		}
	}
	return wadname;
}


static void decompressvis(const bsp_t &bsp, const uint8_t *in, uint8_t *decompressed)
{
	int32_t row = (bsp.numleaves + 7) >> 3;
	uint8_t *out = decompressed;
	while(out - decompressed < row) {
		if(*in) {
			*out++ = *in;
		} else {
			in++;
			for(int32_t i = 0; i < *in; i++) {
				*out++ = 0;
				if(out - decompressed >= row) {
					// bad rle?
					return;
				}
			}
		}
		in++;
	}
}


int32_t bsp_t::pointinleaf(const glm::vec3 &origin) const
{
	dmodel_t *mdl = &models[0];
	int16_t index = mdl->headnode[0];

	while(index >= 0) {
		dnode_t *node = &nodes[index];
		dplane_t *plane = &planes[node->plane];
		float dist = 0;
		switch(plane->type) {
			case PLANE_X: dist = origin[0] - plane->dist; break;
			case PLANE_Y: dist = origin[1] - plane->dist; break;
			case PLANE_Z: dist = origin[2] - plane->dist; break;
		default:
			dist = glm::dot(plane->normal, origin) - plane->dist;
			break;
		}
		index = node->children[dist > 0 ? 0 : 1];
	}
	return -index - 1;
}


void bsp_t::pvsfororigin(const glm::vec3 &origin, uint8_t *out) const
{
	int32_t i = pointinleaf(origin);
	dleaf_t *leaf = &leaves[i];

	memset(out, 255, (numleaves + 7) / 8);

	if(leaf->vis == -1) {
		return;
	}

	decompressvis(*this, &vis[leaf->vis], out);
}


bool bsp_t::getspawn(glm::vec3 &origin, float &yaw) const
{
	yaw = 0;
	origin[0] = origin[1] = origin[2] = 0.0f;
	
	for(int32_t i = 0; i < numentities; i++) {
		const char *classname = entities[i].get("classname");
		if(classname == NULL) {
			continue;
		}
		// get spawn point
		if(!strncmp(classname, "info_player_start", EPAIR_MAX_KEY)) {
			entities[i].get("origin", origin);
			entities[i].get("angle", yaw);

			yaw = (yaw * (M_PI_2 / 180.0f));
			return true;
		}
	}
	return false;
}


bool bsp_t::open(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	
	if(fp == NULL) {
		fprintf(stderr, "%s: failed to open file\n", filename);
		return false;
	}

	dheader_t hdr;
	fseek(fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(dheader_t), fp);

	void **lumps = &datastart;
	int32_t *lumpsize = &sizestart;
	
	for(int i = 0; i < HEADER_LUMPS; i++) {
		lumps[i] = readlump(fp, &hdr, i, &lumpsize[i]);
		if(lumps[i] == NULL) {
			fclose(fp);
			return false;
		}
	}

	entity_t *p_entities = NULL;
	p_entities = new entity_t[MAX_MAP_ENTITIES];

	numentities = entity_t::parse(entdata, entdatasize, p_entities);

	delete[] entdata;
	entities = p_entities;

	return true;
}
