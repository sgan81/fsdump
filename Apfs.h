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

#include "FileSystem.h"

class Apfs : public FileSystem
{
public:
	Apfs(Device &src, uint64_t offset);
	~Apfs();

	uint64_t GetOccupiedSize() const override;
	int CopyData(Device & dst) override;

private:
	int CopyViaSM(Device &dst, uint64_t sm_paddr, uint32_t sm_size);
	int CopyCAB(Device &dst, uint64_t cab_paddr);
	int CopyCIB(Device &dst, uint64_t cib_paddr);

	int ReadBlock(uint64_t paddr, void *data, size_t size = 0x1000);
	int ReadVerifiedBlock(uint64_t paddr, void *data, size_t size = 0x1000);
	int WriteBlock(Device &dev, uint64_t paddr, void *data, size_t size = 0x1000);
	int CopyRange(Device &dst, uint64_t paddr, uint64_t blocks);

	static bool VerifyBlock(const void *data, size_t size);
	static uint64_t Fletcher64(const uint32_t *data, size_t cnt, uint64_t init);

	Device &m_srcdev;
	const uint64_t m_offset;
	uint8_t *m_buf;
};
