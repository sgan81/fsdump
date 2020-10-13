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

#ifdef _MSC_VER
#define __attribute__(x)
#pragma pack(push)
#pragma pack(1)
#endif

constexpr auto MASTER_FILE_TABLE_NUMBER = 0;
constexpr auto MASTER_FILE_TABLE2_NUMBER = 1;
constexpr auto BIT_MAP_FILE_NUMBER = 6;

struct MULTI_SECTOR_HEADER {
	uint32_t Magic;
	uint16_t UsaOff;
	uint16_t UsaLen;
};

struct MFT_SEGMENT_REFERENCE {
	uint32_t SegmentNumberLowPart;
	uint16_t SegmentNumberHighPart;
	uint16_t SequenceNumber;
};
typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE;

struct BIOS_PARAMETER_BLOCK {
	uint16_t BytesPerSector;
	uint8_t SectorsPerCluster;
	uint16_t ReservedSectors;
	uint8_t Fats;
	uint16_t RootEntries;
	uint16_t Sectors;
	uint8_t Media;
	uint16_t SectorsPerFat;
	uint16_t SectorsPerTrack;
	uint16_t Heads;
	uint32_t HiddenSectors;
	uint32_t LargeSectors;
} __attribute__((packed));

struct BOOT_SECTOR {
	uint8_t Jump[3];
	uint64_t Oem;
	BIOS_PARAMETER_BLOCK Bpb;
	uint8_t Unused[4];
	uint64_t NumberSectors;
	uint64_t MftStartLcn;
	uint64_t Mft2StartLcn;
	uint8_t ClustersPerFileRecordSegment;
	uint8_t Reserved0[3];
	uint8_t DefaultClustersPerIndexAllocationBuffer;
	uint8_t Reserved1[3];
	uint64_t SerialNumber;
	uint32_t Checksum;
	uint8_t BootStrap[428];
} __attribute__((packed));

struct FILE_RECORD_SEGMENT_HEADER {
	MULTI_SECTOR_HEADER MultiSectorHeader;
	uint64_t Lsn;
	uint16_t SequenceNumber;
	uint16_t ReferenceCount;
	uint16_t FirstAttributeOffset;
	uint16_t Flags;
	uint32_t FirstFreeByte;
	uint32_t BytesAvailable;
	uint64_t BaseFileRecordSegment; // FILE_REFERENCE
	uint16_t NextAttributeInstance;
	uint16_t UpdateArrayForCreateOnly; // UPDATE_SEQUENCE_ARRAY
} __attribute__((packed));

struct ATTRIBUTE_RECORD_HEADER {
	uint32_t TypeCode;
	uint32_t RecordLength;
	uint8_t FormCode;
	uint8_t NameLength;
	uint16_t NameOffset;
	uint16_t Flags;
	uint16_t Instance;
	union {
		struct {
			uint32_t ValueLength;
			uint16_t ValueOffset;
			uint8_t ResidentFlags;
			uint8_t Reserved;
		} Resident;
		struct {
			uint64_t LowestVcn;
			uint64_t HighestVcn;
			uint16_t MappingPairsOffset;
			uint8_t CompressionUnit;
			uint8_t Reserved[5];
			uint64_t AllocatedLength;
			uint64_t FileSize;
			uint64_t ValidDataLength;
			uint64_t TotalAllocated;
		} Nonresident;
	} Form;
};

static constexpr auto RESIDENT_FORM = 0;
static constexpr auto NONRESIDENT_FORM = 1;

struct DUPLICATED_INFORMATION {
	uint64_t CreationTime;
	uint64_t LastModificationTime;
	uint64_t LastChangeTime;
	uint64_t LastAccessTime;
	uint64_t AllocatedLength;
	uint64_t FileSize;
	uint32_t FileAttributes;
	uint16_t PackedEaSize;
	uint16_t Reserved;
};

struct FILE_NAME {
	FILE_REFERENCE ParentDirectory;
	DUPLICATED_INFORMATION Info;
	uint8_t FileNameLength;
	uint8_t Flags;
	uint16_t FileName[];
};

struct INDEX_HEADER {
	uint32_t FirstIndexEntry;
	uint32_t FirstFreeByte;
	uint32_t BytesAvailable;
	uint8_t Flags;
	uint8_t Reserved[3];
};

struct INDEX_ROOT {
	uint32_t IndexedAttributeType;
	uint32_t CollationRule;
	uint32_t BytesPerIndexBuffer;
	uint8_t BlocksPerIndexBuffer;
	uint8_t Reserved[3];
	INDEX_HEADER IndexHeader;
};

struct INDEX_ALLOCATION_BUFFER {
	uint64_t MultiSectorHeader;
	uint64_t Lsn;
	uint64_t ThisBlock; // VCN
	INDEX_HEADER IndexHeader;
	// UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;
};

struct INDEX_ENTRY {
	union {
		FILE_REFERENCE FileReference;
		struct {
			uint16_t DataOffset;
			uint16_t DataLength;
			uint32_t ReservedForZero;
		};
	};
	uint16_t Length;
	uint16_t AttributeLength;
	uint16_t Flags;
	uint16_t Reserved;
};

#ifdef _MSC_VER
#pragma pack(pop)
#undef __attribute__
#endif
