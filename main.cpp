#include "AppleSparseimage.h"
#include "DeviceLinux.h"
#include "GptPartitionMap.h"

int main(int argc, char *argv[])
{
	AppleSparseimage sprs;
	DeviceLinux bdev;
	GptPartitionMap pmap;

	char testdata[0x1000] = { 'T', 'E', 'S', 'T' };

	if (!bdev.Open("/dev/sdd"))
		return -1;

	pmap.LoadAndVerify(bdev);
	pmap.ListEntries();

	sprs.Create("test.sparseimage", bdev.GetSize());

	pmap.CopyGPT(bdev, sprs);

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
