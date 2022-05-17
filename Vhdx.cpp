#include <cmath>
#include <ctime>
#include <cstring>
#include <cinttypes>

#include <endian.h>

#include "Vhdx.h"

static constexpr uint64_t VHDX_SIGNATURE = 0x656C696678646876;
static constexpr uint32_t VHDX_SIG_HEAD = 0x64616568;
static constexpr uint32_t VHDX_SIG_REGI = 0x69676572;

static constexpr uint32_t VHDX_SIG_LOGE = 0x65676F6C;
static constexpr uint32_t VHDX_SIG_ZERO = 0x6F72657A;
static constexpr uint32_t VHDX_SIG_DESC = 0x63736564;
static constexpr uint32_t VHDX_SIG_DATA = 0x61746164;

static constexpr uint64_t VHDX_SIG_META = 0x617461646174656D;

static constexpr MS_GUID GUID_BAT = { 0x66, 0x77, 0xC2, 0x2D, 0x23, 0xF6, 0x00, 0x42, 0x9D, 0x64, 0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08 };
static constexpr MS_GUID GUID_Metadata = { 0x06, 0xA2, 0x7C, 0x8B, 0x90, 0x47, 0x9A, 0x4B, 0xB8, 0xFE, 0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E };

static constexpr MS_GUID GUID_FILE_PARAMETERS = { 0x37, 0x67, 0xA1, 0xCA, 0x36, 0xFA, 0x43, 0x4D, 0xB3, 0xB6, 0x33, 0xF0, 0xAA, 0x44, 0xE7, 0x6B };
static constexpr MS_GUID GUID_VIRTUAL_DISK_SIZE = { 0x24, 0x42, 0xA5, 0x2F, 0x1B, 0xCD, 0x76, 0x48, 0xB2, 0x11, 0x5D, 0xBE, 0xD8, 0x3B, 0xF4, 0xB8 };
static constexpr MS_GUID GUID_VIRTUAL_DISK_ID = { 0xAB, 0x12, 0xCA, 0xBE, 0xE6, 0xB2, 0x23, 0x45, 0x93, 0xEF, 0xC3, 0x09, 0xE0, 0x00, 0xC7, 0x46 };
static constexpr MS_GUID GUID_LOGICAL_SECTOR_SIZE = { 0x1D, 0xBF, 0x41, 0x81, 0x6F, 0xA9, 0x09, 0x47, 0xBA, 0x47, 0xF2, 0x33, 0xA8, 0xFA, 0xAB, 0x5F };
static constexpr MS_GUID GUID_PHYSICAL_SECTOR_SIZE = { 0xC7, 0x48, 0xA3, 0xCD, 0x5D, 0x44, 0x71, 0x44, 0x9C, 0xC9, 0xE9, 0x88, 0x52, 0x51, 0xC5, 0x56 };
static constexpr MS_GUID GUID_PARENT_LOCATOR = { 0x2D, 0x5F, 0xD3, 0xA8, 0x0B, 0xB3, 0x4D, 0x45, 0xAB, 0xF7, 0xD3, 0xD8, 0x48, 0x34, 0xAB, 0x0C };

static constexpr MS_GUID GUID_Zero = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static constexpr uint32_t REGI_FLAG_REQUIRED = 0x00000001;

static constexpr uint32_t META_FLAGS_IS_USER = 1;
static constexpr uint32_t META_FLAGS_IS_VIRTUAL_DISK = 2;
static constexpr uint32_t META_FLAGS_IS_REQUIRED = 4;

#if 1

#define trace printf

static void trace_guid(const MS_GUID& guid, const char *pfx)
{
	struct ms_guid_t {
		uint32_t d1;
		uint16_t d2;
		uint16_t d3;
		uint8_t d4[8];
	};
	const ms_guid_t* g = reinterpret_cast<const ms_guid_t*>(&guid);
	if (pfx)
		printf("%s", pfx);
	printf("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", g->d1, g->d2, g->d3, g->d4[0], g->d4[1], g->d4[2], g->d4[3], g->d4[4], g->d4[5], g->d4[6], g->d4[7]);
	if (pfx)
		printf("\n");
}

