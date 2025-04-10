#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wad.hpp"

#include "little_endian.hpp"
namespace le = little_endian;

struct texatlas_t {
	//stbrp_rect *m_rects;
};

bool wad_t::open(const char *filename)
{
	// from my tests less than 10% of the textures in the wad file are actually used by the map.
	// let's keep a file handle open and only read what we need.
	fp = fopen(filename, "rb");
	if(fp == nullptr) {
		return false;
	}

	uint8_t hdr[sizeof(wadinfo_t)];
	fseek(fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(wadinfo_t), fp);

	if(hdr[0] != 'W' || hdr[1] != 'A' ||
	   hdr[2] != 'D' || (hdr[3] != '2' && hdr[3] != '3')) {
		fprintf(stderr, "wad_open: %s bad magic (%c%c%c%c)\n",
			filename, hdr[0], hdr[1], hdr[2], hdr[3]);
		return false;
	}

	int32_t infotableofs;
	size_t ofs = offsetof(wadinfo_t, numlumps);
	le::read(hdr, ofs, numdirs);
	le::read(hdr, ofs, infotableofs);

	cache = new uint8_t *[numdirs];
	if(cache == nullptr) {
		return false;
	}

	memset(cache, 0, sizeof(miptex_t *) * numdirs);

	dirs = new lumpinfo_t[numdirs];
	if(dirs == nullptr) {
		return false;
	}

	if(fseek(fp, infotableofs, SEEK_SET) != 0) {
		fprintf(stderr, "wad_open %s: bad offset/seek %d\n",
			filename, infotableofs);
		return false;
	};

	std::vector<uint8_t> infotablebytes;
	infotablebytes.resize(sizeof(lumpinfo_t) * numdirs);

	if(fread(infotablebytes.data(), 1, infotablebytes.size(), fp) != infotablebytes.size()) {
		fprintf(stderr, "wad_open %s: bad read\n", filename);
		return false;
	}

	ofs = 0;
	for(int32_t i = 0; i < numdirs; i++) {
		le::read(infotablebytes.data(), ofs, dirs[i].filepos);
		le::read(infotablebytes.data(), ofs, dirs[i].disksize);
		le::read(infotablebytes.data(), ofs, dirs[i].size);
		dirs[i].type = infotablebytes[ofs++];
		dirs[i].compression = infotablebytes[ofs++];
		ofs += sizeof(dirs[i].padding);
		for(int32_t j = 0; j < MAXTEXTURENAME; j++) {
			dirs[i].name[j] = infotablebytes[ofs++];
		}
	}

	return true;
}

static bool strncaseeq(const char *a, const char *b, size_t n)
{
	for(size_t i = 0; i < n; i++) {
		if(a[i] == '\0' && b[i] == '\0') {
			break;
		}
		char a_up = toupper(a[i]);
		char b_up = toupper(b[i]);
		if(a_up != b_up) {
			return false;
		}
	}
	return true;
}


bool miptex_t::load(const uint8_t *bytes)
{
	size_t ofs = 0;
	for(size_t i = 0; i < MAXTEXTURENAME; i++) {
		name[i] = bytes[ofs++];
	}
	le::read(bytes, ofs, width);
	le::read(bytes, ofs, height);
	for(size_t i = 0; i < MIPLEVELS; i++) {
		le::read(bytes, ofs, offsets[i]);
	}

	if(offsets[0] == 0) {
		return false;
	}

	const uint8_t *palette = bytes + offsets[3] + (width / 8) * (height / 8) + 2;
	const uint8_t *mip = bytes + offsets[0];

	rgba.resize(width * height * 4);
	for(size_t i = 0; i < width * height; i++) {
		rgba[i * 4 + 0] = palette[mip[i] * 3 + 0];
		rgba[i * 4 + 1] = palette[mip[i] * 3 + 1];
		rgba[i * 4 + 2] = palette[mip[i] * 3 + 2];
		if(mip[i] == 255 && bytes[0] == '{') {
			for(size_t j = 0; j < 4; j++) {
				rgba[i * 4 + j] = 0;
			}
		} else {
			rgba[i * 4 + 3] = 0xFF;
		}
	}

	return true;
}


bool miptex_t::load(const wad_t &wad, const char *name)
{
	const uint8_t *bytes = wad.getmiptexbytes(name);
	if(bytes == nullptr) {
		return false;
	}

	if(!load(bytes)) {
		delete[] bytes;
		return false;
	}

	delete[] bytes;
	
	if(!strncaseeq(this->name, name, MAXTEXTURENAME)) {
		fprintf(stderr,
			"warning: miptex name does not match directory name.\n"
			"\texpected: %s, got %s\n", name, this->name);
	}

	return true;
}


const uint8_t *wad_t::getmiptexbytes(const char *name) const
{
	lumpinfo_t *dir = nullptr;
	for(int32_t i = 0; i < numdirs; i++) {
		if(strncaseeq(name, dirs[i].name, MAXTEXTURENAME)) {
			dir = &dirs[i];
			break;
		}
	}

	if(dir == nullptr) {
		return nullptr;
	}

	// TODO: support compression
	if(dir->compression != CMP_NONE) {
		fprintf(stderr, "texture %s is compressed, bspstudy does not support compression", name);
		return nullptr;
	}

	if(dir->type != TYP_MIPTEX) {
		fprintf(stderr, "texture %s is not in miptex format\n", name);
		return nullptr;
	}

	uint8_t *mt = new uint8_t[dir->size];
	if(mt == nullptr) {
		return nullptr;
	}

	fseek(fp, dir->filepos, SEEK_SET);
	fread(mt, 1, dir->size, fp);

	return mt;
}


wad_t::~wad_t()
{
	if(fp != nullptr) {
		fclose(fp);
	}
	delete[] dirs;
}
