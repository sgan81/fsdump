#pragma once

#include <cstddef>

#include "FileSystem.h"

class Apfs : public FileSystem
{
public:
	Apfs(Device &src, uint64_t offset);
	~Apfs();

	uint64_t GetOccupiedSize() const override;
	int CopyData(Device & src, Device & dst) override;

private:


	int ReadBlock(uint64_t paddr, void *data, size_t size = 0x1000);
	int WriteBlock(Device &dev, uint64_t paddr, void *data, size_t size = 0x1000);

	static bool VerifyBlock(const void *data, size_t size);
	static uint64_t Fletcher64(const uint32_t *data, size_t cnt, uint64_t init);

	Device &m_srcdev;
	const uint64_t m_offset;
};