#else
#define trace(...)
#define trace_guid(guid, pfx)
#endif

VhdxDevice::VhdxDevice() : m_crc(true, 0x1EDC6F41), m_ident(), m_head(), m_regi(), m_cache(), m_virtual_disk_id()
{
	srand(time(NULL));

	m_file = nullptr;
	m_writable = false;

	m_active_header = -1;

	m_bat_offset = 0;
	m_bat_size = 0;
	m_bat_flags = 0;
	m_meta_offset = 0;
	m_meta_size = 0;
	m_meta_flags = 0;

	m_block_size = 0;
	m_leave_alloc = false;
	m_has_parent = false;
	m_disk_size = 0;
	m_sector_size_logical = 0;
	m_sector_size_physical = 0;

	m_chunk_ratio = 0;
	m_data_blocks_count = 0;
	m_sector_bitmap_blocks_count = 0;
	m_bat_entries_cnt = 0;

	m_img_size = 0;
}

VhdxDevice::~VhdxDevice()
{
	Close();
}

int VhdxDevice::Create(const char* name, uint64_t size)
{
	errno_t err;
	int n;
	static const char16_t* creator = u"FsDump 0.1";
	uint64_t page_ptr = 0x100000;

	Close();

	err = fopen_s(&m_file, name, "wb");
	if (err) return err;

	m_writable = true;

	m_disk_size = size;
	m_sector_size_logical = 512; // Assuming ...
	m_sector_size_physical = 4096; // Assuming as well ...
	// TODO get these from the device and pass as parameters ...
	m_block_size = 0x2000000;

	m_chunk_ratio = (static_cast<uint64_t>(m_sector_size_logical) << 23) / m_block_size;
	m_data_blocks_count = (m_disk_size + m_block_size - 1) / m_block_size;
	m_sector_bitmap_blocks_count = (m_data_blocks_count + m_chunk_ratio - 1) / m_chunk_ratio;
	m_bat_entries_cnt = m_data_blocks_count + (m_data_blocks_count - 1) / m_chunk_ratio;

	{
		memset(m_cache, 0, sizeof(m_cache));
		VHDX_FILE_IDENTIFIER* id = reinterpret_cast<VHDX_FILE_IDENTIFIER*>(m_cache);
		id->Signature = htole64(VHDX_SIGNATURE);
		for (n = 0; creator[n] != 0; n++)
			id->Creator[n] = htole16(creator[n]);
		id->Creator[n] = 0;
		ImgWrite(0, m_cache, 0x10000);
	}

	m_head[0].Signature = VHDX_SIG_HEAD;
	m_head[0].Checksum = 0;
	m_head[0].SequenceNumber = 1;
	GenerateRandomGUID(m_head[0].FileWriteGuid);
	GenerateRandomGUID(m_head[0].DataWriteGuid);
	m_head[0].LogGuid = GUID_Zero;
	m_head[0].LogVersion = 0;
	m_head[0].Version = 1;
	m_head[0].LogLength = 0x100000;
	m_head[0].LogOffset = page_ptr;
	page_ptr += m_head[0].LogLength;

	m_head[1] = m_head[0];
	m_head[1].SequenceNumber = 2;

	WriteHeader(m_head[0], 0x10000);
	WriteHeader(m_head[1], 0x20000);

	m_meta_offset = page_ptr;
	m_meta_size = 0x100000;
	m_meta_flags = REGI_FLAG_REQUIRED;
	page_ptr += m_meta_size;
	m_bat_offset = 0x300000;
	m_bat_size = m_bat_entries_cnt * sizeof(uint64_t);
	if (m_bat_size & 0xFFFFF)
		m_bat_size = (m_bat_size + 0x100000) & 0xFFF00000;
	m_bat_flags = REGI_FLAG_REQUIRED;
	page_ptr += m_bat_size;

	SetupRegionTable(m_regi[0]);
	SetupRegionTable(m_regi[1]);

	WriteRegionTable(m_regi[0], 0x30000);
	WriteRegionTable(m_regi[1], 0x40000);

	ImgResize(page_ptr);
	
	m_log_b.resize(m_head[0].LogLength);
	m_meta_b.resize(m_meta_size);
	m_bat_b.resize(m_bat_size);

	ImgWrite(m_head[0].LogOffset, m_log_b.data(), m_log_b.size());
	ImgWrite(m_meta_offset, m_meta_b.data(), m_meta_b.size());
	ImgWrite(m_bat_offset, m_bat_b.data(), m_bat_b.size());

	Close();

	return ENOTSUP;
}

