
#ifndef _ENTITY_H
#define _ENTITY_H

#include <stdint.h>
#include <stdlib.h>

struct entity {
	size_t size;
	char  *epairs; /*
	writing a "bump allocator" w/ strict aliasing.
	epairs[0] = key length                    (max size 32 < 127)
	epairs[1] << 8 | epairs[2] = value length (max size 1024 < 32767)
	epairs[3...] = data
	*/
};

const char *entputs(struct entity *e);
const char *entgets(struct entity *e, const char *key);
const char *entgetf(struct entity *e, const char *key, float *out);
const char *entgetv3(struct entity *e, const char *key, float out[3]);

int entparse(const char *entdata, int32_t entdatasize, struct entity *entities);

#endif
