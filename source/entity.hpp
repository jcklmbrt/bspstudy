
#ifndef _ENTITY_H
#define _ENTITY_H

#include <cstdint>
#include <glm/glm.hpp>

struct entity_t {
	/*
	epairs[0] = key length                    (max size 32 < 127)
	epairs[1] << 8 | epairs[2] = value length (max size 1024 < 32767)
	epairs[3...] = data */
	const char *puts() const;
	const char *get(const char *key) const;
	const char *get(const char *key, float &out) const;
	const char *get(const char *key, glm::vec3 &out) const;

	static int parse(const char *entdata, int32_t entdatasize, entity_t *entities);
private:
	ssize_t m_size = 0;
	char *m_epairs = nullptr;
};

#endif
