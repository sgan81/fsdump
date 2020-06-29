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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <endian.h>

#include "Apfs.h"
#include "Device.h"
#include "apfs_layout.h"

#define dbg_printf(...) // printf(__VA_ARGS__)

static constexpr size_t BUF_SIZE = 0x400000;

Apfs::Apfs(Device &src, uint64_t offset) : m_srcdev(src), m_offset(offset)
{
	m_buf = new uint8_t[BUF_SIZE];
}

Apfs::~Apfs()
{
	delete[] m_buf;
}

uint64_t Apfs::GetOccupiedSize() const
{
	return 0;
}

int Apfs::CopyData(Device& dst)
{
	nx_superblock_t *nxsb = nullptr;
	checkpoint_map_phys_t *cpm = nullptr;
	paddr_t base;
	xid_t max_xid = 0;
	paddr_t max_paddr;
	uint32_t size;
	uint32_t idx;
	int rc = ENOTSUP;

	nxsb = reinterpret_cast<nx_superblock_t *>(malloc(NX_DEFAULT_BLOCK_SIZE));

	rc = ReadVerifiedBlock(0, nxsb, NX_DEFAULT_BLOCK_SIZE);
	if (rc) goto error;

	if (le32toh(nxsb->nx_magic) != NX_MAGIC) goto error;
	if (le32toh(nxsb->nx_block_size) != NX_DEFAULT_BLOCK_SIZE) goto error;

	CopyRange(dst, 0, 1);
	base = le64toh(nxsb->nx_xp_desc_base);
	size = le32toh(nxsb->nx_xp_desc_blocks);
	CopyRange(dst, base, size);
	base = le64toh(nxsb->nx_xp_data_base);
	size = le32toh(nxsb->nx_xp_data_blocks);
	CopyRange(dst, base, size);

	base = le64toh(nxsb->nx_xp_desc_base);
	idx = le32toh(nxsb->nx_xp_desc_index);
	for (;;) {
		idx = idx + le32toh(nxsb->nx_xp_desc_len) - 1;
		if (idx >= le32toh(nxsb->nx_xp_desc_blocks)) idx -= le32toh(nxsb->nx_xp_desc_blocks);
		rc = ReadVerifiedBlock(base + idx, nxsb);
		if (rc) goto error;
		dbg_printf("XP srch: nxsb @ %" PRIX64 " xid %" PRId64 "\n", base, le64toh(nxsb->nx_o.o_xid));
		if (le64toh(nxsb->nx_o.o_xid) < max_xid)
			break;
		max_xid = le64toh(nxsb->nx_o.o_xid);
		max_paddr = base + idx;
		idx = le32toh(nxsb->nx_xp_desc_next);
	}

	rc = ReadVerifiedBlock(max_paddr, nxsb);
	if (rc) goto error;

	cpm = reinterpret_cast<checkpoint_map_phys_t *>(malloc(NX_DEFAULT_BLOCK_SIZE));
	idx = le32toh(nxsb->nx_xp_desc_index);
	rc = ReadVerifiedBlock(base + idx, cpm);

	for (idx = 0; idx < le32toh(cpm->cpm_count); idx++)
	{
		if ((le32toh(cpm->cpm_map[idx].cpm_type) & OBJECT_TYPE_MASK) == OBJECT_TYPE_SPACEMAN) {
			dbg_printf("SM found at %" PRIX64 "\n", le64toh(cpm->cpm_map[idx].cpm_paddr));
			// Copy Spaceman ...
			CopyViaSM(dst, le64toh(cpm->cpm_map[idx].cpm_paddr), le32toh(cpm->cpm_map[idx].cpm_size));
		}
	}

	free(cpm);
	free(nxsb);
	return 0;

error:
	free(cpm);
	free(nxsb);
	return rc;
}

int Apfs::CopyViaSM(Device& dst, uint64_t sm_paddr, uint32_t sm_size)
{
	spaceman_phys_t *sm;
	uint64_t *addrs;
	uint32_t cib_cnt;
	uint32_t cab_cnt;
	uint32_t idx;
	int rc;

	sm = reinterpret_cast<spaceman_phys_t*>(malloc(sm_size));

	rc = ReadVerifiedBlock(sm_paddr, sm, sm_size);
	if (rc) {
		free(sm);
		return rc;
	}

	if ((le32toh(sm->sm_o.o_type) & OBJECT_TYPE_MASK) != OBJECT_TYPE_SPACEMAN) {
		free(sm);
		return EINVAL;
	}

	cib_cnt = le32toh(sm->sm_dev[SD_MAIN].sm_cib_count);
	cab_cnt = le32toh(sm->sm_dev[SD_MAIN].sm_cab_count);

	dbg_printf("CAB count: %u\n", cib_cnt);
	dbg_printf("CIB count: %u\n", cab_cnt);

	addrs = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t *>(sm) + le32toh(sm->sm_dev[SD_MAIN].sm_addr_offset));

	if (cab_cnt > 0) {
		free(sm);
		return ENOTSUP;
	} else if (cib_cnt > 0) {
		for (idx = 0; idx < cib_cnt; idx++) {
			dbg_printf("CIB %d : %" PRIX64 "\n", idx, le64toh(addrs[idx]));
			CopyCIB(dst, le64toh(addrs[idx]));
		}
	}

	free(sm);
	return 0;
}



