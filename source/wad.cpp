#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wad.hpp"

bool wad_t::open(const char *filename)
{
	fp = fopen(filename, "rb");
	if(fp == nullptr) {
		return false;
	}

	wadinfo_t hdr;
	fseek(fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(wadinfo_t), fp);

	if(hdr.id[0] != 'W' || hdr.id[1] != 'A' ||
	   hdr.id[2] != 'D' || (hdr.id[3] != '2' && hdr.id[3] != '3')) {
		fprintf(stderr, "wad_open: %s bad magic (%c%c%c%c)\n",
			filename, hdr.id[0], hdr.id[1], hdr.id[2], hdr.id[3]);
		return false;
	}

	numdirs = hdr.numlumps;

	cache = new miptex_t *[numdirs];
	if(cache == nullptr) {
		return false;
	}

	memset(cache, 0, sizeof(miptex_t *) * numdirs);
	
	dirs = new lumpinfo_t[numdirs];
	if(dirs == nullptr) {
		return false;
	}

	if(fseek(fp, hdr.infotableofs, SEEK_SET) != 0) {
		fprintf(stderr, "wad_open %s: bad offset/seek %d\n",
			filename, hdr.infotableofs);
		return false;
	};

	size_t infotablebytes = sizeof(lumpinfo_t) * numdirs;
	if(fread(dirs, 1, infotablebytes, fp) != infotablebytes) {
		fprintf(stderr, "wad_open %s: bad read\n", filename);
		return false;
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

miptex_t *wad_t::getmiptex(const char *name) const
{
	lumpinfo_t *dir = nullptr;
	for(int32_t i = 0; i < numdirs; i++) {
		if(strncaseeq(name, dirs[i].name, MAXTEXTURENAME)) {
			dir = &dirs[i];
			if(cache[i]) {
				return cache[i];
			}
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

	miptex_t *mt = (miptex_t *)new int8_t[dir->size];
	if(mt == nullptr) {
		return nullptr;
	}

	fseek(fp, dir->filepos, SEEK_SET);
	fread(mt, 1, dir->size, fp);

	if(!strncaseeq(dir->name, mt->name, MAXTEXTURENAME)) {
		fprintf(stderr,
			"warning: miptex name does not match directory name.\n"
			"\texpected: %s, got %s\n", dir->name, mt->name);
	}
	return mt;
}

wad_t::~wad_t()
{
	if(fp != nullptr) {
		fclose(fp);
	}
	if(cache != nullptr) {
		for(int32_t i = 0; i < numdirs; i++) {
			delete[] cache[i];
		}
	}
	delete[] cache;
	delete[] dirs;
}
