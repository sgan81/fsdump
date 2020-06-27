
#include <cassert>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <endian.h>

#include "AppleSparseimage.h"

static constexpr int SECTOR_SIZE = 0x200;
static constexpr size_t NODE_SIZE = 0x1000;
static constexpr size_t BAND_SIZE = 0x100000;

static constexpr uint32_t SPRS_SIGNATURE = 0x73707273;

static uint32_t ilog2(uint32_t val)
{
	uint32_t res = 0;

	assert(val != 0);

	if (val & 0xFFFF0000) { res += 16; val >>= 16; }
	if (val & 0x0000FF00) { res += 8; val >>= 8; }
	if (val & 0x000000F0) { res += 4; val >>= 4; }
	if (val & 0x0000000C) { res += 2; val >>= 2; }
	if (val & 0x00000002) { res += 1; val >>= 1; }

	return res;
}

AppleSparseimage::AppleSparseimage()
{
	m_drive_size = 0;
	m_current_node_offset = 0;
	m_file_size = 0;
	m_next_free_band = 0;
	m_next_index_node_nr = 0;
	m_band_size = 0;
	m_band_size_shift = 0;
	m_fd = -1;
	m_writable = false;
}

AppleSparseimage::~AppleSparseimage()
{
	Close();
}

int AppleSparseimage::Create(const char* name, uint64_t size)
{
	Close();

	m_writable = true;

	m_fd = open(name, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (m_fd < 0)
		return errno;

	m_hdr.signature = SPRS_SIGNATURE;
	m_hdr.version = 3;
	m_hdr.sectors_per_band = BAND_SIZE / SECTOR_SIZE;
	m_hdr.flags = 1; // ?
	m_hdr.next_index_node_offset = 0;
	m_hdr.total_sectors = size / SECTOR_SIZE;
	m_hdr.total_sectors_low = m_hdr.total_sectors & 0xFFFFFFFF;
	for (size_t n = 0; n < 0x3F0; n++)
		m_hdr.band_id[n] = 0;

	m_drive_size = m_hdr.total_sectors * SECTOR_SIZE;
	m_current_node_offset = 0;
	m_file_size = 0x1000;
	m_next_free_band = 0;
	m_next_index_node_nr = 0;
	m_band_size = BAND_SIZE;
	m_band_size_shift = 20; // TODO: ILog2

	m_band_offset.clear();
	m_band_offset.resize((m_drive_size + m_band_size - 1) / m_band_size, 0);

	return 0;
}

int AppleSparseimage::Open(const char* name, bool writable)
{
	size_t n;
	uint64_t offset;
	uint64_t node_offset;

	m_writable = writable;

	if (writable)
		m_fd = open(name, O_RDWR);
	else
		m_fd = open(name, O_RDONLY);

	if (m_fd < 0)
		return errno;

	ReadHeader(m_hdr);

	if (m_hdr.signature != SPRS_SIGNATURE) {
		Close();
		return EINVAL;
	}

	m_drive_size = m_hdr.total_sectors * SECTOR_SIZE;
	m_band_size = m_hdr.sectors_per_band * SECTOR_SIZE;
	m_band_size_shift = 20; // TODO: ILog2

	m_current_node_offset = 0;
	m_file_size = 0;
	m_next_free_band = 0;
	m_next_index_node_nr = 0;

	m_band_offset.resize((m_drive_size + m_band_size - 1) / m_band_size, 0);

	offset = NODE_SIZE;
	for (n = 0; n < 0x3F0; n++) {
		if (m_hdr.band_id[n] == 0)
			break;
		m_band_offset[m_hdr.band_id[n] - 1] = offset;
		offset += m_band_size;
	}
	m_next_free_band = n;
	node_offset = m_hdr.next_index_node_offset;

	while (node_offset) {
		m_current_node_offset = node_offset;
		ReadIndex(m_idx, node_offset);
		offset += NODE_SIZE;
		for (n = 0; n < 0x3F2; n++) {
			if (m_idx.band_id[n] == 0)
				break;
			m_band_offset[m_hdr.band_id[n] - 1] = offset;
			offset += m_band_size;
		}
		m_next_free_band = n;
		node_offset = m_idx.next_index_node_offset;
		m_next_index_node_nr = m_idx.index_node_nr + 1;
	}

	m_file_size = offset;

	return 0;
}

void AppleSparseimage::Close()
{
	if (m_writable) {
		if (m_current_node_offset)
			WriteIndex(m_idx, m_current_node_offset);
		else
			WriteHeader(m_hdr);
	}

	close(m_fd);
	m_fd = -1;
}

int AppleSparseimage::Read(void* data, size_t size, uint64_t offset)
{
	uint64_t band_offset;
	uint32_t band_id;
	uint32_t offset_in_band;
	uint8_t *out_data = reinterpret_cast<uint8_t *>(data);
	size_t read_size;

	if ((offset + size) > m_drive_size)
		return EINVAL;

	while (size > 0) {
		band_id = offset >> m_band_size_shift;
		offset_in_band = offset & (m_band_size - 1);
		if ((offset_in_band + size) > m_band_size)
			read_size = m_band_size - offset_in_band;
		else
			read_size = size;
		band_offset = m_band_offset[band_id];
		if (band_offset == 0) {
			memset(out_data, 0, read_size);
		} else {
			pread(m_fd, out_data, read_size, band_offset + offset_in_band);
		}

		size -= read_size;
		offset += read_size;
		out_data += read_size;
	}

	return 0;
}

int AppleSparseimage::Write(const void* data, size_t size, uint64_t offset)
{
	uint64_t band_offset;
	uint32_t band_id;
	uint32_t offset_in_band;
	const uint8_t *in_data = reinterpret_cast<const uint8_t *>(data);
	size_t write_size;

	if ((offset + size) > m_drive_size)
		return EINVAL;

	if (!m_writable)
		return EACCES;

	while (size > 0) {
		band_id = offset >> m_band_size_shift;
		offset_in_band = offset & (m_band_size - 1);
		if ((offset_in_band + size) > m_band_size)
			write_size = m_band_size - offset_in_band;
		else
			write_size = size;
		band_offset = m_band_offset[band_id];
		if (band_offset == 0)
			band_offset = AllocBand(band_id);
		pwrite(m_fd, in_data, write_size, band_offset + offset_in_band);

		size -= write_size;
		offset += write_size;
		in_data += write_size;
	}

	return 0;
}

void AppleSparseimage::ReadHeader(AppleSparseimage::HeaderNode& hdr)
{
	int k;
	HeaderNode hdr_be;

	pread(m_fd, &hdr_be, NODE_SIZE, 0);

	hdr.signature = be32toh(hdr_be.signature);
	hdr.version = be32toh(hdr_be.version);
	hdr.sectors_per_band = be32toh(hdr_be.sectors_per_band);
	hdr.flags = be32toh(hdr_be.flags);
	hdr.total_sectors_low = be32toh(hdr_be.total_sectors_low);
	hdr.next_index_node_offset = be64toh(hdr_be.next_index_node_offset);
	hdr.total_sectors = be64toh(hdr_be.total_sectors);
	std::fill(std::begin(hdr.pad), std::end(hdr.pad), 0);

	for (k = 0; k < 0x3F0; k++)
		hdr.band_id[k] = be32toh(hdr_be.band_id[k]);
}

void AppleSparseimage::WriteHeader(const AppleSparseimage::HeaderNode& hdr)
{
	int k;
	HeaderNode hdr_be;

	hdr_be.signature = htobe32(hdr.signature);
	hdr_be.version = htobe32(hdr.version);
	hdr_be.sectors_per_band = htobe32(hdr.sectors_per_band);
	hdr_be.flags = htobe32(hdr.flags);
	hdr_be.total_sectors_low = htobe32(hdr.total_sectors_low);
	hdr_be.next_index_node_offset = htobe64(hdr.next_index_node_offset);
	hdr_be.total_sectors = htobe64(hdr.total_sectors);
	std::fill(std::begin(hdr_be.pad), std::end(hdr_be.pad), 0);

	for (k = 0; k < 0x3F0; k++)
		hdr_be.band_id[k] = htobe32(hdr.band_id[k]);

	pwrite(m_fd, &hdr_be, NODE_SIZE, 0);
}

void AppleSparseimage::ReadIndex(AppleSparseimage::IndexNode& idx, uint64_t offset)
{
	int k;
	IndexNode idx_be;

	pread(m_fd, &idx_be, NODE_SIZE, offset);

	idx.signature = be32toh(idx_be.signature);
	idx.index_node_nr = be32toh(idx_be.index_node_nr);
	idx.flags = be32toh(idx_be.flags);
	idx.next_index_node_offset = be64toh(idx_be.next_index_node_offset);
	std::fill(std::begin(idx.pad), std::end(idx.pad), 0);

	for (k = 0; k < 0x3F2; k++)
		idx.band_id[k] = be32toh(idx_be.band_id[k]);
}

void AppleSparseimage::WriteIndex(const AppleSparseimage::IndexNode& idx, uint64_t offset)
{
	int k;
	IndexNode idx_be;

	idx_be.signature = htobe32(idx.signature);
	idx_be.index_node_nr = htobe32(idx.index_node_nr);
	idx_be.flags = htobe32(idx.flags);
	idx_be.next_index_node_offset = htobe64(idx.next_index_node_offset);
	std::fill(std::begin(idx_be.pad), std::end(idx_be.pad), 0);

	for (k = 0; k < 0x3F2; k++)
		idx_be.band_id[k] = htobe32(idx.band_id[k]);

	pwrite(m_fd, &idx_be, NODE_SIZE, offset);
}

uint64_t AppleSparseimage::AllocBand(size_t band_id)
{
	uint64_t off;

	if ((m_current_node_offset == 0 && m_next_free_band >= 0x3F0) || (m_current_node_offset != 0 && m_next_free_band >= 0x3F2)) {
		if (m_current_node_offset == 0) {
			m_hdr.next_index_node_offset = m_file_size;
			WriteHeader(m_hdr);
		} else {
			m_idx.next_index_node_offset = m_file_size;
			WriteIndex(m_idx, m_current_node_offset);
		}

		m_current_node_offset = m_file_size;
		m_idx.signature = SPRS_SIGNATURE;
		m_idx.index_node_nr = m_next_index_node_nr++;
		m_idx.flags = 1;
		m_idx.next_index_node_offset = 0;
		for (size_t n = 0; n < 0x3F2; n++)
			m_idx.band_id[n] = 0;
		m_next_free_band = 0;
		m_file_size += NODE_SIZE;
	}

	off = m_file_size;
	m_file_size += m_band_size;
	ftruncate(m_fd, m_file_size);
	if (m_current_node_offset == 0)
		m_hdr.band_id[m_next_free_band++] = band_id + 1;
	else
		m_idx.band_id[m_next_free_band++] = band_id + 1;
	m_band_offset[band_id] = off;
	return off;
}
