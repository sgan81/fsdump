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

#ifdef _WIN32

#include "Device.h"

// #define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>

class ImageFile
{
public:
	ImageFile();
	~ImageFile();

	int Open(const char* name, bool write);
	void Close();
	bool IsOpen() const { return m_handle != INVALID_HANDLE_VALUE; }

	int Read(void* buffer, size_t size, uint64_t offset);
	int Write(const void* buffer, size_t size, uint64_t offset);
	int Resize(uint64_t size);
	int Flush();
	uint64_t GetSize() const { return m_size; }

private:
	HANDLE m_handle;
	uint64_t m_size;
};

#endif
