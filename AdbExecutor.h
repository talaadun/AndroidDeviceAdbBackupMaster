#pragma once

#include <vector>


typedef struct _AdbFile
{
    CString     strFileFullPath;
    CString     strFileName;
    bool        bExist;
    bool        bIsDir;
    ULONGLONG   ullFileSize;
    CString     strModifyTime;
} AdbFile;

class CAdbExecutor
{
public:
    static bool GetPathInfo(
        CString& strPath,
        bool& bExist,
        bool& bIsDir,
        ULONGLONG& ullSize,
        CString& strModifyTime,
        CString& strError,
        DWORD& dwExitCode);

    static bool GetAllFilesInFolder(
        CString strFolder,
        std::vector<AdbFile>& vtrFiles,
        CString& strError,
        DWORD& dwExitCode);

    static bool BackupFiles(
        std::vector<CString>& vtrAdbFiles,
        CString& strLocalFolder,
        CString& strError,
        DWORD& dwExitCode);

    static bool ExecuteAdbCommand(
        LPCTSTR lpAdbPath,
        LPCTSTR lpCmdLine,
        CString& strOutput,
        CString& strError,
        DWORD& dwExitCode);

private:
    static void ParseFileList(const CString& strOutput, std::vector<AdbFile>& vtrFiles);

};

