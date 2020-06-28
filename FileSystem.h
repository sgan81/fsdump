#pragma once

#include <cstdint>

class Device;

class FileSystem
{
protected:
	FileSystem() {}

public:
	virtual ~FileSystem() {}

	virtual uint64_t GetOccupiedSize() const = 0;
	virtual int CopyData(Device &src, Device &dst) = 0;
};
