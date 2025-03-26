
#ifndef _WADLIB_H
#define _WADLIB_H

#include <stdint.h>

struct miptexhdr {
	int32_t nummiptex;
	int32_t dataofs[];
};

#define MAXTEXTURENAME 16
#define MIPLEVELS       4
struct miptex {
	char name[MAXTEXTURENAME];
	uint32_t width;
	uint32_t height;
	uint32_t offsets[MIPLEVELS];
};

enum wad_typ {
	CMP_NONE = 0,
	CMP_LZSS = 1,

	TYP_NONE = 0,
	TYP_MIPTEX = 0x43
};

struct wad_hdr {
	char id[4];
	int32_t numlumps;
	int32_t info_ofs;
};

struct wad_dir {
	int32_t filepos;
	int32_t disksize;
	int32_t size;
	uint8_t type;
	uint8_t compression;
	uint16_t padding;
	char name[MAXTEXTURENAME];
};

struct wad *wad_open(const char *filename);
struct miptex *wad_getmiptex(struct wad *wad, const char *name);
void wad_close(struct wad *wad);

#endif

