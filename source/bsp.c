
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "v_math.h"
#include "entity.h"
#include "bsp.h"
#include "wad.h"


static const char *bsp_lumpname(int lump_num)
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


static void *bsp_readlump(FILE *fp, dheader_t *hdr, int lumpid, int32_t *elemcount)
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
			bsp_lumpname(lumpid), length);
		return NULL;
	}

	if(length / elemsize > maxsize) {
		fprintf(stderr, "waring: lump %s too large (%d > %d)\n",
			bsp_lumpname(lumpid), length / elemsize, maxsize);
	}
	
	if((data = malloc(length)) == NULL) {
		fprintf(stderr, "loadlump %s: bad alloc\n",
			bsp_lumpname(lumpid));
		return NULL;
	}

	if(fseek(fp, offset, SEEK_SET) != 0) {
		fprintf(stderr, "loadlump %s: bad offset/seek %d\n",
			bsp_lumpname(lumpid), offset);
		free(data);
		return NULL;
	};

	if(fread(data, 1, length, fp) != (size_t)length) {
		fprintf(stderr, "loadlump %s: bad read\n",
			bsp_lumpname(lumpid));
		free(data);
		return NULL;
	};

	*elemcount = length / elemsize;
	return data;
}


void bsp_free(bsp_t *bsp)
{
	if(bsp == NULL) {
		return;
	}
	
	for(int i = 0; i < HEADER_LUMPS; i++) {
		free(bsp->lumps[i]);
	}

	free(bsp);
}


const char *bsp_wadname(const bsp_t *bsp)
{
	const char *wadname = NULL;
	for(int i = 0; i < bsp->numentities; i++) {
		const char *classname = entgets(&bsp->entities[i], "classname");
		if(!strncmp(classname, "worldspawn", EPAIR_MAX_KEY)) {
			wadname = entgets(&bsp->entities[i], "wad");
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


void bsp_decompressvis(const bsp_t *bsp, const uint8_t *in, uint8_t *decompressed)
{
	int32_t row = (bsp->numleaves + 7) >> 3;
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


int32_t bsp_pointinleaf(const bsp_t *bsp, const float origin[3])
{
	dmodel_t *mdl = &bsp->models[0];
	int16_t index = mdl->headnode[0];

	while(index >= 0) {
		dnode_t *node = &bsp->nodes[index];
		dplane_t *plane = &bsp->planes[node->plane];
		float dist = 0;
		switch(plane->type) {
			case PLANE_X: dist = origin[0] - plane->dist; break;
			case PLANE_Y: dist = origin[1] - plane->dist; break;
			case PLANE_Z: dist = origin[2] - plane->dist; break;
		default:
			dist = v3dot(plane->normal, origin) - plane->dist;
			break;
		}
		index = node->children[dist > 0 ? 0 : 1];
	}
	return -index - 1;
}


void bsp_pvsfororigin(const bsp_t *bsp, const float origin[3], uint8_t *out)
{
	int32_t i = bsp_pointinleaf(bsp, origin);
	dleaf_t *leaf = &bsp->leaves[i];

	memset(out, 255, (bsp->numleaves + 7) / 8);

	if(leaf->vis == -1) {
		return;
	}

	bsp_decompressvis(bsp, &bsp->vis[leaf->vis], out);
}


bsp_t *bsp_open(const char *filename)
{
	bsp_t *bsp = NULL;
	bsp = calloc(1, sizeof(bsp_t));

	if(bsp == NULL) {
		return NULL;
	}
	
	memset(bsp, 0, sizeof(bsp_t));

	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) {
		fprintf(stderr, "%s: failed to open file\n", filename);
		goto bad;
	}

	dheader_t hdr;
	fseek(fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(dheader_t), fp);
	
	for(int i = 0; i < HEADER_LUMPS; i++) {
		bsp->lumps[i] = bsp_readlump(fp, &hdr, i, &bsp->lumpsize[i]);
		if(bsp->lumps[i] == NULL) {
			goto bad;
		}
	}

	entity_t *entities = NULL;
	entities = calloc(MAX_MAP_ENTITIES, sizeof(entity_t));

	int32_t numentities = entparse(bsp->entdata, bsp->entdatasize, entities);
	
	free(bsp->entdata);
	bsp->entities = entities;
	bsp->numentities = numentities;

	return bsp;
bad:		
	if(fp != NULL) {
		fclose(fp);
	}

	bsp_free(bsp);

	return NULL;
}
