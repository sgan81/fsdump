#pragma once

#include <cstdint>
#include <cstdio>

#include "Device.h"
#include "Crc32.h"

struct MS_GUID {
	uint8_t d[16];
};

struct VHDX_FILE_IDENTIFIER {
	uint64_t Signature; // 'vhdxfile'
	uint16_t Creator[256];
};

struct VHDX_HEADER {
	uint32_t Signature; // 'head'
	uint32_t Checksum;
	uint64_t SequenceNumber;
	MS_GUID FileWriteGuid;
	MS_GUID DataWriteGuid;
	MS_GUID LogGuid;
	uint16_t LogVersion;
	uint16_t Version;
	uint32_t LogLength;
	uint64_t LogOffset;
	uint8_t Reserved[4016];
};

struct VHDX_REGION_TABLE_HEADER {
	uint32_t Signature;
	uint32_t Checksum;
	uint32_t EntryCount;
	uint32_t Reserved;
};

struct VHDX_REGION_TABLE_ENTRY {
	MS_GUID Guid;
	uint64_t FileOffset;
	uint32_t Length;
	uint32_t Flags;
};
static constexpr uint32_t FLAG_REQUIRED = 0x00000001;

struct VHDX_REGION_TABLE {
	VHDX_REGION_TABLE_HEADER hdr;
	VHDX_REGION_TABLE_ENTRY entries[2047];
};

// static constexpr MS_GUID GUID_BAT = { 0x2DC27766, 0xF623, 0x4200, { 0x9D, 0x64, 0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08 }};
// static constexpr MS_GUID GUID_Metadata = { 0x8B7CA206, 0x4790, 0x4B9A, { 0xB8, 0xFE, 0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E }};

static constexpr MS_GUID GUID_BAT = { 0x66, 0x77, 0xC2, 0x2D, 0x23, 0xF6, 0x00, 0x42, 0x9D, 0x64, 0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08 };
static constexpr MS_GUID GUID_Metadata = { 0x06, 0xA2, 0x7C, 0x8B, 0x90, 0x47, 0x9A, 0x4B, 0xB8, 0xFE, 0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E };

struct VHDX_LOG_ENTRY_HEADER {
	uint32_t Signature; // 'loge'
	uint32_t Checksum;
	uint32_t EntryLength;
	uint32_t Tail;
	uint64_t SequenceNumber;
	uint32_t DescriptorCount;
	uint32_t Reserved;
	MS_GUID LogGuid;
	uint64_t FlushedFileOffset;
	uint64_t LastFileOffset;
};

struct VHDX_LOG_ZERO_DESCRIPTOR {
	uint32_t ZeroSignature; // 'zero'
	uint32_t Reserved;
	uint64_t ZeroLength;
	uint64_t FileOffset;
	uint64_t SequenceNumber;
};

struct VHDX_LOG_DATA_DESCRIPTOR {
	uint32_t DataSignature; // 'desc'
	uint32_t TrailingBytes;
	uint64_t LeadingBytes;
	uint64_t FileOffset;
	uint64_t SequenceNumber;
};

struct VHDX_LOG_DATA_SECTOR {
	uint32_t DataSignature; // 'data'
	uint32_t SequenceHigh;
	uint8_t Data[4084];
	uint32_t SequenceLow;
};

/*
struct VHDX_BAT_ENTRY {
	uint64_t State:3;
	uint64_t Reserved:17;
	uint64_t FileOffsetMB:44;
};
*/

enum VHDX_BAT_STATE {
	PAYLOAD_BLOCK_NOT_PRESENT = 0,
	PAYLOAD_BLOCK_UNDEFINED = 1,
	PAYLOAD_BLOCK_ZERO = 2,
	PAYLOAD_BLOCK_UNMAPPED = 3,
	PAYLOAD_BLOCK_FULLY_PRESENT = 6,
	PAYLOAD_BLOCK_PARTIALLY_PRESENT = 7,
	SB_BLOCK_NOT_PRESENT = 0,
	SB_BLOCK_PRESENT = 6
};

struct VHDX_METADATA_TABLE_HEADER {
	uint64_t Signature;
	uint16_t Reserved;
	uint16_t EntryCount;
	uint32_t Reserved2[5];
};

struct VHDX_METADATA_TABLE_ENTRY {
	MS_GUID ItemId;
	uint32_t Offset;
	uint32_t Length;
	uint32_t Flags;
};

