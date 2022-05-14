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
#include "DeviceWindows.h"
#include "GptPartitionMap.h"
#include "MasterBootRecord.h"
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

int CopyRaw(Device &src, Device &dst)
{
	uint8_t buf[0x1000];
	uint64_t off;
	const size_t size = src.GetPartitionSize();
	size_t bsize;
	int err;

	off = 0;

	while (off < size) {
		bsize = 0x1000;
		if ((size - off) < 0x1000)
			bsize = size - off;

		err = src.Read(buf, bsize, off);
		if (err) return err;
		err = dst.Write(buf, bsize, off);
		if (err) return err;

		off += bsize;
	}

	return 0;
}

int CopyPartition(Device &src, Device &dst)
{
	int err;
	uint8_t test[0x1000];

	src.Read(test, src.GetSectorSize(), 0);
	printf("\n");
	DumpHex(test, 0x200);

	if (!memcmp(test + 32, "NXSB", 4)) {
		printf("[APFS]\n");
		Apfs apfs(src);
		err = apfs.CopyData(dst);
		if (err) perror("APFS err: ");
	} else if (!memcmp(test + 3, "MSDOS5.0", 8) || !memcmp(test + 3, "BSD  4.4", 8)) {
		printf("[FAT]\n");
		// CopyRaw(bdev, sprs, start, end);
		err = CopyRaw(src, dst);
	} else if (!memcmp(test + 3, "NTFS    ", 8)) {
		printf("[NTFS]\n");
		Ntfs ntfs(src);
		err = ntfs.CopyData(dst);
	} else {
		printf("[UNKNOWN, skipping]\n");
		err = 0;
	}
	return err;
}

int main(int argc, char *argv[])
{
	AppleSparseimage sprs;
#ifdef WIN32
	DeviceWindows bdev;
#else
	DeviceLinux bdev;
#endif
	GptPartitionMap pmap;
	uint64_t start;
	uint64_t end;
	uint64_t size;
	int pt;
	int err;
	GptPartitionMap::PMAP_Entry pe;

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

	err = sprs.Create(argv[2], bdev.GetSize());
	if (err) {
		perror("Error creating image file: ");
		return err;
	}

	if (pmap.LoadAndVerify(bdev)) {
		fprintf(stderr, "Found GPT partition table.\n");
		pmap.ListEntries();

		printf("Copying GPT\n");
		pmap.CopyGPT(bdev, sprs);

		pt = 0;
		for (;;) {
			pmap.GetPartitionEntry(pt, pe);
			if (pe.StartingLBA == 0 || pe.EndingLBA == 0) break;
			start = pe.StartingLBA * bdev.GetSectorSize();
			end = (pe.EndingLBA + 1) * bdev.GetSectorSize();

			bdev.SetPartitionLimits(start, end - start);
			sprs.SetPartitionLimits(start, end - start);

			printf("Copying partition %d: %" PRIX64 " - %" PRIX64 " ", pt, pe.StartingLBA, pe.EndingLBA);
			CopyPartition(bdev, sprs);

			bdev.ResetPartitionLimits();
			sprs.ResetPartitionLimits();

			pt++;
		}
	} else {
		MasterBootRecord mbr;

		bdev.Read(&mbr, sizeof(mbr), 0);
		if (mbr.signature[0] == 0x55 && mbr.signature[1] == 0xAA) {
			printf("MBR partition table detected.\n");

			sprs.Write(&mbr, sizeof(mbr), 0);

			for (pt = 0; pt < 4; pt++) {
				printf("Partition %d: %u - %u ... ", pt, mbr.partition[pt].lba_start, mbr.partition[pt].lba_size);
				start = static_cast<uint64_t>(mbr.partition[pt].lba_start) * bdev.GetSectorSize();
				size = static_cast<uint64_t>(mbr.partition[pt].lba_size) * bdev.GetSectorSize();
				if (size > 0) {
					bdev.SetPartitionLimits(start, size);
					sprs.SetPartitionLimits(start, size);
					CopyPartition(bdev, sprs);
					bdev.ResetPartitionLimits();
					sprs.ResetPartitionLimits();
				} else {
					printf("\n");
				}
			}
		} else {
			printf("No supported partition table found, trying raw copy ... ");
			CopyPartition(bdev, sprs);
		}
	}

	sprs.Close();
	bdev.Close();

	return 0;
}