int VhdxDevice::Open(const char* name, bool writable)
{
	errno_t err;
	size_t n;

	Close();

	err = fopen_s(&m_file, name, writable ? "wb" : "rb");
	if (err) return err;

	err = ImgRead(0, m_cache, 0x10000);
	if (err) {
		Close();
		return err;
	}

	m_writable = writable;

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

	if (m_head[0].Signature != VHDX_SIG_HEAD && m_head[1].Signature != VHDX_SIG_HEAD) {
		Close();
		return EINVAL;
	}

	if (m_head[0].Signature != VHDX_SIG_HEAD)
		m_active_header = 1;
	else if (m_head[1].Signature != VHDX_SIG_HEAD)
		m_active_header = 0;
	else {
		m_active_header = m_head[1].SequenceNumber > m_head[0].SequenceNumber ? 1 : 0;
	}

	trace("Active Header = %d\n", m_active_header);

	LogReplay();

	ReadRegionTable(m_regi[0], 0x30000);
	ReadRegionTable(m_regi[1], 0x40000);

	const VHDX_REGION_TABLE& rt = m_regi[m_active_header];

	for (n = 0; n < rt.hdr.EntryCount; n++) {
		const VHDX_REGION_TABLE_ENTRY& e = rt.entries[n];
		if (e.Guid == GUID_Metadata) {
			m_meta_offset = e.FileOffset;
			m_meta_size = e.Length;
			m_meta_flags = e.Flags;
		}
		else if (e.Guid == GUID_BAT) {
			m_bat_offset = e.FileOffset;
			m_bat_size = e.Length;
			m_bat_flags = e.Flags;
		}
		else {
			printf("Unknown GUID in region table.\n");
			if (e.Flags & REGI_FLAG_REQUIRED) {
				printf("GUID is required ... terminating.\n");
				Close();
				return ENOTSUP;
			}
		}
	}

	ReadMetadata(m_meta_offset, m_meta_size);
	ReadBAT(m_bat_offset, m_bat_size);

	_fseeki64(m_file, 0, SEEK_END);
	m_img_size = _ftelli64(m_file);

	return 0;
}

void VhdxDevice::Close()
{
	if (m_file) {
		if (m_writable)
			_fseeki64(m_file, m_img_size, SEEK_SET);

		fclose(m_file);
	}

	m_file = nullptr;
	m_writable = false;

	m_active_header = -1;

	m_bat_offset = 0;
	m_bat_size = 0;
	m_bat_flags = 0;
	m_meta_offset = 0;
	m_meta_size = 0;
	m_meta_flags = 0;

	m_block_size = 0;
	m_leave_alloc = false;
	m_has_parent = false;
	m_disk_size = 0;
	m_sector_size_logical = 0;
	m_sector_size_physical = 0;

	m_chunk_ratio = 0;
	m_data_blocks_count = 0;
	m_sector_bitmap_blocks_count = 0;
	m_bat_entries_cnt = 0;
}

int VhdxDevice::Read(void* data, size_t size, uint64_t offset)
{
	if (!m_file)
		return EINVAL;

	uint8_t* bdata = reinterpret_cast<uint8_t*>(data);
	const uint64_t mask = static_cast<uint64_t>(m_block_size) - 1;
	uint64_t off_hi;
	uint64_t bat_entry;
	uint64_t bat_off;
	uint64_t file_off;
	VHDX_BAT_STATE st;
	uint32_t off_lo;
	uint32_t cur_blk_size;

	offset += GetPartitionStart();

	if ((offset + size) > m_disk_size)
		return EINVAL;

	while (size > 0) {
		off_hi = offset & ~mask;
		off_lo = offset & mask;
		bat_off = off_hi / m_block_size;
		bat_off = bat_off + (bat_off / m_chunk_ratio);
		bat_entry = le64toh(m_bat_b[bat_off]);
		st = static_cast<VHDX_BAT_STATE>(bat_entry & 7);
		file_off = bat_entry & 0xFFFFFFFFFFF00000U;
		cur_blk_size = m_block_size - off_lo;
		if (size < cur_blk_size)
			cur_blk_size = size;

		if (file_off == 0)
			memset(bdata, 0, cur_blk_size);
		else
			ImgRead(file_off + off_lo, bdata, cur_blk_size);

		offset += cur_blk_size;
		size -= cur_blk_size;
		bdata += cur_blk_size;
	}

	return 0;
}

