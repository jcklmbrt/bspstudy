#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wad.h"

wad_t *wad_open(const char *filename)
{
	wad_t *wad = malloc(sizeof(wad_t));
	
	if(wad == NULL) {
		goto bad;
	}

	memset(wad, 0, sizeof(wad_t));

	wad->fp = fopen(filename, "rb");
	if(wad->fp == NULL) {
		goto bad;
	}

	wadinfo_t hdr;
	fseek(wad->fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(wadinfo_t), wad->fp);

	if(hdr.id[0] != 'W' || hdr.id[1] != 'A' ||
	   hdr.id[2] != 'D' || (hdr.id[3] != '2' && hdr.id[3] != '3')) {
		fprintf(stderr, "wad_open: %s bad magic (%c%c%c%c)\n",
			filename, hdr.id[0], hdr.id[1], hdr.id[2], hdr.id[3]);
		goto bad;
	}

	wad->numdirs = hdr.numlumps;
	wad->cache = calloc(wad->numdirs, sizeof(miptex_t *));
	if(wad->cache == NULL) {
		goto bad;
	}
	
	wad->dirs = malloc(sizeof(lumpinfo_t) * wad->numdirs);
	if(wad->dirs == NULL) {
		fprintf(stderr, "wad_open %s: bad alloc", filename);
		goto bad;
	}

	if(fseek(wad->fp, hdr.infotableofs, SEEK_SET) != 0) {
		fprintf(stderr, "wad_open %s: bad offset/seek %d\n",
			filename, hdr.infotableofs);
		goto bad;
	};

	size_t infotablebytes = sizeof(lumpinfo_t) * wad->numdirs;
	if(fread(wad->dirs, 1, infotablebytes, wad->fp) != infotablebytes) {
		fprintf(stderr, "wad_open %s: bad read\n", filename);
		goto bad;
	}
	
	return wad;
bad:
	wad_close(wad);
	return NULL;
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

miptex_t *wad_getmiptex(const wad_t *wad, const char *name)
{
	lumpinfo_t *dir = NULL;
	for(int32_t i = 0; i < wad->numdirs; i++) {
		if(strncaseeq(name, wad->dirs[i].name, MAXTEXTURENAME)) {
			dir = &wad->dirs[i];
			if(wad->cache[i]) {
				return wad->cache[i];
			}
		}
	}

	if(dir == NULL) {
		return NULL;
	}

	// TODO: support compression
	if(dir->compression != CMP_NONE) {
		fprintf(stderr, "texture %s is compressed, bspstudy does not support compression", name);
		return NULL;
	}

	if(dir->type != TYP_MIPTEX) {
		fprintf(stderr, "texture %s is not in miptex format\n", name);
		return NULL;
	}

	miptex_t *mt = malloc(dir->size);
	if(mt == NULL) {
		return NULL;
	}

	fseek(wad->fp, dir->filepos, SEEK_SET);
	fread(mt, 1, dir->size, wad->fp);

	if(!strncaseeq(dir->name, mt->name, MAXTEXTURENAME)) {
		fprintf(stderr,
			"warning: miptex name does not match directory name.\n"
			"\texpected: %s, got %s\n", dir->name, mt->name);
	}
	return mt;
}

void wad_close(wad_t *wad)
{
	if(wad == NULL) {
		return;
	}
	if(wad->fp != NULL) {
		fclose(wad->fp);
	}
	if(wad->cache != NULL) {
		for(int32_t i = 0; i < wad->numdirs; i++) {
			free(wad->cache[i]);
		}
	}
	free(wad->cache);
	free(wad->dirs);
	free(wad);
}
