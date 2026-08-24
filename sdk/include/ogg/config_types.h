#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

/* Autotools-generated on every other platform (from config_types.h.in via
 * ./configure) - this tree vendors libogg without running autotools, and
 * the generic Unix branch of os_types.h expects this header to exist.
 * Straightforward for any LP64/ILP32 target with <stdint.h>. */
#include <stdint.h>

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;
typedef uint64_t ogg_uint64_t;

#endif