int VhdxDevice::Write(const void* data, size_t size, uint64_t offset)
{
	return ENOTSUP;
}

uint64_t VhdxDevice::GetSize() const
{
	return m_disk_size;
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

	ccrc = m_crc.GetDataCRC(m_cache, 0x1000, 0xFFFFFFFF, 0xFFFFFFFF);
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

	trace("head seq=%" PRId64 " lver=%d ver=%d llen=%X loff=%" PRIX64 "\n", header.SequenceNumber, header.LogVersion, header.Version, header.LogLength, header.LogOffset);
	trace_guid(header.FileWriteGuid, "  FileWriteGuid: ");
	trace_guid(header.DataWriteGuid, "  DataWriteGuid: ");
	trace_guid(header.LogGuid, "  LogGuid:       ");

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
	if (le32toh(t->hdr.Signature) != VHDX_SIG_REGI)
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

	trace("regi %d\n", table.hdr.EntryCount);

	for (n = 0; n < table.hdr.EntryCount; n++) {
		table.entries[n].Guid = t->entries[n].Guid;
		table.entries[n].FileOffset = le64toh(t->entries[n].FileOffset);
		table.entries[n].Length = le32toh(t->entries[n].Length);
		table.entries[n].Flags = le32toh(t->entries[n].Flags);
		if (table.entries[n].Guid == GUID_BAT)
			printf("  BAT:  ");
		else if (table.entries[n].Guid == GUID_Metadata)
			printf("  META: ");
		trace(" %" PRIX64 " %08X %08X\n", table.entries[n].FileOffset, table.entries[n].Length, table.entries[n].Flags);
	}

	return 0;
}

int VhdxDevice::SetupRegionTable(VHDX_REGION_TABLE& table)
{
	table.hdr.Signature = VHDX_SIG_REGI;
	table.hdr.Checksum = 0;
	table.hdr.EntryCount = 2;
	table.hdr.Reserved = 0;

	table.entries[0].Guid = GUID_BAT;
	table.entries[0].FileOffset = m_bat_offset;
	table.entries[0].Length = m_bat_size;
	table.entries[0].Flags = REGI_FLAG_REQUIRED;

	table.entries[1].Guid = GUID_Metadata;
	table.entries[1].FileOffset = m_meta_offset;
	table.entries[1].Length = m_meta_size;
	table.entries[1].Flags = REGI_FLAG_REQUIRED;

	return 0;
}

int VhdxDevice::WriteRegionTable(const VHDX_REGION_TABLE& table, uint64_t offset)
{
	return 0;
}

