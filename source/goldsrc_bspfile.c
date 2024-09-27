
#include <stdio.h>
#include <stdlib.h>
#include <goldsrc_bspfile.h>

static const char *bsp_lump_name(int lump_num)
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
	default:
		break;
	}
}


void *bsp_readlump(FILE *fp, struct bsp_header *hdr, int lumpid, int32_t *elemcount)
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
		/* LUMP_PLANES       */ sizeof(struct bsp_plane),
		/* LUMP_TEXTURES     */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_VERTICES     */ sizeof(struct bsp_vertex),
		/* LUMP_VISIBILITY   */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_NODES        */ sizeof(struct bsp_node),
		/* LUMP_TEXINFO      */ sizeof(struct bsp_texinfo),
		/* LUMP_FACES        */ sizeof(struct bsp_face),
		/* LUMP_LIGHTING     */ sizeof(char), /* BYTE ARRAY */
		/* LUMP_CLIPNODES    */ sizeof(struct bsp_clipnode),
		/* LUMP_LEAVES       */ sizeof(struct bsp_leaf),
		/* LUMP_MARKSURFACES */ sizeof(uint16_t),
		/* LUMP_EDGES        */ sizeof(struct bsp_edge),
		/* LUMP_SURFEDGES    */ sizeof(int32_t),
		/* LUMP_MODELS       */ sizeof(struct bsp_model)
	};

	void *data = NULL;
	int32_t length = hdr->lump[lumpid].length;
	int32_t offset = hdr->lump[lumpid].offset;
	int32_t maxsize = maxsizes[lumpid];
	int32_t elemsize = elemsizes[lumpid];

	printf("\t%-18s{ length: %6X, offset: %6X }\n",
		bsp_lump_name(lumpid), length, offset);

	if(length % elemsize != 0 || length / elemsize > maxsize) {
		fprintf(stderr, "loadlump %s: bad length: %d\n",
			bsp_lump_name(lumpid), length);
		return NULL;
	}

	if((data = malloc(length)) == NULL) {
		fprintf(stderr, "loadlump %s: bad alloc\n",
			bsp_lump_name(lumpid));
		return NULL;
	}

	if(fseek(fp, offset, SEEK_SET) != 0) {
		fprintf(stderr, "loadlump %s: bad offset/seek %d\n",
			bsp_lump_name(lumpid), offset);
		free(data);
		return NULL;
	};

	if(fread(data, 1, length, fp) != length) {
		fprintf(stderr, "loadlump %s: bad read\n",
			bsp_lump_name(lumpid));
		free(data);
		return NULL;
	};

	*elemcount = length / elemsize;
	return data;
}
