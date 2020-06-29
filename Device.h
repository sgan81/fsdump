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

class Device
{
protected:
	Device() { m_sector_size = 0x200; }

public:
	virtual ~Device() {}

	virtual int Read(void *data, size_t size, uint64_t offset) = 0;
	virtual int Write(const void *data, size_t size, uint64_t offset) = 0;
	virtual uint64_t GetSize() const = 0;

	unsigned int GetSectorSize() const { return m_sector_size; }
	void SetSectorSize(unsigned int size) { m_sector_size = size; }

private:
	unsigned int m_sector_size;
};