int VhdxDevice::ReadMetadata(uint64_t offset, uint32_t length)
{
	int err;
	uint16_t n;
	uint16_t cnt;
	uint32_t off;
	uint32_t len;
	void* vp;

	m_meta_b.resize(length);
	err = ImgRead(offset, m_meta_b.data(), length);
	if (err)
		return err;

	const VHDX_METADATA_TABLE_HEADER* h = reinterpret_cast<const VHDX_METADATA_TABLE_HEADER*>(m_meta_b.data());
	const VHDX_METADATA_TABLE_ENTRY* e = reinterpret_cast<const VHDX_METADATA_TABLE_ENTRY*>(h + 1);

	if (le64toh(h->Signature) != VHDX_SIG_META)
		return EINVAL;

	cnt = le16toh(h->EntryCount);

	trace("Metadata:\n");
	for (n = 0; n < cnt; n++) {
		off = le32toh(e[n].Offset);
		len = le32toh(e[n].Length);
		vp = m_meta_b.data() + off;

		trace("%08X %08X ", off, len);
		trace_guid(e[n].ItemId, nullptr);
		trace(" : ");

		if (e[n].ItemId == GUID_FILE_PARAMETERS) {
			const VHDX_FILE_PARAMETERS* p = reinterpret_cast<const VHDX_FILE_PARAMETERS*>(vp);
			uint32_t flags = le32toh(p->Flags);
			m_block_size = le32toh(p->BlockSize);
			m_leave_alloc = (flags & 0x80000000) != 0;
			m_has_parent = (flags & 0x40000000) != 0;
			trace("File Parameters: BlockSize = %X, LeaveAlloc=%d, HasParent=%d\n", m_block_size, m_leave_alloc, m_has_parent);
		}
		else if (e[n].ItemId == GUID_VIRTUAL_DISK_SIZE) {
			const VHDX_VIRTUAL_DISK_SIZE* p = reinterpret_cast<const VHDX_VIRTUAL_DISK_SIZE*>(vp);
			m_disk_size = le64toh(p->VirtualDiskSize);
			trace("Virtual Disk Size: %" PRId64 " Bytes\n", m_disk_size);
		}
		else if (e[n].ItemId == GUID_VIRTUAL_DISK_ID) {
			const VHDX_VIRTUAL_DISK_ID* p = reinterpret_cast<const VHDX_VIRTUAL_DISK_ID*>(vp);
			m_virtual_disk_id = p->VirtualDiskId;
			trace_guid(m_virtual_disk_id, "Virtual Disk ID: ");
		}
		else if (e[n].ItemId == GUID_LOGICAL_SECTOR_SIZE) {
			const VHDX_LOGICAL_SECTOR_SIZE* p = reinterpret_cast<const VHDX_LOGICAL_SECTOR_SIZE*>(vp);
			m_sector_size_logical = le32toh(p->LogicalSectorSize);
			trace("Logical Sector Size: %d\n", m_sector_size_logical);
		}
		else if (e[n].ItemId == GUID_PHYSICAL_SECTOR_SIZE) {
			const VHDX_PHYSICAL_SECTOR_SIZE* p = reinterpret_cast<const VHDX_PHYSICAL_SECTOR_SIZE*>(vp);
			m_sector_size_physical = le32toh(p->PhysicalSectorSize);
			trace("Physical Sector Size: %d\n", m_sector_size_physical);
		}
		else if (e[n].ItemId == GUID_PARENT_LOCATOR) {
			trace("Parent Locator not supported.\n");
		}
		else {
			trace("\n");
		}
	}

	trace("\n");

	return 0;
}

int VhdxDevice::ReadBAT(uint64_t offset, uint32_t length)
{
	int err;
	uint64_t e;
	size_t n;

	m_bat_b.resize(length / sizeof(uint64_t));
	err = ImgRead(offset, m_bat_b.data(), length);
	if (err)
		return err;

	m_chunk_ratio = (static_cast<uint64_t>(m_sector_size_logical) << 23) / m_block_size;
	m_data_blocks_count = (m_disk_size + m_block_size - 1) / m_block_size;
	m_sector_bitmap_blocks_count = (m_data_blocks_count + m_chunk_ratio - 1) / m_chunk_ratio;
	m_bat_entries_cnt = m_data_blocks_count + (m_data_blocks_count - 1) / m_chunk_ratio;
	// TODO ... differencing VHDX would be different ... but we don't support it anyways

	trace("Chunk Ratio: %" PRId64 "\n", m_chunk_ratio);
	trace("Data Blocks Count: %" PRId64 "\n", m_data_blocks_count);
	trace("Sector Bitmap Blocks Count: %" PRId64 "\n", m_sector_bitmap_blocks_count);
	trace("BAT Entries Count: %" PRId64 "\n", m_bat_entries_cnt);

#if 1
	uint64_t FileOffsetMB;
	VHDX_BAT_STATE st;
	for (n = 0; n < m_bat_entries_cnt; n++) {
		e = le64toh(m_bat_b[n]);
		st = static_cast<VHDX_BAT_STATE>(e & 7);
		FileOffsetMB = (e & 0xFFFFFFFFFFF00000U);
		trace("%08" PRIX64 " : % d : % 016" PRIX64 " ", n, st, FileOffsetMB);
		switch (st) {
		case PAYLOAD_BLOCK_NOT_PRESENT: trace("NOT_PRESENT"); break;
		case PAYLOAD_BLOCK_UNDEFINED: trace("UNDEFINED"); break;
		case PAYLOAD_BLOCK_ZERO: trace("ZERO"); break;
		case PAYLOAD_BLOCK_UNMAPPED: trace("UNMAPPED"); break;
		case PAYLOAD_BLOCK_FULLY_PRESENT: trace("FULLY_PRESENT"); break;
		case PAYLOAD_BLOCK_PARTIALLY_PRESENT: trace("PARTIALLY_PRESENT"); break;
		}
		trace("\n");
	}
#endif

	return 0;
}

