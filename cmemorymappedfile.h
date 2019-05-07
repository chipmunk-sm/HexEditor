/* Copyright (C) 2019 chipmunk-sm <dannico@linuxmail.org> */

#ifndef CMEMORYMAPPEDFILE_H
#define CMEMORYMAPPEDFILE_H

#include <cstdio>
#include <string>

class CMemoryMappedFile
{
public:

    typedef enum tag_CacheAccess {
        CacheAccess_Normal = 0,
        CacheAccess_Sequential,
        CacheAccess_Random
    }CacheAccess;

    CMemoryMappedFile();
    ~CMemoryMappedFile();
    void Close();
    static int64_t GetSysPageSize();
    bool OpenMapped(const std::wstring& filename, CacheAccess hint);
    int64_t GetFileSize() const;
    int64_t GetMappedBytes() const;
    uint8_t* GetDataPtr(int64_t pos, int64_t* pageSize);

private:
    bool RemapFile(int64_t offset);
#ifdef _MSC_VER
    void* m_file;
    void* m_mappedFile = nullptr;
#else
    int           m_file = -1;
#endif
    std::wstring  m_filename;
    int64_t       m_sysPageSize = 0;
    CacheAccess   m_hint = CacheAccess::CacheAccess_Normal;
    int64_t       m_mappedBytes = 0;
    void* m_mappedView = nullptr;


};

#endif // CMEMORYMAPPEDFILE_H
