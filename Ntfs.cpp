#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <endian.h>

#include "Ntfs.h"
#include "Device.h"
#include "ntfs_layout.h"

#undef DBG_VERBOSE
#define DBG_SIMPLE

enum TypeCodeEnum {
	TC_UNUSED = 0,
	TC_STANDARD_INFORMATION = 0x10,
	TC_ATTRIBUTE_LIST = 0x20,
	TC_FILE_NAME = 0x30,
	TC_OBJECT_ID = 0x40,
	TC_SECURITY_DESCRIPTOR = 0x50,
	TC_VOLUME_NAME = 0x60,
	TC_VOLUME_INFORMATION = 0x70,
	TC_DATA = 0x80,
	TC_INDEX_ROOT = 0x90,
	TC_INDEX_ALLOCATION = 0xA0,
	TC_BITMAP = 0xB0,
	TC_SYMBOLIC_LINK = 0xC0,
	TC_EA_INFORMATION = 0xD0,
	TC_EA = 0xE0,
	TC_PROPERTY_SET = 0xF0,
	TC_FIRST_USER_DEFINED_ATTRIBUTE = 0x100,
	TC_END = 0xFFFFFFFF
};

void DumpHex(const void *vdata, uint32_t size);

static const char *attr_str(uint32_t type_code)
{
	const char *ret;

	switch (type_code) {
	case TC_UNUSED: ret = "$UNUSED"; break;
	case TC_STANDARD_INFORMATION: ret = "$STANDARD_INFORMATION"; break;
	case TC_ATTRIBUTE_LIST: ret = "$ATTRIBUTE_LIST"; break;
	case TC_FILE_NAME: ret = "$FILE_NAME"; break;
	case TC_OBJECT_ID: ret = "$OBJECT_ID"; break;
	case TC_SECURITY_DESCRIPTOR: ret = "$SECURITY_DESCRIPTOR"; break;
	case TC_VOLUME_NAME: ret = "$VOLUME_NAME"; break;
	case TC_VOLUME_INFORMATION: ret = "$VOLUME_INFORMATION"; break;
	case TC_DATA: ret = "$DATA"; break;
	case TC_INDEX_ROOT: ret = "$INDEX_ROOT"; break;
	case TC_INDEX_ALLOCATION: ret = "$INDEX_ALLOCATION"; break;
	case TC_BITMAP: ret = "$BITMAP"; break;
	case TC_SYMBOLIC_LINK: ret = "$SYMBOLIC_LINK"; break;
	case TC_EA_INFORMATION: ret = "$EA_INFORMATION"; break;
	case TC_EA: ret = "$EA"; break;
	case TC_PROPERTY_SET: ret = "$PROPERTY_SET"; break;
	case TC_FIRST_USER_DEFINED_ATTRIBUTE: ret = "$FIRST_USER_DEFINED_ATTRIBUTE"; break;
	case TC_END: ret = "$END"; break;
	default: ret = "UNKNOWN"; break;
	}

	return ret;
}

static void print_u16(const void *pname, uint32_t len)
{
	uint32_t n;
	const uint16_t *name = reinterpret_cast<const uint16_t *>(pname);
	for (n = 0; n < len; n++) {
		printf("%c", name[n]);
	}
}

NtfsFile::NtfsFile(Ntfs& fs) : m_fs(fs)
{
	m_size = 0;
}

NtfsFile::~NtfsFile()
{
}

