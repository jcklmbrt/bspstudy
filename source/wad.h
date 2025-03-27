
#ifndef _WADLIB_H
#define _WADLIB_H

#include <stdio.h>
#include <stdint.h>

enum {
	CMP_NONE = 0,
	CMP_LZSS = 1,
	
	TYP_NONE = 0,
	TYP_MIPTEX = 0x43
};

typedef struct miptexhdr {
	int32_t nummiptex;
	int32_t dataofs[];
} dmiptexlump_t;

#define MAXTEXTURENAME 16
#define MIPLEVELS       4
typedef struct miptex_s {
	char name[MAXTEXTURENAME];
	uint32_t width;
	uint32_t height;
	uint32_t offsets[MIPLEVELS];
} miptex_t;

typedef struct {
	char identification[4];
	int32_t numlumps;
	int32_t infotableofs;
} wadinfo_t;

typedef struct {
	int32_t filepos;
	int32_t disksize;
	int32_t size;
	uint8_t type;
	uint8_t compression;
	uint16_t padding;
	char name[MAXTEXTURENAME];
} lumpinfo_t;

typedef struct {
	FILE *fp;
	lumpinfo_t *dirs;
	int32_t numdirs;
} wad_t;

wad_t *wad_open(const char *filename);
miptex_t *wad_getmiptex(wad_t *wad, const char *name);
void wad_close(wad_t *wad);

#endif

