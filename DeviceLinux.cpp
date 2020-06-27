/*
This file is part of apfs-fuse, a read-only implementation of APFS
(Apple File System) for FUSE.
Copyright (C) 2017 Simon Gander

Apfs-fuse is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Apfs-fuse is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with apfs-fuse.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __linux__

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <cstdio>
#include <cstring>

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
	m_device = open(name, O_RDONLY | O_LARGEFILE);

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
		ioctl(m_device, BLKGETSIZE64, &m_size);
	} else {
		fprintf(stderr, "I don't know what to do with this kind of file ...\n");
	}

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
	size_t nread;

	nread = pread64(m_device, data, size, offset);

	// TODO: Better error handling ...
	return nread == size;
}

int DeviceLinux::Write(const void *data, size_t size, uint64_t offset)
{
	return ENOTSUP;
}

#endif