bool NtfsFile::Open(const uint8_t* mft_rec)
{
	uint32_t attr_off;
	const FILE_RECORD_SEGMENT_HEADER *frsh = reinterpret_cast<const FILE_RECORD_SEGMENT_HEADER *>(mft_rec);
	const ATTRIBUTE_RECORD_HEADER *atrh;
	int l;
	int v;
	int i;
	int n;
	int64_t dl;
	int64_t dv;
	uint64_t lcn;
	const uint8_t *lvp;
	uint8_t b;

	attr_off = frsh->FirstAttributeOffset;

	for (;;) {
		atrh = reinterpret_cast<const ATTRIBUTE_RECORD_HEADER *>(mft_rec + attr_off);
		if (atrh->TypeCode == TC_END)
			return false;
		if (atrh->TypeCode == TC_DATA)
			break;
		attr_off += atrh->RecordLength;
	}

	if (atrh->FormCode == RESIDENT_FORM) {
		return false;
	} else if (atrh->FormCode == NONRESIDENT_FORM) {
		m_size = atrh->Form.Nonresident.FileSize;
		lvp = mft_rec + attr_off + atrh->Form.Nonresident.MappingPairsOffset;

		m_exts.reserve(8);

		n = 0;
		lcn = 0;
		while (lvp[n] != 0) {
			v = lvp[n] & 0x0F;
			l = (lvp[n] & 0xF0) >> 4;
			++n;

			dv = 0;
			b = 0;
			for (i = 0; i < v; i++) {
				b = lvp[n + i];
				dv |= static_cast<uint64_t>(b) << (8 * i);
			}
			if (b & 0x80)
				dv |= 0xFFFFFFFFFFFFFFFFULL << (8 * i);
			n += v;

			dl = 0;
			b = 0;
			for (i = 0; i < l; i++) {
				b = lvp[n + i];
				dl |= static_cast<uint64_t>(b) << (8 * i);
			}
			if (b & 0x80)
				dl |= 0xFFFFFFFFFFFFFFFFULL << (8 * i);
			n += l;

			lcn += dl;

			m_exts.push_back(Extent(dv, lcn));
		}
	}

	return true;
}

void NtfsFile::Close()
{
	m_size = 0;
	m_exts.clear();
}

uint64_t NtfsFile::GetSize()
{
	return m_size;
}

void NtfsFile::Read(void* data, size_t size, uint64_t offset)
{
	// m_fs.m_srcdev.Read(m_fs.m_offset + ...);
}

Ntfs::Ntfs(Device& src, uint64_t offset) : m_srcdev(src), m_offset(offset)
{
}

Ntfs::~Ntfs()
{
}

uint64_t Ntfs::GetOccupiedSize() const
{
	return 0;
}

int Ntfs::CopyData(Device& dst)
{
	BOOT_SECTOR boot;

	m_srcdev.Read(&boot, sizeof(boot), m_offset);

	m_blksize = boot.Bpb.BytesPerSector * boot.Bpb.SectorsPerCluster;

	printf("BytesPerSector: %04X\n", boot.Bpb.BytesPerSector);
	printf("SecsPerCluster: %02X\n", boot.Bpb.SectorsPerCluster);
	printf("NumberSectors:  %" PRIX64 "\n", boot.NumberSectors);
	printf("MftStartLcn:    %" PRIX64 "\n", boot.MftStartLcn);
	printf("Mft2StartLcn:   %" PRIX64 "\n", boot.Mft2StartLcn);

	for (int n = 0; n < 0x40; n++) {
		ReadBlock(boot.MftStartLcn + n, m_blk);
#if 0
		DumpHex(m_blk, 0x1000);
		printf("\n");
#endif

		DumpMFTEntry(m_blk + 0x000, (n << 2));
		DumpMFTEntry(m_blk + 0x400, (n << 2) + 1);
		DumpMFTEntry(m_blk + 0x800, (n << 2) + 2);
		DumpMFTEntry(m_blk + 0xC00, (n << 2) + 3);
	}

	(void)dst;

	return ENOTSUP;
}

int Ntfs::ReadBlock(uint64_t lcn, void* data, size_t size)
{
	uint64_t off = (lcn * m_blksize) + m_offset;

	return m_srcdev.Read(data, size, off);
}

int Ntfs::ReadData(uint64_t pos, void* data, size_t size)
{
	return m_srcdev.Read(data, size, pos + m_offset);
}

bool Ntfs::FixUpdSeqRecord(uint8_t* out, const uint8_t* in, size_t size)
{
	const uint16_t *usa;
	uint16_t *rec_u16;
	int n;
	bool ok;

	memcpy(out, in, size);
	const FILE_RECORD_SEGMENT_HEADER *frsh = reinterpret_cast<const FILE_RECORD_SEGMENT_HEADER *>(out);
	if (frsh->MultiSectorHeader.Magic != 0x454C4946) return false;
	usa = reinterpret_cast<const uint16_t *>(out + frsh->MultiSectorHeader.UsaOff);
	rec_u16 = reinterpret_cast<uint16_t *>(out);
	ok = true;
	for (n = 1; n < frsh->MultiSectorHeader.UsaLen; n++) {
		if (rec_u16[(n << 8) - 1] != usa[0]) ok = false;
		rec_u16[(n << 8) - 1] = usa[n];
	}
	if (!ok)
		printf("Invalid update sequence record\n");
	return ok;
}

