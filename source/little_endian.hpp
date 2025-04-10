
#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN

#include <cstdint>
#include <glm/glm.hpp>

namespace little_endian {
	inline uint32_t readu32(const uint8_t in[]) { return in[0] + (in[1] << 8) + (in[2] << 16) + (in[3] << 24); }
	inline uint16_t readu16(const uint8_t in[]) { return in[0] + (in[1] << 8); }
	inline int32_t  readi32(const uint8_t in[]) { union { uint32_t u32; int32_t i32; } r; r.u32 = readu32(in); return r.i32; }
	inline int16_t  readi16(const uint8_t in[]) { union { uint16_t u16; int16_t i16; } r; r.u16 = readu16(in); return r.i16; }
	inline float    readf32(const uint8_t in[]) { union { uint32_t u32; float   f32; } r; r.u32 = readu32(in); return r.f32; }

	inline void read(const uint8_t in[], size_t &ofs, uint32_t &out) { out = readu32(&in[ofs]); ofs += sizeof(uint32_t); }
	inline void read(const uint8_t in[], size_t &ofs, int32_t  &out) { out = readi32(&in[ofs]); ofs += sizeof(int32_t); }
	inline void read(const uint8_t in[], size_t &ofs, float    &out) { out = readf32(&in[ofs]); ofs += sizeof(float); }
	inline void read(const uint8_t in[], size_t &ofs, uint16_t &out) { out = readu16(&in[ofs]); ofs += sizeof(int16_t); }
	inline void read(const uint8_t in[], size_t &ofs, int16_t  &out) { out = readi16(&in[ofs]); ofs += sizeof(uint16_t); }

	inline void read(const uint8_t in[], size_t &ofs, glm::vec3 &out) {
		read(in, ofs, out.x);
		read(in, ofs, out.y);
		read(in, ofs, out.z);
	}
};

#endif
