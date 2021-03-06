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

#include <cstddef>
#include <cstdint>

#include <vector>

#include "Device.h"

class AppleSparseimage : public Device
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
	} __attribute__((packed, aligned(4)));

	struct IndexNode {
		uint32_t signature;
		uint32_t index_node_nr;
		uint32_t flags;
		uint64_t next_index_node_offset;
		uint8_t pad[0x24];
		uint32_t band_id[0x3F2];
	} __attribute__((packed, aligned(4)));

public:
	AppleSparseimage();
	~AppleSparseimage();

	int Create(const char *name, uint64_t size);
	int Open(const char *name, bool writable);
	void Close();

	int Read(void *data, size_t size, uint64_t offset) override;
	int Write(const void *data, size_t size, uint64_t offset) override;

	uint64_t GetSize() const override { return m_drive_size; }

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

