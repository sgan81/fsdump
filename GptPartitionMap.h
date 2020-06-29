/*
	This file is part of fsdump, a tool for dumping drives into image files.
	Copyright (C) 2020 Simon Gander.

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
#include <vector>

#include "Crc32.h"

class Device;

struct PMAP_GptHeader;
struct PMAP_Entry;

class GptPartitionMap
{
public:
	typedef uint8_t PM_GUID[0x10];

	struct PMAP_GptHeader
	{
		uint64_t Signature;
		uint32_t Revision;
		uint32_t HeaderSize;
		uint32_t HeaderCRC32;
		uint32_t Reserved;
		uint64_t MyLBA;
		uint64_t AlternateLBA;
		uint64_t FirstUsableLBA;
		uint64_t LastUsableLBA;
		PM_GUID  DiskGUID;
		uint64_t PartitionEntryLBA;
		uint32_t NumberOfPartitionEntries;
		uint32_t SizeOfPartitionEntry;
		uint32_t PartitionEntryArrayCRC32;
		uint32_t _pad;
	} __attribute__((packed, aligned(8)));

	struct PMAP_Entry
	{
		PM_GUID  PartitionTypeGUID;
		PM_GUID  UniquePartitionGUID;
		uint64_t StartingLBA;
		uint64_t EndingLBA;
		uint64_t Attributes;
		uint16_t PartitionName[36];
	};

	GptPartitionMap();

	bool LoadAndVerify(Device &dev);

	int FindFirstAPFSPartition();
	bool GetPartitionOffsetAndSize(int partnum, uint64_t &offset, uint64_t &size);
	bool GetPartitionEntry(int partnum, PMAP_Entry &pe);

	void ListEntries();

	int CopyGPT(Device &src, Device &dst);

private:
	Crc32 m_crc;

	std::vector<uint8_t> m_hdr_data;
	std::vector<uint8_t> m_entry_data;

	const PMAP_GptHeader *m_hdr;
	const PMAP_Entry *m_map;
	unsigned int m_sector_size;

public:
	static const PM_GUID PTYPE_EFI_SYS;
	static const PM_GUID PTYPE_APFS;
};
