#pragma once

#include <intrin.h>
#include <stdint.h>

#define __LITTLE_ENDIAN 3412
#define __BYTE_ORDER 3412

#define le16toh(x) x
#define le32toh(x) x
#define le64toh(x) x

static inline uint16_t be16toh(uint16_t x) { return _byteswap_ushort(x); }
static inline uint32_t be32toh(uint32_t x) { return _byteswap_ulong(x); }
static inline uint64_t be64toh(uint64_t x) { return _byteswap_uint64(x); }

static inline uint16_t htobe16(uint16_t x) { return _byteswap_ushort(x); }
static inline uint32_t htobe32(uint32_t x) { return _byteswap_ulong(x); }
static inline uint64_t htobe64(uint64_t x) { return _byteswap_uint64(x); }

