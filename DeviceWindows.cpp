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

#ifdef _WIN32

#include "DeviceWindows.h"

DeviceWindows::DeviceWindows()
{
	m_drive = INVALID_HANDLE_VALUE;
	m_size = 0;
}

DeviceWindows::~DeviceWindows()
{
	Close();
}

bool DeviceWindows::Open(const char* name)
{
	wchar_t path[MAX_PATH];
	uint8_t buf[0x1000];
	DWORD bytes_ret = 0;

	// _stprintf_s(path, _T("\\\\.\\PhysicalDrive%d"), disk);

	MultiByteToWideChar(CP_UTF8, 0, name, -1, path, MAX_PATH);

	m_drive = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, nullptr);
	if (m_drive == INVALID_HANDLE_VALUE)
		return false;

	DeviceIoControl(m_drive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &buf, sizeof(buf), &bytes_ret, nullptr);

	if (bytes_ret == 0)
		return false;

	const DISK_GEOMETRY_EX* geom = reinterpret_cast<const DISK_GEOMETRY_EX*>(buf);

	m_size = geom->DiskSize.QuadPart;

	ResetPartitionLimits();

	return true;
}

void DeviceWindows::Close()
{
	if (m_drive != INVALID_HANDLE_VALUE)
		CloseHandle(m_drive);
	m_drive = INVALID_HANDLE_VALUE;
	m_size = 0;
}

int DeviceWindows::Read(void* data, size_t size, uint64_t offset)
{
	DWORD read_bytes = 0;
	LARGE_INTEGER off;
	BOOL rc;

	if (m_drive == INVALID_HANDLE_VALUE)
		return false;

	off.QuadPart = offset + GetPartitionStart();

	SetFilePointerEx(m_drive, off, nullptr, FILE_BEGIN);
	rc = ReadFile(m_drive, data, size, &read_bytes, nullptr);

	return rc ? 0 : EFAULT;
}

int DeviceWindows::Write(const void* data, size_t size, uint64_t offset)
{
	return ENOTSUP;
}

uint64_t DeviceWindows::GetSize() const
{
	return m_size;
}

#endif
