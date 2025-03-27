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

	if(hdr.identification[0] != 'W' || hdr.identification[1] != 'A' ||
	   hdr.identification[2] != 'D' || (hdr.identification[3] != '2' && hdr.identification[3] != '3')) {
		fprintf(stderr, "wad_open: %s bad magic (%c%c%c%c)\n",
			filename, hdr.identification[0], hdr.identification[1], hdr.identification[2], hdr.identification[3]);
		goto bad;
	}

	wad->numdirs = hdr.numlumps;
	
	wad->dirs = malloc(sizeof(lumpinfo_t) * wad->numdirs);
	if(wad->dirs == NULL) {
		goto bad;
	}

	fseek(wad->fp, hdr.infotableofs, SEEK_SET);
	fread(wad->dirs, 1, sizeof(lumpinfo_t) * wad->numdirs, wad->fp);
	
	return wad;
bad:
	wad_close(wad);
	return NULL;
}

miptex_t *wad_getmiptex(wad_t *wad, const char *name)
{
	lumpinfo_t *dir = NULL;
	for(int32_t i = 0; i < wad->numdirs; i++) {
		if(!strncmp(name, wad->dirs[i].name, MAXTEXTURENAME)) {
			dir = &wad->dirs[i];
			break;
		}
	}

	if(dir == NULL) {
		return NULL;
	}

	// TODO: support compression
	if(dir->compression != CMP_NONE) {
		fprintf(stderr, "Texture %s is compressed, bspstudy does not support compression", name);
		return NULL;
	}

	if(dir->type != TYP_MIPTEX) {
		fprintf(stderr, "Texture %s is not in miptex format\n", name);
		return NULL;
	}

	miptex_t *mt = malloc(dir->size);
	if(mt == NULL) {
		return NULL;
	}

	fseek(wad->fp, dir->filepos, SEEK_SET);
	fread(mt, 1, dir->size, wad->fp);

	if(strncmp(dir->name, mt->name, MAXTEXTURENAME) != 0) {
		fprintf(stderr,
			"Warning: Miptex name does not match directory name.\n"
			"\tExpected: %s, got %s\n"
			, dir->name, mt->name);
	}
	
	return mt;
}

void wad_close(wad_t *wad)
{
	if(wad != NULL) {
		if(wad->fp != NULL) {
			fclose(wad->fp);
		}
		if(wad->dirs != NULL) {
			free(wad->dirs);
		}
		free(wad);
	}
}
