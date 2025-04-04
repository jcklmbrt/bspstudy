
#ifndef _WADLIB_H
#define _WADLIB_H

#include <vector>

#include <stdio.h>
#include <stdint.h>

enum {
	CMP_NONE = 0,
	CMP_LZSS = 1,
	
	TYP_NONE = 0,
	TYP_MIPTEX = 0x43
};

struct dmiptexlump_t {
	int32_t nummiptex;
	int32_t dataofs[4]; /* flexible array member */
};

struct wad_t;

constexpr int MAXTEXTURENAME = 16;
constexpr int MIPLEVELS = 4;
struct miptex_t {
	char name[MAXTEXTURENAME];
	uint32_t width;
	uint32_t height;
	uint32_t offsets[MIPLEVELS];

	bool load(const uint8_t *bytes);
	bool load(const wad_t &wad, const char *name);
	std::vector<uint8_t> rgba;
};


struct wadinfo_t {
	uint8_t id[4];
	int32_t numlumps;
	int32_t infotableofs;
};

struct lumpinfo_t {
	int32_t filepos;
	int32_t disksize;
	int32_t size;
	uint8_t type;
	uint8_t compression;
	uint16_t padding;
	char name[MAXTEXTURENAME];
};

struct wad_t {
	bool open(const char *filename);
	~wad_t();
	const uint8_t *getmiptexbytes(const char *name) const;

	FILE *fp = nullptr;
	lumpinfo_t *dirs = nullptr;
	uint8_t **cache = nullptr;
	int32_t numdirs;
};

#endif

