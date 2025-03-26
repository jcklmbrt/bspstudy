#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wad.h"

struct wad {
	FILE *fp;
	struct wad_dir *dirs;
	int32_t num_dirs;
};

struct wad *wad_open(const char *filename)
{
	struct wad *wad = malloc(sizeof(struct wad));
	
	if(wad == NULL) {
		goto bad;
	}

	memset(wad, 0, sizeof(struct wad));

	wad->fp = fopen(filename, "rb");
	if(wad->fp == NULL) {
		goto bad;
	}

	struct wad_hdr hdr;
	fseek(wad->fp, 0, SEEK_SET);
	fread(&hdr, 1, sizeof(struct wad_hdr), wad->fp);

	if(hdr.id[0] != 'W' || hdr.id[1] != 'A' ||
	   hdr.id[2] != 'D' || (hdr.id[3] != '2' && hdr.id[3] != '3')) {
		fprintf(stderr, "wad_open: %s bad magic (%c%c%c%c)\n",
			filename, hdr.id[0], hdr.id[1], hdr.id[2], hdr.id[3]);
		goto bad;
	}

	wad->num_dirs = hdr.numlumps;
	
	wad->dirs = malloc(sizeof(struct wad_dir) * wad->num_dirs);
	if(wad->dirs == NULL) {
		goto bad;
	}

	fseek(wad->fp, hdr.info_ofs, SEEK_SET);
	fread(wad->dirs, 1, sizeof(struct wad_dir) * wad->num_dirs, wad->fp);
	
	return wad;
bad:
	wad_close(wad);
	return NULL;
}

struct miptex *wad_getmiptex(struct wad *wad, const char *name)
{
	struct wad_dir *dir = NULL;
	for(int32_t i = 0; i < wad->num_dirs; i++) {
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

	struct miptex *mt = malloc(dir->size);
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

void wad_close(struct wad *wad)
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
