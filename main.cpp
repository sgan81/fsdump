#include <cstring>
#include <cstdio>
#include <cinttypes>

#include "AppleSparseimage.h"
#include "DeviceLinux.h"
#include "GptPartitionMap.h"

int CopyRaw(Device &src, Device &dst, uint64_t start_off, uint64_t end_off)
{
	uint8_t buf[0x1000];
	uint64_t off;
	size_t size;

	off = start_off;

	while (off < end_off) {
		size = ((end_off - off) > 0x1000) ? 0x1000 : end_off - off;

		src.Read(buf, size, off);
		dst.Write(buf, size, off);

		off += size;
	}
}

int main(int argc, char *argv[])
{
	AppleSparseimage sprs;
	DeviceLinux bdev;
	GptPartitionMap pmap;
	uint64_t start;
	uint64_t end;
	int pt;
	GptPartitionMap::PMAP_Entry pe;

	if (!bdev.Open("/dev/sdd"))
		return -1;

	pmap.LoadAndVerify(bdev);
	pmap.ListEntries();

	sprs.Create("test.sparseimage", bdev.GetSize());

	pmap.CopyGPT(bdev, sprs);

	pt = 0;
	for (;;) {
		pmap.GetPartitionEntry(pt, pe);
		if (pe.StartingLBA == 0 || pe.EndingLBA == 0) break;
		start = pe.StartingLBA * bdev.GetSectorSize();
		end = (pe.EndingLBA + 1) * bdev.GetSectorSize();

		printf("Partition: %" PRIX64 " - %" PRIX64 " ", pe.StartingLBA, pe.EndingLBA);
		if (!memcmp(pe.PartitionTypeGUID, GptPartitionMap::PTYPE_EFI_SYS, sizeof(GptPartitionMap::PM_GUID))) {
			printf("[EFI SYSTEM]\n");
			CopyRaw(bdev, sprs, start, end);
		} else if (!memcmp(pe.PartitionTypeGUID, GptPartitionMap::PTYPE_APFS, sizeof(GptPartitionMap::PM_GUID))) {
			printf("[APFS] (TODO)\n");
		} else {
			printf("[Unknown] (TODO)\n");
		}

		pt++;
	}

	sprs.Close();

	/*
	sprs.Create("test.sparseimage", 0x40000000);

	sprs.Write(testdata, 0x1000, 0);
	sprs.Write(testdata, 0x1000, 0x3FFFF000);

	sprs.Close();
	*/

	bdev.Close();

	return 0;
}
