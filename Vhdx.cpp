#include <cmath>
#include <ctime>
#include <cstring>

#include <endian.h>

#include "Vhdx.h"

static constexpr uint64_t VHDX_SIGNATURE = 0x656C696678646876;
static constexpr uint32_t VHDX_HEAD = 0x64616568;
static constexpr uint32_t VHDX_REGI = 0x69676572;

VhdxDevice::VhdxDevice() : m_crc(true, 0x1EDC6F41), m_ident(), m_head(), m_regi(), m_cache()
{
	srand(time(NULL));

	m_file = nullptr;
	m_writable = false;
}

VhdxDevice::~VhdxDevice()
{
}

int VhdxDevice::Create(const char* name, uint64_t size)
{
	return 0;
}

int VhdxDevice::Open(const char* name, bool writable)
{
	errno_t err;
	size_t n;

	Close();

	err = fopen_s(&m_file, name, writable ? "wb" : "rb");
	if (err) return err;

	m_writable = writable;

	ImgRead(0, m_cache, 0x10000);

	{
		const VHDX_FILE_IDENTIFIER* id = reinterpret_cast<const VHDX_FILE_IDENTIFIER*>(m_cache);
		if (le64toh(id->Signature) != VHDX_SIGNATURE) {
			Close();
			return EINVAL;
		}
		m_ident.Signature = le64toh(id->Signature);
		for (n = 0; n < 256; n++)
			m_ident.Creator[n] = le16toh(id->Creator[n]);
	}

	ReadHeader(m_head[0], 0x10000);
	ReadHeader(m_head[1], 0x20000);



	return 0;
}

void VhdxDevice::Close()
{
	if (m_file) {
		fclose(m_file);
		m_file = nullptr;
	}
}

int VhdxDevice::Read(void* data, size_t size, uint64_t offset)
{
	return 0;
}

int VhdxDevice::Write(const void* data, size_t size, uint64_t offset)
{
	return 0;
}

uint64_t VhdxDevice::GetSize() const
{
	return uint64_t();
}

int VhdxDevice::ReadHeader(VHDX_HEADER& header, uint64_t offset)
{
	uint32_t crc;
	uint32_t ccrc;
	int err;
	VHDX_HEADER* h;

	err = ImgRead(offset, m_cache, 0x10000);
	if (err)
		return err;

	h = reinterpret_cast<VHDX_HEADER*>(m_cache);
	crc = le32toh(h->Checksum);
	h->Checksum = 0;

	ccrc = m_crc.GetDataCRC(m_cache, 0x10000, 0xFFFFFFFF, 0xFFFFFFFF);
	if (ccrc != crc)
		return EINVAL;

	header.Signature = le32toh(h->Signature);
	header.Checksum = 0;
	header.SequenceNumber = le64toh(h->SequenceNumber);
	header.FileWriteGuid = h->FileWriteGuid;
	header.DataWriteGuid = h->DataWriteGuid;
	header.LogGuid = h->LogGuid;
	header.LogVersion = le16toh(h->LogVersion);
	header.Version = le16toh(h->Version);
	header.LogLength = le32toh(h->LogLength);
	header.LogOffset = le64toh(h->LogOffset);

	return 0;
}

int VhdxDevice::WriteHeader(const VHDX_HEADER& header, uint64_t offset)
{
	VHDX_HEADER* h = reinterpret_cast<VHDX_HEADER*>(m_cache);

	memset(m_cache, 0, sizeof(m_cache));

	h->Signature = htole32(header.Signature);
	h->Checksum = 0;
	h->SequenceNumber = htole64(header.SequenceNumber);
	h->FileWriteGuid = header.FileWriteGuid;
	h->DataWriteGuid = header.DataWriteGuid;
	h->LogGuid = header.LogGuid;
	h->LogVersion = htole16(header.LogVersion);
	h->Version = htole16(header.Version);
	h->LogLength = htole32(header.LogLength);
	h->LogOffset = htole64(header.LogOffset);

	h->Checksum = htole32(m_crc.GetDataCRC(m_cache, 0x10000, 0xFFFFFFFF, 0xFFFFFFFF));

	return ImgWrite(offset, m_cache, 0x10000);
}

int VhdxDevice::ReadRegionTable(VHDX_REGION_TABLE& table, uint64_t offset)
{
	uint32_t n;
	int err;
	uint32_t crc;
	uint32_t ccrc;
	VHDX_REGION_TABLE* t = reinterpret_cast<VHDX_REGION_TABLE*>(m_cache);

	err = ImgRead(offset, m_cache, 0x10000);
	if (err)
		return err;
	if (le32toh(t->hdr.Signature) != VHDX_REGI)
		return EINVAL;

	crc = le32toh(t->hdr.Checksum);
	t->hdr.Checksum = 0;
	ccrc = m_crc.GetDataCRC(m_cache, 0x10000, 0xFFFFFFFF, 0xFFFFFFFF);

	if (crc != ccrc)
		return EINVAL;

	table.hdr.Signature = le32toh(t->hdr.Signature);
	table.hdr.Checksum = 0;
	table.hdr.EntryCount = le32toh(t->hdr.EntryCount);
	table.hdr.Reserved = le32toh(t->hdr.Reserved);

	for (n = 0; n < table.hdr.EntryCount; n++) {
		table.entries[n].Guid = t->entries[n].Guid;
		table.entries[n].FileOffset = le64toh(t->entries[n].FileOffset);
		table.entries[n].Length = le32toh(t->entries[n].Length);
		table.entries[n].Flags = le32toh(t->entries[n].Flags);
	}

	return 0;
}

int VhdxDevice::WriteRegionTable(const VHDX_REGION_TABLE& table, uint64_t offset)
{
	return 0;
}

void VhdxDevice::GenerateRandomGUID(MS_GUID& guid)
{
	for (int n = 0; n < 16; n++)
		guid.d[n] = rand();
	guid.d[7] = (guid.d[7] & 0x0F) | 0x40;
	guid.d[8] = (guid.d[8] & 0x3F) | 0x80;
}

int VhdxDevice::ImgRead(uint64_t offset, void* buffer, size_t size)
{
	size_t nread;

	if (m_file == nullptr)
		return EINVAL;

	_fseeki64(m_file, offset, SEEK_SET);
	nread = fread(buffer, 1, size, m_file);

	if (nread != size)
		return errno;

	return 0;
}

int VhdxDevice::ImgWrite(uint64_t offset, const void* buffer, size_t size)
{
	size_t nwritten;

	if (m_file == nullptr)
		return EINVAL;

	if (!m_writable)
		return EACCES;

	_fseeki64(m_file, offset, SEEK_SET);
	nwritten = fwrite(buffer, 1, size, m_file);

	if (nwritten != size)
		return errno;

	return 0;
}
