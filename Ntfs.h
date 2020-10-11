#pragma once

#include <cstdint>
#include <vector>

#include "FileSystem.h"

class Ntfs;

class NtfsFile
{
	struct Extent
	{
		Extent(uint64_t s, uint64_t c) { start = s; count = c; }

		uint64_t start;
		uint64_t count;
	};

public:
	NtfsFile(Ntfs &fs);
	~NtfsFile();

	bool Open(const uint8_t *mft_rec);
	void Close();
	void Read(void *data, size_t size, uint64_t offset);
	uint64_t GetSize();

private:
	Ntfs &m_fs;
	std::vector<Extent> m_exts;
	uint64_t m_size;
};

class Ntfs : public FileSystem
{
	friend class NtfsFile;
public:
	Ntfs(Device &src, uint64_t offset);
	~Ntfs();

	uint64_t GetOccupiedSize() const override;
	int CopyData(Device & dst) override;

private:
	int ReadBlock(uint64_t lcn, void *data, size_t size = 0x1000);
	int WriteBlock(Device &dev, uint64_t lcn, const void *data, size_t size = 0x1000);
	int ReadData(uint64_t pos, void *data, size_t size);
	static bool FixUpdSeqRecord(uint8_t *out, const uint8_t *in, size_t size = 0x400);

	static void DumpMFTEntry(const uint8_t *entry, uint64_t idx);

	Device &m_srcdev;
	const uint64_t m_offset;
	uint32_t m_blksize;
	uint8_t m_blk[0x1000];
};
