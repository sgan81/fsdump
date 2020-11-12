#pragma once

#ifdef _MSC_VER

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

#endif

#ifdef __APPLE__
// Definitions for macOS
#include <libkern/OSByteOrder.h>
#define bswap_16(x) _OSSwapInt16(x)
#define bswap_32(x) _OSSwapInt32(x)
#define bswap_64(x) _OSSwapInt64(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define htole64(x) OSSwapHostToLittleInt64(x)

#define __LITTLE_ENDIAN 0x3412
#define __BIG_ENDIAN 0x1234
#define __BYTE_ORDER __LITTLE_ENDIAN

#endif
