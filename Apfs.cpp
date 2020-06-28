#include <cerrno>
#include <endian.h>

#include "Apfs.h"
#include "Device.h"



Apfs::Apfs(Device &src, uint64_t offset) : m_srcdev(src), m_offset(offset)
{
}

Apfs::~Apfs()
{
}

uint64_t Apfs::GetOccupiedSize() const
{
	return 0;
}

int Apfs::CopyData(Device& src, Device& dst)
{
	return ENOTSUP;
}

int Apfs::ReadBlock(uint64_t paddr, void* data, size_t size)
{
	uint64_t off = (paddr << 12) + m_offset; // TODO: Blocksize

	return m_srcdev.Read(data, size, off);
}

int Apfs::WriteBlock(Device& dev, uint64_t paddr, void* data, size_t size)
{
	uint64_t off = (paddr << 12) + m_offset;

	return dev.Write(data, size, off);
}

bool Apfs::VerifyBlock(const void* data, size_t size)
{
	uint64_t cs;
	const uint32_t * const bdata = reinterpret_cast<const uint32_t *>(data);

	size /= sizeof(uint32_t);

	cs = le64toh(*reinterpret_cast<const uint64_t *>(data));
	if (cs == 0)
		return false;
	if (cs == 0xFFFFFFFFFFFFFFFFULL)
		return false;

	cs = Fletcher64(bdata + 2, size - 2, 0);
	cs = Fletcher64(bdata, 2, cs);

	return cs == 0;
}

uint64_t Apfs::Fletcher64(const uint32_t *data, size_t cnt, uint64_t init)
{
	size_t k;

	uint64_t sum1 = init & 0xFFFFFFFFU;
	uint64_t sum2 = (init >> 32);

	for (k = 0; k < cnt; k++)
	{
		sum1 = (sum1 + le32toh(data[k]));
		sum2 = (sum2 + sum1);
	}

	sum1 = sum1 % 0xFFFFFFFF;
	sum2 = sum2 % 0xFFFFFFFF;

	return (static_cast<uint64_t>(sum2) << 32) | static_cast<uint64_t>(sum1);
}