void Ntfs::DumpMFTEntry(const uint8_t* prec, uint64_t idx)
{
	uint16_t off;
	const uint16_t *usa;
	uint16_t *rec_u16;
	uint8_t record[0x400];
	const FILE_RECORD_SEGMENT_HEADER *frsh;
	const ATTRIBUTE_RECORD_HEADER *atrh;
	int n;

	FixUpdSeqRecord(record, prec, 0x400);

	frsh = reinterpret_cast<const FILE_RECORD_SEGMENT_HEADER *>(record);

	printf("============================================================================================================================================\n");
	printf("MFT Entry %" PRIX64 "\n", idx);
	printf("\n");
#if 0
	DumpHex(record, 0x400);
	printf("\n");
#endif

	off = frsh->FirstAttributeOffset;

#ifdef DBG_SIMPLE
	printf("%08X %04X %04X %" PRIX64 " %04X %04X %04X %04X %08X %08X %" PRIX64 " %04X %04X\n\n",
		frsh->MultiSectorHeader.Magic, frsh->MultiSectorHeader.UsaOff, frsh->MultiSectorHeader.UsaLen,
		frsh->Lsn, frsh->SequenceNumber, frsh->ReferenceCount, frsh->FirstAttributeOffset, frsh->Flags,
		frsh->FirstFreeByte, frsh->BytesAvailable, frsh->BaseFileRecordSegment, frsh->NextAttributeInstance, frsh->UpdateArrayForCreateOnly);
#endif

	for (;;) {
		atrh = reinterpret_cast<const ATTRIBUTE_RECORD_HEADER *>(record + off);
		if (atrh->TypeCode == TC_END) break;

#ifdef DBG_SIMPLE
		printf("%08X %08X %02X %02X %04X %04X %s",
			atrh->TypeCode, atrh->RecordLength, atrh->FormCode, atrh->NameLength,
			atrh->NameOffset, atrh->Instance, attr_str(atrh->TypeCode));
		if (atrh->NameLength) {
			printf(" : '");
			print_u16(record + off + atrh->NameOffset, atrh->NameLength);
			printf("'");
		}
		printf("\n");
#endif
#ifdef DBG_VERBOSE
		printf("TypeCode:       %08X (%s)\n", atrh->TypeCode, attr_str(atrh->TypeCode));
		printf("RecordLength:   %08X\n", atrh->RecordLength);
		printf("FormCode:       %02X\n", atrh->FormCode);
		printf("NameLength:     %02X\n", atrh->NameLength);
		printf("NameOffset:     %04X", atrh->NameOffset);
		if (atrh->NameLength) {
			printf(" (");
			print_u16(record + off + atrh->NameOffset, atrh->NameLength);
			printf(")");
		}
		printf("\n");
		printf("Instance:       %04X\n", atrh->Instance);
#endif

		if (atrh->FormCode == RESIDENT_FORM) {
#ifdef DBG_SIMPLE
			printf("R %08X %04X %02X %02X\n",
				atrh->Form.Resident.ValueLength, atrh->Form.Resident.ValueOffset,
				atrh->Form.Resident.ResidentFlags, atrh->Form.Resident.Reserved);
#endif
#ifdef DBG_VERBOSE
			printf("Resident\n");
			printf("ValueLength:    %08X\n", atrh->Form.Resident.ValueLength);
			printf("ValueOffset:    %04X\n", atrh->Form.Resident.ValueOffset);
			printf("ResidentFlags:  %02X\n", atrh->Form.Resident.ResidentFlags);
			printf("Reserved:       %02X\n", atrh->Form.Resident.Reserved);
#endif
			DumpHex(record + off + atrh->Form.Resident.ValueOffset, atrh->Form.Resident.ValueLength);

			if (atrh->TypeCode == TC_FILE_NAME) {
				const FILE_NAME *fn = reinterpret_cast<const FILE_NAME *>(record + off + atrh->Form.Resident.ValueOffset);
#ifdef DBG_SIMPLE
				printf("  %04X%08X %04X ", fn->ParentDirectory.SegmentNumberHighPart, fn->ParentDirectory.SegmentNumberLowPart, fn->ParentDirectory.SequenceNumber);
				printf("%02X '", fn->Flags);
				print_u16(fn->FileName, fn->FileNameLength);
				printf("'\n");
#endif
#ifdef DBG_VERBOSE
				printf("ParentDirectory:%04X%08X %04X\n", fn->ParentDirectory.SegmentNumberHighPart,
					fn->ParentDirectory.SegmentNumberLowPart, fn->ParentDirectory.SequenceNumber);
				printf("FileName:       '");
				print_u16(fn->FileName, fn->FileNameLength);
				printf("'\n");
#endif
			}
		} else if (atrh->FormCode == NONRESIDENT_FORM) {
#ifdef DBG_SIMPLE
			printf("N %" PRIX64 " %" PRIX64 " %04X %02X %" PRIX64 " %" PRIX64 " %" PRIX64 "\n",
				atrh->Form.Nonresident.LowestVcn, atrh->Form.Nonresident.HighestVcn,
				atrh->Form.Nonresident.MappingPairsOffset, atrh->Form.Nonresident.CompressionUnit,
				atrh->Form.Nonresident.AllocatedLength, atrh->Form.Nonresident.FileSize,
				atrh->Form.Nonresident.ValidDataLength);
#endif
#ifdef DBG_VERBOSE
			printf("Nonresident\n");
			printf("LowestVcn:      %" PRIX64 "\n", atrh->Form.Nonresident.LowestVcn);
			printf("HighestVcn:     %" PRIX64 "\n", atrh->Form.Nonresident.HighestVcn);
			printf("MappingPairsOff:%04X\n", atrh->Form.Nonresident.MappingPairsOffset);
			printf("CompressionUnit:%02X\n", atrh->Form.Nonresident.CompressionUnit);
			printf("AllocatedLength:%" PRIX64 "\n", atrh->Form.Nonresident.AllocatedLength);
			printf("FileSize:       %" PRIX64 "\n", atrh->Form.Nonresident.FileSize);
			printf("ValidDataLength:%" PRIX64 "\n", atrh->Form.Nonresident.ValidDataLength);
#endif
			// printf("TotalAllocated: %" PRIX64 "\n", atrh->Form.Nonresident.TotalAllocated);
#ifdef DBG_VERBOSE
			DumpHex(record + off + atrh->Form.Nonresident.MappingPairsOffset, atrh->RecordLength - atrh->Form.Nonresident.MappingPairsOffset);
#endif
			const uint8_t *lvp = record + off + atrh->Form.Nonresident.MappingPairsOffset;
			uint64_t NextVcn = atrh->Form.Nonresident.LowestVcn;
			uint64_t CurrentVcn = NextVcn;
			uint64_t CurrentLcn = 0;
			int64_t diff;
			int v;
			int l;

			while (lvp[0]) {
				CurrentVcn = NextVcn;

				v = lvp[0] & 0xF;
				l = (lvp[0] >> 4) & 0xF;

#ifdef DBG_SIMPLE
				printf("%02X :", lvp[0]);
				for (n = 0; n < v; n++)
					printf(" %02X", lvp[1 + n]);
				printf(" /");
				for (n = 0; n < l; n++)
					printf(" %02X", lvp[1 + v + n]);
				printf("\n");
#endif

				++lvp;

				diff = 0;
				for (n = 0; n < v; n++)
					diff |= lvp[n] << (8 * n);
				if (lvp[n - 1] & 0x80) {
					for (; n < 8; n++) {
						diff |= 0xFFULL << (8 * n);
					}
				}
				lvp += v;
				NextVcn += diff;
#ifdef DBG_SIMPLE
				printf("diff: %" PRIX64 " / ", diff);
#endif

				diff = 0;
				for (n = 0; n < l; n++)
					diff |= lvp[n] << (8 * n);
				if (lvp[n - 1] & 0x80) {
					for (; n < 8; n++)
						diff |= 0xFFULL << (8 * n);
				}
				lvp += l;
				CurrentLcn += diff;
#ifdef DBG_SIMPLE
				printf("%" PRIX64 "\n", diff);
#endif
				printf("%016" PRIX64 ": %016" PRIX64 " %016" PRIX64 "\n", CurrentVcn, NextVcn, CurrentLcn);
			}
		}
		printf("\n");
		off += atrh->RecordLength;
		if (off >= frsh->FirstFreeByte) break;
	}
}