static constexpr uint32_t META_FLAGS_IS_USER = 1;
static constexpr uint32_t META_FLAGS_IS_VIRTUAL_DISK = 2;
static constexpr uint32_t META_FLAGS_IS_REQUIRED = 4;

/*
static constexpr MS_GUID GUID_FILE_PARAMETERS = { 0xCAA16737, 0xFA36, 0x4D34, { 0xB3, 0xB6, 0x33, 0xF0, 0xAA, 0x44, 0xE7, 0x6B }};
static constexpr MS_GUID GUID_VIRTUAL_DISK_SIZE = { 0x2FA54224, 0xCD1B, 0x4876, { 0xB2, 0x11, 0x5D, 0xBE, 0xD8, 0x3B, 0xF4, 0xB8 }};
static constexpr MS_GUID GUID_PAGE83_DATA = { 0xBECA12AB, 0xB2E6, 0x4523, { 0x93, 0xEF, 0xC3, 0x09, 0xE0, 0x00, 0xC7, 0x46 }};
static constexpr MS_GUID GUID_LOGICAL_SECTOR_SIZE = { 0x8141BF1D, 0xA96F, 0x4709, { 0xBA, 0x47, 0xF2, 0x33, 0xA8, 0xFA, 0xAB, 0x5F }};
static constexpr MS_GUID GUID_PHYSICAL_SECTOR_SIZE = { 0xCDA348C7, 0x445D, 0x4471, { 0x9C, 0xC9, 0xE9, 0x88, 0x52, 0x51, 0xC5, 0x56 }};
static constexpr MS_GUID GUID_PARENT_LOCATOR = { 0xA8D35F2D, 0xB30B, 0x454D, { 0xAB, 0xF7, 0xD3, 0xD8, 0x48, 0x34, 0xAB, 0x0C }};
*/

struct VHDX_FILE_PARAMETERS {
	uint32_t BlockSize;
	uint32_t Flags;
};

static constexpr uint32_t FILE_PARAM_FLAG_LEAVE_BLOCKS_ALLOCATED = 1;
static constexpr uint32_t FILE_PARAM_FLAG_HAS_PARENT = 2;

struct VHDX_VIRTUAL_DISK_SIZE {
	uint64_t VirtualDiskSize;
};

struct VHDX_PAGE83_DATA {
	MS_GUID Page83Data;
};

struct VHDX_VIRTUAL_DISK_LOGICAL_SECTOR_SIZE {
	uint32_t LogicalSectorSize;
};

struct VHDX_VIRTUAL_DISK_PHYSICAL_SECTOR_SIZE {
	uint32_t PhysicalSectorSize;
};

struct VHDX_PARENT_LOCATOR_HEADER {
	MS_GUID LocatorType;
	uint16_t Reserved;
	uint16_t KeyValueCount;
};

struct VHDX_PARENT_LOCATOR_ENTRY {
	uint32_t KeyOffset;
	uint32_t ValueOffset;
	uint16_t KeyLength;
	uint16_t ValueLength;
};

class VhdxDevice : public Device
{
public:
	VhdxDevice();
	~VhdxDevice();

	int Create(const char* name, uint64_t size);
	int Open(const char* name, bool writable);
	void Close();

	int Read(void* data, size_t size, uint64_t offset) override;
	int Write(const void* data, size_t size, uint64_t offset) override;

	uint64_t GetSize() const override;

private:
	int ReadHeader(VHDX_HEADER& header, uint64_t offset);
	int WriteHeader(const VHDX_HEADER& header, uint64_t offset);
	int ReadRegionTable(VHDX_REGION_TABLE& table, uint64_t offset);
	int WriteRegionTable(const VHDX_REGION_TABLE& table, uint64_t offset);

	void GenerateRandomGUID(MS_GUID& guid);

	int ImgRead(uint64_t offset, void* buffer, size_t size);
	int ImgWrite(uint64_t offset, const void* buffer, size_t size);

	// Layout:
	// File identifier
	// Header 1, 2
	// Region Table 1, 2
	// Rest reserved

	VHDX_FILE_IDENTIFIER m_ident;
	VHDX_HEADER m_head[2];
	VHDX_REGION_TABLE_HEADER m_regi[2];

	Crc32 m_crc;
	FILE* m_file;
	bool m_writable;

	uint8_t m_cache[0x10000];
};
