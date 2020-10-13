#pragma once

#include <cstdint>

#ifdef _MSC_VER
#define __attribute__(x)
#pragma pack(push)
#pragma pack(1)
#endif

struct MBR_CHS
{
	uint8_t head;
	uint8_t cylh_sector;
	uint8_t cylinder;
} __attribute__((packed));

struct MBR_Entry
{
	uint8_t status;
	MBR_CHS chs_start;
	uint8_t type;
	MBR_CHS chs_end;
	uint32_t lba_start;
	uint32_t lba_size;
} __attribute__((packed));

struct MasterBootRecord
{
	uint8_t u1[0x1BE];
	MBR_Entry partition[4];
	uint8_t signature[2];
} __attribute__((packed));

#ifdef _MSC_VER
#pragma pack(pop)
#undef __attribute__
#endif
