/*
	This file is part of fsdump, a tool for dumping drives into image files.
	Copyright (C) 2022 Simon Gander.

	FSDump is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	FSDump is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with fsdump.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <cstdio>

#include <vector>

#include "Device.h"
#include "Crc32.h"
#include "ImageFile.h"

struct MS_GUID {
	bool operator==(const MS_GUID& o) const
	{
		return !memcmp(this, &o, sizeof(MS_GUID));
	}

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

struct VHDX_REGION_TABLE {
	VHDX_REGION_TABLE_HEADER hdr;
	VHDX_REGION_TABLE_ENTRY entries[2047];
};

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
	uint32_t LeadingBytes[2];
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
	uint32_t Reserved2;
};

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

struct VHDX_VIRTUAL_DISK_ID {
	MS_GUID VirtualDiskId;
};

struct VHDX_LOGICAL_SECTOR_SIZE {
	uint32_t LogicalSectorSize;
};

struct VHDX_PHYSICAL_SECTOR_SIZE {
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
	int Open(const char* name, bool writable = false);
	void Close();

	int Read(void* data, size_t size, uint64_t offset) override;
	int Write(const void* data, size_t size, uint64_t offset) override;

	uint64_t GetSize() const override;

private:
	int ReadHeader(VHDX_HEADER& header, uint64_t offset);
	int WriteHeader(const VHDX_HEADER& header, uint64_t offset);
	const VHDX_HEADER& CurrentHeader() const { return m_head[m_active_header]; }
	int UpdateHeader(const VHDX_HEADER& header);
	int ReadRegionTable(VHDX_REGION_TABLE& table, uint64_t offset);
	int SetupRegionTable(VHDX_REGION_TABLE& table);
	int WriteRegionTable(const VHDX_REGION_TABLE& table, uint64_t offset);
	int ReadMetadata(uint64_t offset, uint32_t length);
	void AddMetaEntry(uint32_t& off, VHDX_METADATA_TABLE_ENTRY& entry, const MS_GUID& guid, const void* data, size_t size, uint32_t flags);
	int WriteMetadata();
	int ReadBAT(uint64_t offset, uint32_t length);
	int WriteBAT();
	int UpdateBAT(int index, uint64_t entry);

	int LogReplay();
	int LogStart();
	int LogWrite(uint64_t offset, const void* block_4k);
	int LogWriteZero(uint64_t offset, uint64_t length);
	int LogCommit();
	int LogComplete();

	void GenerateRandomGUID(MS_GUID& guid);
	void UpdateFileWriteGUID();
	void UpdateDataWriteGUID();

	// Layout:
	// File identifier
	// Header 1, 2
	// Region Table 1, 2
	// Rest reserved

	VHDX_FILE_IDENTIFIER m_ident;
	VHDX_HEADER m_head[2];
	VHDX_REGION_TABLE m_regi[2];

	Crc32 m_crc;
	ImageFile m_file;
	bool m_writable;
	int8_t m_active_header;
	bool m_file_guid_changed;
	bool m_data_guid_changed;

	uint8_t m_cache[0x10000];
	std::vector<uint8_t> m_log_b;
	std::vector<uint64_t> m_bat_b;
	std::vector<uint8_t> m_meta_b;

	uint32_t m_log_head;
	uint32_t m_log_tail;
	uint64_t m_log_seqno;
	VHDX_LOG_ENTRY_HEADER m_leh;

	uint64_t m_bat_offset;
	uint32_t m_bat_size;
	uint32_t m_bat_flags;
	uint64_t m_meta_offset;
	uint32_t m_meta_size;
	uint32_t m_meta_flags;

	uint32_t m_block_size;
	bool m_leave_alloc;
	bool m_has_parent;
	uint64_t m_disk_size;
	uint32_t m_sector_size_logical;
	uint32_t m_sector_size_physical;
	MS_GUID m_virtual_disk_id;

	uint64_t m_chunk_ratio;
	uint64_t m_data_blocks_count;
	uint64_t m_sector_bitmap_blocks_count;
	uint64_t m_bat_entries_cnt;
};