int Apfs::CopyCIB(Device& dst, uint64_t cib_paddr)
{
	uint8_t bm[NX_DEFAULT_BLOCK_SIZE];
	uint8_t cibd[NX_DEFAULT_BLOCK_SIZE];
	chunk_info_block_t * const cib = reinterpret_cast<chunk_info_block_t*>(cibd);
	int err;
	uint32_t index;

	err = ReadVerifiedBlock(cib_paddr, cib);
	if (err) return err;
	if ((le32toh(cib->cib_o.o_type) & OBJECT_TYPE_MASK) != OBJECT_TYPE_SPACEMAN_CIB)
		return EINVAL;

	dbg_printf("CIB index: %u\n", cib->cib_index);

	for (index = 0; index < le32toh(cib->cib_chunk_info_count); index++)
	{
		const chunk_info_t &ci = cib->cib_chunk_info[index];

		paddr_t addr = le64toh(ci.ci_addr);
		uint32_t block_count = le32toh(ci.ci_block_count);
		uint32_t free_count = le32toh(ci.ci_free_count);

		if (free_count == block_count) {
			continue;
		}
		else if (free_count == 0) {
			dbg_printf("  %" PRIX64 " %04X %04X %" PRIX64 "\n", addr, block_count, free_count, le64toh(ci.ci_bitmap_addr));
			CopyRange(dst, addr, block_count);
		}
		else {
			dbg_printf("  %" PRIX64 " %04X %04X %" PRIX64 "\n", addr, block_count, free_count, le64toh(ci.ci_bitmap_addr));
			uint32_t blk;
			uint64_t start;
			uint64_t end;
			int old_bit;

			ReadBlock(le64toh(ci.ci_bitmap_addr), bm);

			old_bit = 0;
			for (blk = 0; blk < block_count; blk++) {
				if ((bm[blk >> 3] >> (blk & 7)) & 1) {
					if (old_bit == 0) {
						old_bit = 1;
						start = addr + blk;
					}
				} else {
					if (old_bit == 1) {
						old_bit = 0;
						end = addr + blk;
						CopyRange(dst, start, end - start);
					}
				}
			}
			if (old_bit == 1) {
				end = addr + blk;
				CopyRange(dst, start, end - start);
			}
		}
	}

	return 0;
}


int Apfs::ReadBlock(uint64_t paddr, void* data, size_t size)
{
	uint64_t off = (paddr << 12) + m_offset; // TODO: Blocksize

	return m_srcdev.Read(data, size, off);
}

int Apfs::ReadVerifiedBlock(uint64_t paddr, void* data, size_t size)
{
	int err;

	err = ReadBlock(paddr, data, size);
	if (err) return err;
	if (!VerifyBlock(data, size)) {
		fprintf(stderr, "Block verification failed.\n");
		return EINVAL;
	}
	return 0;
}

int Apfs::WriteBlock(Device& dev, uint64_t paddr, void* data, size_t size)
{
	uint64_t off = (paddr << 12) + m_offset;

	return dev.Write(data, size, off);
}

int Apfs::CopyRange(Device& dst, uint64_t paddr, uint64_t blocks)
{
	uint64_t off;
	uint64_t size;
	size_t bsize;

	dbg_printf("CopyRange %" PRIX64 " L %" PRIX64 "\n", paddr, blocks);

	off = (paddr << 12) + m_offset;
	size = blocks << 12;

	while (size > 0) {
		bsize = size;
		if (bsize > BUF_SIZE) bsize = BUF_SIZE;
		m_srcdev.Read(m_buf, bsize, off);
		dst.Write(m_buf, bsize, off);
		off += bsize;
		size -= bsize;
	}

	return 0;
}

bool Apfs::VerifyBlock(const void* data, size_t size)
{
	uint64_t cs;
	const uint32_t * const bdata = reinterpret_cast<const uint32_t *>(data);

	size /= sizeof(uint32_t);

	cs = le64toh(*reinterpret_cast<const uint64_t *>(data));
	if (cs == 0)
		return false;
	if (cs == 0xFFFFFFFFFFFFFFFFULL)
		return false;

	cs = Fletcher64(bdata + 2, size - 2, 0);
	cs = Fletcher64(bdata, 2, cs);

	return cs == 0;
}

uint64_t Apfs::Fletcher64(const uint32_t *data, size_t cnt, uint64_t init)
{
	size_t k;

	uint64_t sum1 = init & 0xFFFFFFFFU;
	uint64_t sum2 = (init >> 32);

	for (k = 0; k < cnt; k++)
	{
		sum1 = (sum1 + le32toh(data[k]));
		sum2 = (sum2 + sum1);
	}

	sum1 = sum1 % 0xFFFFFFFF;
	sum2 = sum2 % 0xFFFFFFFF;

	return (static_cast<uint64_t>(sum2) << 32) | static_cast<uint64_t>(sum1);
}
