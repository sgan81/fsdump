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

#if defined(__linux__) || defined(__APPLE__)

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#ifdef __APPLE__
#include <sys/disk.h>
#endif
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <cinttypes>

#include "DeviceLinux.h"

DeviceLinux::DeviceLinux()
{
	m_device = -1;
	m_size = 0;
}

DeviceLinux::~DeviceLinux()
{
	Close();
}

bool DeviceLinux::Open(const char* name)
{
#ifdef __linux__
	m_device = open(name, O_RDONLY | O_LARGEFILE);
#endif
#ifdef __APPLE__
	m_device = open(name, O_RDONLY);
#endif

	if (m_device < 0) {
		perror("Error opening device: ");
		return false;
	}

	struct stat64 st;

	fstat64(m_device, &st);

	if (S_ISREG(st.st_mode)) {
		m_size = st.st_size;
	} else if (S_ISBLK(st.st_mode)) {
		// Hmmm ...
#ifdef __linux__
		ioctl(m_device, BLKGETSIZE64, &m_size);
#endif
#ifdef __APPLE__
		uint64_t sector_count = 0;
		uint32_t sector_size = 0;

		ioctl(m_device, DKIOCGETBLOCKCOUNT, &sector_count);
		ioctl(m_device, DKIOCGETBLOCKSIZE, &sector_size);

		m_size = sector_size * sector_count;

		printf("sector size  = %d\n", sector_size);
		printf("sector count = %" PRIu64 "\n", sector_count);
#endif
	} else {
		fprintf(stderr, "I don't know what to do with this kind of file ...\n");
	}

	SetPartitionLimits(0, m_size);

	return m_device >= 0;
}

void DeviceLinux::Close()
{
	if (m_device >= 0)
		close(m_device);
	m_device = -1;
	m_size = 0;
}

int DeviceLinux::Read(void* data, size_t size, uint64_t offset)
{
	ssize_t nread;
	uint8_t *pdata = reinterpret_cast<uint8_t *>(data);

	offset += GetPartitionStart();

	while (size > 0) {
#ifdef __linux__
		nread = pread64(m_device, pdata, size, offset);
#endif
#ifdef __APPLE__
		nread = pread(m_device, pdata, size, offset);
#endif
		if (nread < 0) return errno;
		size -= nread;
		offset += nread;
		pdata += nread;
	}

	return 0;
}

int DeviceLinux::Write(const void *data, size_t size, uint64_t offset)
{
	return ENOTSUP;
}

#endif
