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

class DeviceWindows : public Device
{
public:
	DeviceWindows();
	~DeviceWindows();

	bool Open(const char* name);
	void Close();

	int Read(void* data, size_t size, uint64_t offset) override;
	int Write(const void* data, size_t size, uint64_t offset) override;
	uint64_t GetSize() const override;

private:
	HANDLE m_drive;
	uint64_t m_size;
};

#endif
