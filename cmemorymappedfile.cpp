/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */


#ifdef _MSC_VER

#   include <Windows.h>

#else

#ifndef _LARGEFILE64_SOURCE
#   define _LARGEFILE64_SOURCE
#endif

#ifdef  _FILE_OFFSET_BITS
#   undef  _FILE_OFFSET_BITS
#endif

#define _FILE_OFFSET_BITS 64

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#endif

#include <locale>
#include <codecvt>

#include "cmemorymappedfile.h"

CMemoryMappedFile::CMemoryMappedFile()
{
    m_sysPageSize = CMemoryMappedFile::GetSysPageSize();
}

CMemoryMappedFile::~CMemoryMappedFile()
{
    Close();
}

void CMemoryMappedFile::Close()
{
    if (m_mappedView)
    {
#ifdef _MSC_VER
        ::UnmapViewOfFile(m_mappedView);
#else
        ::munmap(m_mappedView, 0);
#endif
        m_mappedView = nullptr;
    }

#ifdef _MSC_VER
    if (m_mappedFile)
    {
        ::CloseHandle(m_mappedFile);
        m_mappedFile = nullptr;
    }
    if (m_file != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
#else
    if (m_file != -1)
    {
        ::close(m_file);
        m_file = -1;
    }
#endif
}

int64_t CMemoryMappedFile::GetSysPageSize()
{
    int64_t sysPageSize;
#ifdef _MSC_VER
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    sysPageSize = sysInfo.dwAllocationGranularity;
#else
    sysPageSize = sysconf(_SC_PAGESIZE);
#endif

    if(!sysPageSize)
        sysPageSize = 4096;

    return sysPageSize;
}

bool CMemoryMappedFile::OpenMapped(const std::wstring& filename, CacheAccess hint)
{

    if (m_mappedView != nullptr)
        return false;

    m_hint = hint;

#ifdef _MSC_VER
    DWORD nHint;
    switch (m_hint)
    {
        case CacheAccess_Normal:     nHint = FILE_ATTRIBUTE_NORMAL;     break;
        case CacheAccess_Sequential: nHint = FILE_FLAG_SEQUENTIAL_SCAN; break;
        case CacheAccess_Random:     nHint = FILE_FLAG_RANDOM_ACCESS;   break;
    }

    m_file = ::CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, nHint, nullptr);
    if (m_file == INVALID_HANDLE_VALUE)
        return false;

    m_mappedFile = ::CreateFileMapping(m_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!m_mappedFile)
        return false;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    std::string utf8FilePath = convert.to_bytes(filename);
    m_file = ::open(utf8FilePath.c_str(), O_RDONLY | O_LARGEFILE);
    if (m_file == -1)
        return false;
#endif

    RemapFile(0);

    if (!m_mappedView)
        return false;

    return true;
}

char* CMemoryMappedFile::GetDataPtr(int64_t pos, int64_t *pageSize)
{
    if(!RemapFile(pos))
    {
        *pageSize = 0;
        return nullptr;
    }

    *pageSize = m_mappedBytes;

    return static_cast<char*>(m_mappedView);
}

int64_t CMemoryMappedFile::GetFileSize() const
{
#ifdef _MSC_VER
    LARGE_INTEGER result;
    if (!GetFileSizeEx(m_file, &result))
        return false;
    return result.QuadPart;
#else
    struct stat64 statInfo;
    if (fstat64(m_file, &statInfo) < 0)
        return false;
    return statInfo.st_size;
#endif
}

int64_t CMemoryMappedFile::GetMappedBytes() const
{
    return m_mappedBytes;
}

bool CMemoryMappedFile::RemapFile(int64_t offset)
{
#ifdef _MSC_VER
    if (m_file == INVALID_HANDLE_VALUE)
        return false;
#else
    if (m_file == -1)
        return false;
#endif

    if (m_mappedView)
    {
#ifdef _MSC_VER
        ::UnmapViewOfFile(m_mappedView);
#else
        ::munmap(m_mappedView, static_cast<uint64_t>(m_mappedBytes));
#endif
        m_mappedView = nullptr;
        m_mappedBytes = 0;
    }

    auto fileSize = GetFileSize();
    if (offset > fileSize)
        return false;

    m_mappedBytes = (offset + m_sysPageSize) > fileSize ? fileSize - offset : m_sysPageSize;

#ifdef _MSC_VER
    m_mappedView = ::MapViewOfFile(m_mappedFile, FILE_MAP_READ, DWORD(offset >> 32), DWORD(offset & 0xFFFFFFFF), m_mappedBytes);
    if (m_mappedView == nullptr)
    {
        m_mappedBytes = 0;
        m_mappedView  = nullptr;
        return false;
    }
#else
    m_mappedView = ::mmap64(nullptr, static_cast<uint64_t>(m_mappedBytes), PROT_READ, MAP_SHARED, m_file, offset);
    if (m_mappedView == MAP_FAILED)
    {
        m_mappedBytes = 0;
        m_mappedView  = nullptr;
        return false;
    }

    int nHint;
    switch (m_hint)
    {
        case CacheAccess_Normal:     nHint = MADV_NORMAL;     break;
        case CacheAccess_Sequential: nHint = MADV_SEQUENTIAL; break;
        case CacheAccess_Random:     nHint = MADV_RANDOM;     break;
    }
    ::madvise(m_mappedView, static_cast<uint64_t>(m_mappedBytes), nHint);
#endif
    return true;
}
