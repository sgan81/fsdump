#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <cerrno>
#include <endian.h>

#include "Device.h"
#include "GptPartitionMap.h"

static_assert(sizeof(GptPartitionMap::PMAP_GptHeader) == 96, "PMAP GPT-Header wrong size");
static_assert(sizeof(GptPartitionMap::PMAP_Entry) == 128, "PMAP Entry wrong size");

const GptPartitionMap::PM_GUID GptPartitionMap::PTYPE_EFI_SYS = { 0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B };
const GptPartitionMap::PM_GUID GptPartitionMap::PTYPE_APFS = { 0xEF, 0x57, 0x34, 0x7C, 0x00, 0x00, 0xAA, 0x11, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC };

static void PrintGUID(const GptPartitionMap::PM_GUID &guid)
{
	printf("%02X%02X%02X%02X-", guid[3], guid[2], guid[1], guid[0]);
	printf("%02X%02X-", guid[5], guid[4]);
	printf("%02X%02X-", guid[7], guid[6]);
	printf("%02X%02X-", guid[9], guid[8]);
	printf("%02X%02X%02X%02X%02X%02X", guid[10], guid[11], guid[12], guid[13], guid[14], guid[15]);
}

GptPartitionMap::GptPartitionMap() : m_crc(true)
{
	m_hdr = nullptr;
	m_map = nullptr;
	m_sector_size = 0x200;
}

bool GptPartitionMap::LoadAndVerify(Device & dev)
{
	PMAP_GptHeader *hdr;

	m_hdr = nullptr;
	m_map = nullptr;
	m_entry_data.clear();
	m_hdr_data.clear();

	m_sector_size = dev.GetSectorSize();

	m_hdr_data.resize(m_sector_size);

	dev.Read(m_hdr_data.data(), m_sector_size, m_sector_size);

	hdr = reinterpret_cast<PMAP_GptHeader *>(m_hdr_data.data());

	if (le64toh(hdr->Signature) != 0x5452415020494645)
		return false;

	if (le32toh(hdr->Revision) != 0x00010000)
		return false;

	if (le32toh(hdr->HeaderSize) > m_sector_size)
		return false;

	if (le32toh(hdr->SizeOfPartitionEntry) != 0x80)
		return false;

	uint32_t hdr_crc;
	uint32_t calc_crc;

	hdr_crc = le32toh(hdr->HeaderCRC32);
	hdr->HeaderCRC32 = 0;

	m_crc.SetCRC(0xFFFFFFFF);
	m_crc.Calc(m_hdr_data.data(), le32toh(hdr->HeaderSize));
	calc_crc = m_crc.GetCRC() ^ 0xFFFFFFFF;

	if (calc_crc != hdr_crc) {
		m_hdr_data.clear();
		return false;
	}

	size_t mapsize;

	mapsize = le32toh(hdr->NumberOfPartitionEntries) * le32toh(hdr->SizeOfPartitionEntry);
	mapsize = (mapsize + m_sector_size - 1) & ~(m_sector_size - 1);

	m_entry_data.resize(mapsize);
	dev.Read(m_entry_data.data(), m_entry_data.size(), m_sector_size * le64toh(hdr->PartitionEntryLBA));

	m_crc.SetCRC(0xFFFFFFFF);
	m_crc.Calc(m_entry_data.data(), le32toh(hdr->SizeOfPartitionEntry) * le32toh(hdr->NumberOfPartitionEntries));
	calc_crc = m_crc.GetCRC() ^ 0xFFFFFFFF;

	if (calc_crc != le32toh(hdr->PartitionEntryArrayCRC32)) {
		m_entry_data.clear();
		m_hdr_data.clear();
		return false;
	}

	m_hdr = reinterpret_cast<const PMAP_GptHeader *>(m_hdr_data.data());
	m_map = reinterpret_cast<const PMAP_Entry *>(m_entry_data.data());

	return true;
}

int GptPartitionMap::FindFirstAPFSPartition()
{
	if (!m_hdr || !m_map)
		return -1;

	unsigned int k;
	int rc = -1;

	for (k = 0; k < le32toh(m_hdr->NumberOfPartitionEntries); k++)
	{
		if (le64toh(m_map[k].StartingLBA) == 0 && le64toh(m_map[k].EndingLBA) == 0)
			break;

		if (!memcmp(m_map[k].PartitionTypeGUID, PTYPE_APFS, sizeof(PM_GUID)))
		{
			rc = k;
			break;
		}
	}

	return rc;
}

