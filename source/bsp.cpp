#include <iostream>
#include <cstring>
#include <cstdlib>

#include <glm/glm.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include "entity.hpp"
#include "bsp.hpp"

#include "little_endian.hpp"
namespace le = little_endian;

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


bsp_t::~bsp_t()
{
	delete[] entities;
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
	int32_t row = (bsp.m_leaves.size() + 7) >> 3;
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
	const dmodel_t *mdl = &m_models[0];
	int16_t index = mdl->headnode[0];

	while(index >= 0) {
		const dnode_t *node = &m_nodes[index];
		const dplane_t *plane = &m_planes[node->plane];
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
	const dleaf_t *leaf = &m_leaves[i];

	memset(out, 255, (m_leaves.size() + 7) / 8);

	if(leaf->vis == -1) {
		return;
	}

	decompressvis(*this, &m_vis[leaf->vis], out);
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
	std::vector<uint8_t> bytes;
	FILE *fp = fopen(filename, "rb");
	if(fp == nullptr) {
		std::cerr << filename << ": failed to open file" << std::endl;
		return false;
	}

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);

	bytes.resize(size);

	fseek(fp, 0, SEEK_SET);
	if(fread(bytes.data(), 1, size, fp) != size) {
		std::cerr << filename << ": failed to read file" << std::endl;
		return false;
	}

	dheader_t hdr;
	size_t ofs = 0;
	le::read(bytes.data(), ofs, hdr.version);
	if(hdr.version != 30) {
		std::cerr << filename << ": bad bsp version" << std::endl
			  << "\texpected 30, got: " << hdr.version << std::endl;
		return false;
	}
	
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

	int32_t lumpsizes[HEADER_LUMPS];
	for(int32_t i = 0; i < HEADER_LUMPS; i++) {
		le::read(bytes.data(), ofs, hdr.lump[i].offset);
		le::read(bytes.data(), ofs, hdr.lump[i].length);
		if(hdr.lump[i].length % elemsizes[i] != 0) {
			std::cerr << filename << ": "
				  << lumpname(i)
				  << "bad lump size"
				  << std::endl;
				return false;
		}
		lumpsizes[i] = hdr.lump[i].length / elemsizes[i];
	}
	
	/* LUMP_ENTITIES */
	m_entdata.assign(
		bytes.begin() + hdr.lump[LUMP_ENTITIES].offset,
		bytes.begin() + hdr.lump[LUMP_ENTITIES].offset +
		hdr.lump[LUMP_ENTITIES].length);

	/* LUMP_PLANES */
	ofs = hdr.lump[LUMP_PLANES].offset;
	m_planes.resize(lumpsizes[LUMP_PLANES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_PLANES]; i++) {
		le::read(bytes.data(), ofs, m_planes[i].normal);
		le::read(bytes.data(), ofs, m_planes[i].dist);
		le::read(bytes.data(), ofs, m_planes[i].type);
	}

	/* LUMP_TEXTURES */
	int32_t nummiptex;
	ofs = hdr.lump[LUMP_TEXTURES].offset;
	le::read(bytes.data(), ofs, nummiptex);
	if(nummiptex > lumpsizes[LUMP_TEXTURES] / 4) {
		std::cerr << filename
			  << ": bad number of textures: "
			  << nummiptex << std::endl; 
	}

	m_mipofs.resize(nummiptex);
	for(int32_t i = 0; i < nummiptex; i++) {
		le::read(bytes.data(), ofs, m_mipofs[i]);
	}

	m_textures.assign(
		bytes.begin() + hdr.lump[LUMP_TEXTURES].offset,
		bytes.begin() + hdr.lump[LUMP_TEXTURES].offset +
		hdr.lump[LUMP_TEXTURES].length);

	/* LUMP_VERTICES */
	ofs = hdr.lump[LUMP_VERTICES].offset;
	m_vertices.resize(lumpsizes[LUMP_VERTICES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_VERTICES]; i++) {
		le::read(bytes.data(), ofs, m_vertices[i].point[0]);
		le::read(bytes.data(), ofs, m_vertices[i].point[1]);
		le::read(bytes.data(), ofs, m_vertices[i].point[2]);
	}

	/* LUMP_VISIBILITY */
	m_vis.assign(
		bytes.begin() + hdr.lump[LUMP_VISIBILITY].offset,
		bytes.begin() + hdr.lump[LUMP_VISIBILITY].offset +
		hdr.lump[LUMP_VISIBILITY].length);

	/* LUMP_NODES */
	ofs = hdr.lump[LUMP_NODES].offset;
	m_nodes.resize(lumpsizes[LUMP_NODES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_NODES]; i++) {
		le::read(bytes.data(), ofs, m_nodes[i].plane);
		le::read(bytes.data(), ofs, m_nodes[i].children[0]);
		le::read(bytes.data(), ofs, m_nodes[i].children[1]);
		le::read(bytes.data(), ofs, m_nodes[i].mins[0]);
		le::read(bytes.data(), ofs, m_nodes[i].mins[1]);
		le::read(bytes.data(), ofs, m_nodes[i].mins[2]);
		le::read(bytes.data(), ofs, m_nodes[i].maxs[0]);
		le::read(bytes.data(), ofs, m_nodes[i].maxs[1]);
		le::read(bytes.data(), ofs, m_nodes[i].maxs[2]);
		le::read(bytes.data(), ofs, m_nodes[i].firstface);
		le::read(bytes.data(), ofs, m_nodes[i].numfaces);
	}

	/* LUMP_TEXINFO */
	ofs = hdr.lump[LUMP_TEXINFO].offset;
	m_texinfo.resize(lumpsizes[LUMP_TEXINFO]);
	for(int32_t i = 0; i < lumpsizes[LUMP_TEXINFO]; i++) {
		for(int32_t x = 0; x < 2; x++) {
			for(int32_t y = 0; y < 4; y++) {
				le::read(bytes.data(), ofs, m_texinfo[i].vecs[x][y]);
			}
		}
		le::read(bytes.data(), ofs, m_texinfo[i].miptex);
		le::read(bytes.data(), ofs, m_texinfo[i].flags);
	}

	/* LUMP_FACES */
	ofs = hdr.lump[LUMP_FACES].offset;
	m_faces.resize(lumpsizes[LUMP_FACES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_FACES]; i++) {
		le::read(bytes.data(), ofs, m_faces[i].plane);
		le::read(bytes.data(), ofs, m_faces[i].side);
		le::read(bytes.data(), ofs, m_faces[i].firstedge);
		le::read(bytes.data(), ofs, m_faces[i].numedges);
		le::read(bytes.data(), ofs, m_faces[i].texinfo);
		for(int32_t j = 0; j < MAXLIGHTMAPS; j++) {
			m_faces[i].styles[j] = bytes[ofs+j];
		}
		ofs += 4;
		le::read(bytes.data(), ofs, m_faces[i].lightmapoffset);
	}

	/* LUMP_LIGHTING */
	m_lighting.assign(
		bytes.begin() + hdr.lump[LUMP_LIGHTING].offset,
		bytes.begin() + hdr.lump[LUMP_LIGHTING].offset +
		hdr.lump[LUMP_LIGHTING].length);

	/* LUMP_CLIPNODES */
	ofs = hdr.lump[LUMP_CLIPNODES].offset;
	m_clipnodes.resize(lumpsizes[LUMP_CLIPNODES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_CLIPNODES]; i++) {
		le::read(bytes.data(), ofs, m_clipnodes[i].plane);
		le::read(bytes.data(), ofs, m_clipnodes[i].children[0]);
		le::read(bytes.data(), ofs, m_clipnodes[i].children[1]);
	}

	/* LUMP_LEAVES */
	ofs = hdr.lump[LUMP_LEAVES].offset;
	m_leaves.resize(lumpsizes[LUMP_LEAVES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_LEAVES]; i++) {
		le::read(bytes.data(), ofs, m_leaves[i].contents);
		le::read(bytes.data(), ofs, m_leaves[i].vis);
		le::read(bytes.data(), ofs, m_leaves[i].mins[0]);
		le::read(bytes.data(), ofs, m_leaves[i].mins[1]);
		le::read(bytes.data(), ofs, m_leaves[i].mins[2]);
		le::read(bytes.data(), ofs, m_leaves[i].maxs[0]);
		le::read(bytes.data(), ofs, m_leaves[i].maxs[1]);
		le::read(bytes.data(), ofs, m_leaves[i].maxs[2]);
		le::read(bytes.data(), ofs, m_leaves[i].firstmarksurface);
		le::read(bytes.data(), ofs, m_leaves[i].nummarksurfaces);
		for(int j = 0; j < 4; j++) {
			m_leaves[i].ambientlevels[j] = bytes[ofs++];
		}
	}

	/* LUMP_MARKSURFACES */
	ofs = hdr.lump[LUMP_MARKSURFACES].offset;
	m_marksurfaces.resize(lumpsizes[LUMP_MARKSURFACES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_MARKSURFACES]; i++) {
		le::read(bytes.data(), ofs, m_marksurfaces[i]);
	}

	/* LUMP_EDGES */
	ofs = hdr.lump[LUMP_EDGES].offset;
	m_edges.resize(lumpsizes[LUMP_EDGES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_EDGES]; i++) {
		le::read(bytes.data(), ofs, m_edges[i].v[0]);
		le::read(bytes.data(), ofs, m_edges[i].v[1]);
	}

	/* LUMP_SURFEDGES */
	ofs = hdr.lump[LUMP_SURFEDGES].offset;
	m_surfedges.resize(lumpsizes[LUMP_SURFEDGES]);
	for(int32_t i = 0; i < lumpsizes[LUMP_SURFEDGES]; i++) {
		le::read(bytes.data(), ofs, m_surfedges[i]);
	}

	/* LUMP_MODELS */
	ofs = hdr.lump[LUMP_MODELS].offset;
	m_models.resize(lumpsizes[LUMP_MODELS]);
	for(int32_t i = 0; i < lumpsizes[LUMP_MODELS]; i++) {
		le::read(bytes.data(), ofs, m_models[i].mins);
		le::read(bytes.data(), ofs, m_models[i].maxs);
		le::read(bytes.data(), ofs, m_models[i].origin);
		for(int32_t j = 0; j < MAX_MAP_HULLS; j++) {
			le::read(bytes.data(), ofs, m_models[i].headnode[j]);
		}
		le::read(bytes.data(), ofs, m_models[i].visleafs);
		le::read(bytes.data(), ofs, m_models[i].firstface);
		le::read(bytes.data(), ofs, m_models[i].numfaces);
	}

	entities = new entity_t[MAX_MAP_ENTITIES];
	numentities = entity_t::parse(m_entdata.data(), m_entdata.size(), entities);

	return true;
}
