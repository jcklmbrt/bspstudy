
#ifndef _ENTITY_H
#define _ENTITY_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	int32_t size;
	char  *epairs; /*
	epairs[0] = key length                    (max size 32 < 127)
	epairs[1] << 8 | epairs[2] = value length (max size 1024 < 32767)
	epairs[3...] = data */
} entity_t;

const char *entputs(const entity_t *e);
const char *entgets(const entity_t *e, const char *key);
const char *entgetf(const entity_t *e, const char *key, float *out);
const char *entgetv3(const entity_t *e, const char *key, float out[3]);

int entparse(const char *entdata, int32_t entdatasize, entity_t *entities);

#endif
