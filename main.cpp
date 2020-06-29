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

#include "AppleSparseimage.h"
#include "DeviceLinux.h"
#include "GptPartitionMap.h"
#include "Apfs.h"

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

	if (!bdev.Open("/dev/sdd"))
		return -1;

	pmap.LoadAndVerify(bdev);
	pmap.ListEntries();

	sprs.Create("test.sparseimage", bdev.GetSize());

	printf("Copying GPT\n");
	pmap.CopyGPT(bdev, sprs);

	pt = 0;
	for (;;) {
		pmap.GetPartitionEntry(pt, pe);
		if (pe.StartingLBA == 0 || pe.EndingLBA == 0) break;
		start = pe.StartingLBA * bdev.GetSectorSize();
		end = (pe.EndingLBA + 1) * bdev.GetSectorSize();

		printf("Copying partition %d: %" PRIX64 " - %" PRIX64 " ", pt, pe.StartingLBA, pe.EndingLBA);
		if (!memcmp(pe.PartitionTypeGUID, GptPartitionMap::PTYPE_EFI_SYS, sizeof(GptPartitionMap::PM_GUID))) {
			printf("[EFI SYSTEM]\n");
			CopyRaw(bdev, sprs, start, end);
		} else if (!memcmp(pe.PartitionTypeGUID, GptPartitionMap::PTYPE_APFS, sizeof(GptPartitionMap::PM_GUID))) {
			printf("[APFS]\n");
			Apfs apfs(bdev, start);
			err = apfs.CopyData(sprs);
			if (err) perror("APFS err: ");
		} else {
			printf("[Unknown, skipping]\n");
		}

		pt++;
	}

	sprs.Close();
	bdev.Close();

	return 0;
}