int VhdxDevice::LogReplay()
{
	const VHDX_HEADER& h = m_head[m_active_header];
	int err;
	uint32_t signature;
	uint32_t n;
	uint32_t count;
	size_t off;
	uint8_t block[0x1000];
	uint32_t crc;
	uint32_t ccrc;

	m_log_b.resize(h.LogLength);
	err = ImgRead(h.LogOffset, m_log_b.data(), m_log_b.size());
	if (err) return err;

	if (h.LogGuid == GUID_Zero)
		return 0;
	else if (!m_writable)
		return EFAULT;

	VHDX_LOG_ENTRY_HEADER* lh;

	for (off = 0; off < m_log_b.size(); off += 0x1000) {
		lh = reinterpret_cast<VHDX_LOG_ENTRY_HEADER*>(m_log_b.data() + off);
		signature = le32toh(lh->Signature);

		if (signature == VHDX_SIG_LOGE) {
			crc = le32toh(lh->Checksum);
			lh->Checksum = 0;
			ccrc = m_crc.GetDataCRC(m_log_b.data() + off, le32toh(lh->EntryLength), 0xFFFFFFFF, 0xFFFFFFFF);
			lh->Checksum = htole32(crc);
			if (crc != ccrc) {
				trace("CRC error\n");
				continue;
			}

			trace("loge len=%X tail=%X seq=%" PRId64 " cnt=%d ffo=%" PRIX64 " lfo=%" PRIX64,
				lh->EntryLength, lh->Tail, lh->SequenceNumber, lh->DescriptorCount, lh->FlushedFileOffset, lh->LastFileOffset);
			trace_guid(lh->LogGuid, " guid=");
			const VHDX_LOG_DATA_DESCRIPTOR* desc = reinterpret_cast<const VHDX_LOG_DATA_DESCRIPTOR*>(lh + 1);
			count = le32toh(lh->DescriptorCount);
			for (n = 0; n < count; n++) {
				trace("desc ... %" PRIX64 " %" PRId64 "\n", desc[n].FileOffset, desc[n].SequenceNumber);
			}
		}
		else if (signature == VHDX_SIG_DATA) {
			const VHDX_LOG_DATA_SECTOR* d = reinterpret_cast<const VHDX_LOG_DATA_SECTOR*>(lh);
			trace("data %" PRId64 "\n", static_cast<uint64_t>(d->SequenceHigh) << 32 | d->SequenceLow);
		}
		else if (signature == VHDX_SIG_DESC) {
			trace("desc ...\n");
		}
		else if (signature == VHDX_SIG_ZERO) {
			trace("zero ...\n");
		}
		else if (signature == 0) {

		}
		else {
			trace("????\n");
		}
	}

	return 0;
}

int VhdxDevice::LogStart()
{
	return 0;
}

int VhdxDevice::LogWrite(uint64_t offset, void* block_4k)
{
	return 0;
}

int VhdxDevice::LogCommit()
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

	if (nwritten != size) {
		perror("Error writing data");
		return errno;
	}

	return 0;
}

int VhdxDevice::ImgResize(uint64_t size)
{
	m_img_size = size;

	return 0;
}
