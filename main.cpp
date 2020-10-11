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

#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cerrno>

#include "AppleSparseimage.h"
#include "DeviceLinux.h"
#include "GptPartitionMap.h"
#include "Apfs.h"
#include "Ntfs.h"

void DumpHex(const void *vdata, uint32_t size)
{
	const uint8_t *data = reinterpret_cast<const uint8_t *>(vdata);
	uint32_t y;
	uint32_t x;
	constexpr uint32_t linesize = 32;

	for (y = 0; y < size; y += linesize) {
		printf("%08X: ", y);
		for (x = 0; x < linesize && (x + y) < size; x++)
			printf("%02X ", data[y + x]);
		for (; x < linesize; x++)
			printf("   ");
		printf(": ");
		for (x = 0; x < linesize && (x + y) < size; x++) {
			if (data[y + x] >= 0x20 && data[y + x] < 0x7F)
				printf("%c", data[y + x]);
			else
				printf(".");
		}
		printf("\n");
	}
}

int CopyRaw(Device &src, Device &dst, uint64_t start_off, uint64_t end_off)
{
	uint8_t buf[0x1000];
	uint64_t off;
	size_t size;
	int err;

	off = start_off;

	while (off < end_off) {
		size = ((end_off - off) > 0x1000) ? 0x1000 : end_off - off;

		err = src.Read(buf, size, off);
		if (err) return err;
		err = dst.Write(buf, size, off);
		if (err) return err;

		off += size;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	AppleSparseimage sprs;
	DeviceLinux bdev;
	GptPartitionMap pmap;
	uint64_t start;
	uint64_t end;
	int pt;
	int err;
	GptPartitionMap::PMAP_Entry pe;
	uint8_t test[0x1000];

	if (argc < 3) {
		printf("Syntax: fsdump <srcdevice> <dstfile>\n");
		printf("srcdevice: Block device (whole disk, for example /dev/sda\n");
		printf("dstfile: Image file to be written, for example image.sparseimage\n");
		return EINVAL;
	}

	if (!bdev.Open(argv[1]))
	{
		fprintf(stderr, "Unable to open device %s\n", argv[1]);
		return ENOENT;
	}

	if (!pmap.LoadAndVerify(bdev)) {
		fprintf(stderr, "Partition table invalid.\n");
		return EINVAL;
	}
	pmap.ListEntries();

	err = sprs.Create(argv[2], bdev.GetSize());
	if (err) {
		perror("Error creating image file: ");
		return err;
	}

	printf("Copying GPT\n");
	pmap.CopyGPT(bdev, sprs);

	pt = 0;
	for (;;) {
		pmap.GetPartitionEntry(pt, pe);
		if (pe.StartingLBA == 0 || pe.EndingLBA == 0) break;
		start = pe.StartingLBA * bdev.GetSectorSize();
		end = (pe.EndingLBA + 1) * bdev.GetSectorSize();

		printf("Copying partition %d: %" PRIX64 " - %" PRIX64 " ", pt, pe.StartingLBA, pe.EndingLBA);
		bdev.Read(test, bdev.GetSectorSize(), start);
		if (!memcmp(test + 32, "NXSB", 4)) {
			printf("[APFS]\n");
			Apfs apfs(bdev, start);
			err = apfs.CopyData(sprs);
			if (err) perror("APFS err: ");
		} else if (!memcmp(test + 3, "MSDOS5.0", 8)) {
			printf("[FAT]\n");
			CopyRaw(bdev, sprs, start, end);
		} else if (!memcmp(test + 3, "NTFS    ", 8)) {
			printf("[NTFS]\n");
			Ntfs ntfs(bdev, start);
			err = ntfs.CopyData(sprs);
		} else {
			printf("[UNKNOWN, skipping]\n");
		}

#if 0
		if (true) {
			DumpHex(test, bdev.GetSectorSize());
		}
#endif

		pt++;
	}

	sprs.Close();
	bdev.Close();

	return 0;
}
