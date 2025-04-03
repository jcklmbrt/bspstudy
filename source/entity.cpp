#include <stdio.h>
#include <string.h>

#include "bsp.hpp"
#include "entity.hpp"

static unsigned char getkeylen(char *ep) { return ep[0]; }
static unsigned short getvallen(char *ep) { return ep[1] << 8 | ep[2]; }
static char *getkey(char *ep)  { return ep + 3; }
static char *getval(char *ep)  { return ep + 3 + getkeylen(ep); }
static char *getnext(char *ep) { return getval(ep) + getvallen(ep); }


const char *entity_t::puts() const
{
	const char *classname = nullptr;
	static char lines[EPAIR_MAX_VALUE][128];
	int i = 0;
	for(char *ep = m_epairs; ep - m_epairs < m_size; ep = getnext(ep)) {
		// assuming 1st key will be classname
		if(!strncmp(getkey(ep), "classname", getkeylen(ep))) {
			classname = getval(ep);
		} else {
			snprintf(lines[i++], EPAIR_MAX_VALUE, "%s: %s\n", getkey(ep), getval(ep));
		}
	}

	if(classname != nullptr) {
		printf("%s:\n", classname);
		while(i--) {
			printf("\t%s", lines[i]);
		}
	}

	return nullptr;
}


const char *entity_t::get(const char *key) const
{
	for(char *ep = m_epairs; ep - m_epairs < m_size; ep = getnext(ep)) {
		if(!strncmp(getkey(ep), key, getkeylen(ep))) {
			return getval(ep);
		}
	}
	return nullptr;
}


const char *entity_t::get(const char *key, float &out) const
{
	const char *value = get(key);
	if(value != nullptr) {
		out = atof(value);
	}
	return value
;
}


const char *entity_t::get(const char *key, glm::vec3 &out) const
{
	const char *value = get(key);
	if(value != nullptr) {
		if(sscanf(value, "%f %f %f", &out[0], &out[1], &out[2]) != 3) {
			return nullptr;
		};
	}
	return value;
}


enum tokid {
	ENTLEX_END   = 0,
	ENTLEX_IDENT = 1,
	ENTLEX_LBRACE = '{',
	ENTLEX_RBRACE = '}',
	ENTLEX_QUOTE  = '\"'
};


static int elex(const char **p_start, const char *end, char token[EPAIR_MAX_VALUE])
{
	const char *start = *p_start;
	char       *ptok  = token;

	while(start != end) {
		switch(*start) {
			/* symbols */
		case ENTLEX_LBRACE:
		case ENTLEX_RBRACE:
			*p_start = start + 1;
			return *start;
		case ENTLEX_QUOTE:
			start++;
			while(*start != '\"') {
				*ptok++ = *start++;
				if(ptok > &token[EPAIR_MAX_VALUE]) {
					return ENTLEX_END;
				}
			}
			*ptok = '\0';
			*p_start = start + 1;
			return ENTLEX_IDENT;
		/* skip whitespace*/
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case  ' ':
			start++;
			break;
		case '/':
			/* comments */
			start++;
			if(*start != '/') {
				break;
			}
			/* fallthrough */
		case '#':
		case ';':
			while(*start != '\n') {
				start++;
			}
			break;
			/* EOF */
		case '\0':
		case   -1:
			return ENTLEX_END;
		default:
			fprintf(stderr, "entlex: unexpected char: %c\n", *start);
			return ENTLEX_END;
		}
	}
	
	return ENTLEX_END;
}


int entity_t::parse(const char *entdata, int32_t entdatasize, entity_t *entities)
{
	/* based upon the sample bsp file's entity data,
	   let's construct a formal grammar.
	<entitylist> -> <entitylist>\n?<entity>
	<entitylist> -> <entity>
	<entity>     -> {<epairlist>}
	<epairlist>  -> <epairlist>\n?<epair>
	<epairlist>  -> <epair>
	<epair>      -> <quote> <quote>
	<quote>      -> "<value>"
	<value>      -> .*
	*/

	/* ...or you can just use a regular expression */
	/* {\s(".*" ".*"\s)+} */
	char token[EPAIR_MAX_VALUE];
	const char *end = entdata + entdatasize;
	int numentities = 0;
	int m_epairsize   = 0;
	int old_m_epairsize;
	int tokid;

	char key[EPAIR_MAX_KEY];
	char value[EPAIR_MAX_VALUE];

	unsigned char  keylen = 0;
	unsigned short vallen = 0;

	for(;;) {
		tokid = elex(&entdata, end, token);

		if(tokid == ENTLEX_END) {
			break;
		} else if(tokid != ENTLEX_LBRACE) {
			fprintf(stderr, "entparse: expected \'{\'\n");
			break;
		}

		for(;;) {
			tokid = elex(&entdata, end, token);

			if(tokid == ENTLEX_RBRACE) {
				/* ensure entity has mandatory key "classname" */
				if(entities[numentities].get("classname") == nullptr) {
					fprintf(stderr, "entparse: bad entity, no classname\n");
				}
				if(numentities++ > MAX_MAP_ENTITIES) {
					fprintf(stderr, "entparse: too many entities\n");
					return numentities;
				}
				entities[numentities].m_epairs = nullptr;
				m_epairsize = 0;
				break;
			} else if(tokid != ENTLEX_IDENT) {
				fprintf(stderr, "entparse: %d expected identifier or }\n", tokid);
				return numentities;
			}

			keylen = snprintf(key, EPAIR_MAX_KEY, "%s", token) + 1;

			if(elex(&entdata, end, token) != ENTLEX_IDENT) {
				fprintf(stderr, "entparse: expected identifier\n");
				return numentities;
			}

			vallen = snprintf(value, EPAIR_MAX_VALUE, "%s", token) + 1;

			char *m_epairs = entities[numentities].m_epairs;

			old_m_epairsize = m_epairsize;
			m_epairsize += keylen + vallen + 3;

			if(m_epairs == nullptr) {
				m_epairs = (char *)malloc(m_epairsize);
			} else {
				m_epairs = (char *)realloc(m_epairs, m_epairsize);
			}

			if(m_epairs == nullptr) {
				fprintf(stderr, "entparse: bad realloc: m_epairsize: %d\n", m_epairsize);
				return numentities;
			}

			char *newepair = m_epairs + old_m_epairsize;

			newepair[0] = keylen;
			newepair[1] = (vallen >> 8) & 0xFF;
			newepair[2] = (vallen >> 0) & 0xFF;

			memcpy(newepair + 3,          key,   keylen);
			memcpy(newepair + 3 + keylen, value, vallen);

			entities[numentities].m_epairs = m_epairs;
			entities[numentities].m_size   = m_epairsize;
		}
	}

	return numentities;
}
