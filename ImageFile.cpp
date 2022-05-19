#include <cassert>

#include "ImageFile.h"

#ifdef _WIN32

ImageFile::ImageFile()
{
	m_handle = INVALID_HANDLE_VALUE;
	m_size = 0;
}

ImageFile::~ImageFile()
{
	Close();
}

int ImageFile::Open(const char* name, bool write)
{
	wchar_t wname[MAX_PATH];
	DWORD access;
	LARGE_INTEGER fsize;

	MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_PATH);

	access = write ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;

	m_handle = CreateFileW(wname, access, FILE_SHARE_READ, NULL, write ? OPEN_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_handle == INVALID_HANDLE_VALUE)
		return GetLastError();

	GetFileSizeEx(m_handle, &fsize);
	m_size = fsize.QuadPart;

	return 0;
}

void ImageFile::Close()
{
	if (m_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
		m_size = 0;
	}
}

int ImageFile::Read(void* buffer, size_t size, uint64_t offset)
{
	LARGE_INTEGER foff = {};
	DWORD nread;
	BOOL rc;

	foff.QuadPart = offset;
	rc = SetFilePointerEx(m_handle, foff, NULL, FILE_BEGIN);
	assert(rc);
	if (!rc) return GetLastError();

	rc = ReadFile(m_handle, buffer, size, &nread, NULL);
	assert(rc);
	if (!rc) return GetLastError();
	assert(nread == size);

	return 0;
}

int ImageFile::Write(const void* buffer, size_t size, uint64_t offset)
{
	LARGE_INTEGER foff = {};
	DWORD nwritten;
	BOOL rc;

	foff.QuadPart = offset;
	rc = SetFilePointerEx(m_handle, foff, NULL, FILE_BEGIN);
	assert(rc);
	if (!rc) return GetLastError();

	rc = WriteFile(m_handle, buffer, size, &nwritten, NULL);
	assert(rc);
	if (!rc) return GetLastError();
	assert(nwritten == size);

	if ((offset + size) > m_size)
		m_size = offset + size;

	return 0;
}

int ImageFile::Resize(uint64_t size)
{
	LARGE_INTEGER foff = {};
	BOOL rc;

	foff.QuadPart = size;
	rc = SetFilePointerEx(m_handle, foff, NULL, FILE_BEGIN);
	assert(rc);
	if (!rc) return GetLastError();

	rc = SetEndOfFile(m_handle);
	assert(rc);
	if (!rc) return GetLastError();

	m_size = size;

	return 0;
}

int ImageFile::Flush()
{
	BOOL rc;
	rc = FlushFileBuffers(m_handle);
	assert(rc);
	if (!rc) return GetLastError();
	return 0;
}

#endif
