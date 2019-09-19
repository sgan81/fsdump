#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>

class AppleSparseimage
{
	struct HeaderNode {
		uint32_t signature;
		uint32_t version;
		uint32_t sectors_per_band;
		uint32_t flags;
		uint32_t total_sectors_low;
		uint64_t next_index_node_offset;
		uint64_t total_sectors;
		uint8_t pad[0x1C];
		uint32_t band_id[0x3F0];
	} __attribute__((packed));

	struct IndexNode {
		uint32_t signature;
		uint32_t index_node_nr;
		uint32_t flags;
		uint64_t next_index_node_offset;
		uint8_t pad[0x24];
		uint32_t band_id[0x3F2];
	} __attribute__((packed));

public:
	AppleSparseimage();
	~AppleSparseimage();

	int Create(const char *name, uint64_t size);
	int Open(const char *name, bool writable);
	void Close();

	int Read(void *data, size_t size, uint64_t offset);
	int Write(const void *data, size_t size, uint64_t offset);

	uint64_t GetSize() const { return m_drive_size; }

private:
	void ReadHeader(HeaderNode &hdr);
	void WriteHeader(const HeaderNode &hdr);
	void ReadIndex(IndexNode &idx, uint64_t offset);
	void WriteIndex(const IndexNode &idx, uint64_t offset);
	uint64_t AllocBand(size_t band_id);

	std::vector<uint64_t> m_band_offset;
	uint64_t m_drive_size;
	uint64_t m_current_node_offset;
	uint64_t m_file_size;
	uint32_t m_next_free_band;
	uint32_t m_next_index_node_nr;
	uint32_t m_band_size;
	int m_band_size_shift;
	int m_fd;
	bool m_writable;

	HeaderNode m_hdr;
	IndexNode m_idx;
};

