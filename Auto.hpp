#pragma once

struct AutoCloseHandle
{
    HANDLE m_hFile;
    AutoCloseHandle(HANDLE hFile) : m_hFile(hFile)
    {
    }
    ~AutoCloseHandle()
    {
        CloseHandle(m_hFile);
    }
    operator HANDLE()
    {
        return m_hFile;
    }
};