bool GptPartitionMap::GetPartitionOffsetAndSize(int partnum, uint64_t & offset, uint64_t & size)
{
	if (!m_hdr || !m_map)
		return false;

	offset = le64toh(m_map[partnum].StartingLBA) * m_sector_size;
	size = (le64toh(m_map[partnum].EndingLBA) - le64toh(m_map[partnum].StartingLBA) + 1) * m_sector_size;

	return true;
}

bool GptPartitionMap::GetPartitionEntry(int partnum, GptPartitionMap::PMAP_Entry& pe)
{
	if (__BYTE_ORDER == __LITTLE_ENDIAN) {
		pe = m_map[partnum];
		return true;
	} else {
		return false; // TODO ...
	}
}

void GptPartitionMap::ListEntries()
{
	if (!m_hdr || !m_map)
		return;

	size_t k;
	size_t n;

	for (k = 0; k < le32toh(m_hdr->NumberOfPartitionEntries); k++)
	{
		const PMAP_Entry &e = m_map[k];

		if (le64toh(e.StartingLBA) == 0 && le64toh(e.EndingLBA) == 0)
			break;

		PrintGUID(e.PartitionTypeGUID);
		printf(" ");
		PrintGUID(e.UniquePartitionGUID);
		printf(" %016" PRIX64 " %016" PRIX64 " ", le64toh(e.StartingLBA), le64toh(e.EndingLBA));
		printf("%016" PRIX64 " ", le64toh(e.Attributes));

		for (n = 0; (n < 36) && (e.PartitionName[n] != 0); n++)
			printf("%c", le16toh(e.PartitionName[n]));

		printf("\n");
	}
}

int GptPartitionMap::CopyGPT(Device& src, Device& dst)
{
	uint8_t buf[0x1000];

	uint64_t pmap_off;
	size_t pmap_size;
	size_t size;

	printf("Header size: %08X\n", le32toh(m_hdr->HeaderSize));
	printf("Entry size: %08X\n", le32toh(m_hdr->SizeOfPartitionEntry));
	printf("My LBA: %016" PRIX64 "\n", le64toh(m_hdr->MyLBA));
	printf("Alt LBA: %016" PRIX64 "\n", le64toh(m_hdr->AlternateLBA));
	printf("Pe LBA: %016" PRIX64 "\n", le64toh(m_hdr->PartitionEntryLBA));

	src.Read(buf, m_sector_size, 0);
	dst.Write(buf, m_sector_size, 0);
	src.Read(buf, m_sector_size, m_sector_size);
	dst.Write(buf, m_sector_size, m_sector_size);

	pmap_off = le64toh(m_hdr->PartitionEntryLBA) * m_sector_size;
	pmap_size = le32toh(m_hdr->NumberOfPartitionEntries) * le32toh(m_hdr->SizeOfPartitionEntry);

	printf("pmap_off = %" PRIX64 " pmap_size = %" PRIX64 "\n", pmap_off, pmap_size);

	while (pmap_size > 0) {
		size = pmap_size;
		if (size > 0x1000) size = 0x1000;
		printf("read %p %zX %" PRIX64 "\n", buf, size, pmap_off);
		src.Read(buf, size, pmap_off);
		printf("write same\n");
		dst.Write(buf, size, pmap_off);
		pmap_off += size;
		pmap_size -= size;
	}

	src.Read(buf, m_sector_size, le64toh(m_hdr->AlternateLBA) * m_sector_size);
	dst.Write(buf, m_sector_size, le64toh(m_hdr->AlternateLBA) * m_sector_size);

	const PMAP_GptHeader *alt_hdr = reinterpret_cast<const PMAP_GptHeader *>(buf);

	printf("Pe LBA: %016" PRIX64 "\n", le64toh(alt_hdr->PartitionEntryLBA));

	pmap_off = le64toh(alt_hdr->PartitionEntryLBA) * m_sector_size;
	pmap_size = le32toh(alt_hdr->NumberOfPartitionEntries) * le32toh(alt_hdr->SizeOfPartitionEntry);

	printf("pmap_off = %" PRIX64 " pmap_size = %" PRIX64 "\n", pmap_off, pmap_size);

	while (pmap_size > 0) {
		size = pmap_size;
		if (size > 0x1000) size = 0x1000;
		printf("read %p %zX %" PRIX64 "\n", buf, size, pmap_off);
		src.Read(buf, size, pmap_off);
		printf("write same\n");
		dst.Write(buf, size, pmap_off);
		pmap_off += size;
		pmap_size -= size;
	}

	return ENOTSUP;
}
