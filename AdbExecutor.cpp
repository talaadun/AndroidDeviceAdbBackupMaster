#include "pch.h"
#include "AdbExecutor.h"
#include "StringUtil.h"
#include <algorithm>


bool CAdbExecutor::GetPathInfo(
    CString& strPath,
    bool& bExist,
    bool& bIsDir,
    ULONGLONG& ullSize,
    CString& strModifyTime,
    CString& strError,
    DWORD& dwExitCode)
{
    bExist = false;

    CString strOutput;

    CString strCmdLine;
    strCmdLine.Format(_T("adb shell ls -ld \\\"%s\\\""), (LPCTSTR)strPath);

    bool bRet = CAdbExecutor::ExecuteAdbCommand(
        NULL,
        strCmdLine,
        strOutput,
        strError,
        dwExitCode);

    if (!bRet)
    {
        // check if strError contains: strPath: No such file or directory
        CString strNotExist;
        strNotExist.Format(_T("%s: No such file or directory"), (LPCTSTR)strPath);
        if (strError.Find(strNotExist) != -1)
        {
            // strPath not exist, return true
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        CStringArray strArray;
        CStringUtil::SplitString(strOutput, _T(" "), strArray);
        if (strArray.GetCount() < 8)
        {
            // ??? take it as strPath not exist, and return true
            return true;
        }


        if (strArray[0][0] == _T('d'))
        {
            bExist = true;
            bIsDir = true;
        }
        else if (strArray[0][0] == _T('-'))
        {
            bExist = true;
            bIsDir = false;
        }
        else
        {
            // ??? take it as strPath not exist, and return true
        }

        ullSize = _wtoi64(strArray[4]);
        strModifyTime = CString(strArray[5]) + _T(" ") +strArray[6];

        return true;
    }
}

bool CAdbExecutor::GetAllFilesInFolder(
    CString strFolder,
    std::vector<AdbFile>& vtrFiles,
    CString& strError,
    DWORD& dwExitCode)
{
    CString strOutput;

    strFolder.Replace(_T("'"), _T("'\\''"));

    CString strCmdLine;
    //strCmdLine.Format(_T("adb shell find \\\"%s\\\" -mindepth 1 -maxdepth 1 -exec ls -ld {} +"), (LPCTSTR)strFolder);
    //strCmdLine.Format(_T("adb shell sh -c 'find \"$1\" -mindepth 1 -maxdepth 1 -exec ls -ld {} +' _ \"%s\""), (LPCTSTR)strFolder);
    strCmdLine.Format(_T("adb shell \"find '%s' -mindepth 1 -maxdepth 1 -exec ls -ld {} +\""), (LPCTSTR)strFolder);
    bool bRet = CAdbExecutor::ExecuteAdbCommand(
        NULL,
        strCmdLine,
        strOutput,
        strError,
        dwExitCode);

    if (bRet)
    {
        vtrFiles.clear();
        CAdbExecutor::ParseFileList(strOutput, vtrFiles);

        std::sort(vtrFiles.begin(), vtrFiles.end(), [](const AdbFile& a, const AdbFile& b)
            {
                if (a.bIsDir != b.bIsDir)
                {
                    return a.bIsDir > b.bIsDir;
                }

                return a.strFileName.CompareNoCase(b.strFileName) < 0;
            });
    }

    return bRet;
}

bool CAdbExecutor::BackupFiles(
    std::vector<CString>& vtrAdbFiles,
    CString& strLocalFolder,
    CString& strError,
    DWORD& dwExitCode)
{
    CString strCmdLine = _T("adb pull -a ");
    for (auto& adbFile : vtrAdbFiles)
    {
        strCmdLine += _T("\"") + adbFile + _T("\" ");
    }
    strCmdLine += _T("\"") + strLocalFolder + _T("\" ");

    CString strOutput;
    bool bRet = CAdbExecutor::ExecuteAdbCommand(
        NULL,
        strCmdLine,
        strOutput,
        strError,
        dwExitCode);

    return bRet;
}

// 返回值：命令执行成功返回 true，失败返回 false
// strOutput：输出信息
// strError：错误信息（失败时才有）
// dwExitCode：退出码 0=成功
bool CAdbExecutor::ExecuteAdbCommand(
    LPCTSTR lpAdbPath,
    LPCTSTR lpCmdLine,
    CString& strOutput,
    CString& strError,
    DWORD& dwExitCode)
{
    strOutput.Empty();
    strError.Empty();
    dwExitCode = 0;

    SECURITY_ATTRIBUTES sa;
    HANDLE hReadPipeOut, hWritePipeOut; // 标准输出
    HANDLE hReadPipeErr, hWritePipeErr; // 错误输出
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // 创建输出管道
    if (!CreatePipe(&hReadPipeOut, &hWritePipeOut, &sa, 0))
        return false;

    // 创建错误管道
    if (!CreatePipe(&hReadPipeErr, &hWritePipeErr, &sa, 0))
    {
        CloseHandle(hReadPipeOut);
        CloseHandle(hWritePipeOut);
        return false;
    }

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hWritePipeOut;
    si.hStdError = hWritePipeErr;
    si.dwFlags = STARTF_USESTDHANDLES;

    TCHAR szCmd[1024] = { 0 };
    _stprintf_s(szCmd, _T("cmd.exe /c %s"), lpCmdLine);

    // 启动进程
    BOOL bRet = CreateProcess(
        NULL, szCmd, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!bRet)
    {
        CloseHandle(hReadPipeOut); CloseHandle(hWritePipeOut);
        CloseHandle(hReadPipeErr); CloseHandle(hWritePipeErr);
        return false;
    }

    // 关闭写端，否则读不到数据
    CloseHandle(hWritePipeOut);
    CloseHandle(hWritePipeErr);

    // 读取输出
    char buf[4096] = { 0 };
    DWORD read;

    while (true) {
        BOOL bSuccess = ReadFile(hReadPipeOut, buf, sizeof(buf) - 1, &read, nullptr);
        if (!bSuccess || read == 0) break;

        buf[read] = '\0';
        strOutput += CA2T(buf, CP_UTF8);

        //int nWideLen = MultiByteToWideChar(CP_UTF8, 0, buf, read, nullptr, 0);
        //if (nWideLen > 0) {
        //    CStringW strWide;
        //    MultiByteToWideChar(CP_UTF8, 0, buf, read, strWide.GetBuffer(nWideLen), nWideLen);
        //    strWide.ReleaseBuffer();
        //    strOutput += strWide;
        //}
    }

    while (true) {
        BOOL bSuccess = ReadFile(hReadPipeErr, buf, sizeof(buf) - 1, &read, nullptr);
        if (!bSuccess || read == 0) 
            break;

        buf[read] = '\0';
        strError += CA2T(buf, CP_UTF8);

        //int nWideLen = MultiByteToWideChar(CP_UTF8, 0, buf, read, nullptr, 0);
        //if (nWideLen > 0) {
        //    CStringW strWide;
        //    MultiByteToWideChar(CP_UTF8, 0, buf, read, strWide.GetBuffer(nWideLen), nWideLen);
        //    strWide.ReleaseBuffer();
        //    strError += strWide;
        //}
    }

    // 等待结束 + 获取退出码（关键！）
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    // 清理
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipeOut);
    CloseHandle(hReadPipeErr);

    // 退出码 0 = 成功
    return (dwExitCode == 0);
}

void CAdbExecutor::ParseFileList(const CString& strOutput, std::vector<AdbFile>& vtrFiles)
{  
    int nPos = 0;
    int nLen = strOutput.GetLength();
    
    // 逐行读取
    while (nPos < nLen)
    {
        // 取一行
        int nEnd = strOutput.Find(_T("\r\n"), nPos);
        if (nEnd == -1)
            nEnd = nLen;

        CString strLine = strOutput.Mid(nPos, nEnd - nPos);
        nPos = nEnd + 2;

        // 去掉换行、空格
        strLine.Trim();
        if (strLine.IsEmpty())
            continue;

        // 跳过 total 开头的行
        if (strLine.Left(5) == _T("total"))
            continue;

        AdbFile info;
        info.bIsDir = false;
        info.ullFileSize = 0;

        // ==============================================
        // 解析第 1 个字符：判断是文件还是目录
        // ==============================================
        TCHAR chType = strLine[0];
        // d=目录, -=文件, and ??
        if (chType == _T('d'))
            info.bIsDir = true;
        else if (chType == _T('-'))
            info.bIsDir = false;
        else
            continue;

        // ==============================================
        // 按空格分段（自动跳过多个空格）
        // ==============================================
        int nPos = 0;
        CString arrField[10];
        int nIdx = 0;

        while (nPos < strLine.GetLength() && nIdx < 10)
        {
            // 跳过空格
            while (nPos < strLine.GetLength() && strLine[nPos] == _T(' '))
                nPos++;

            if (nPos >= strLine.GetLength())
                break;

            // 取一段
            int nStart = nPos;
            if (nIdx < 7)
            {
                while (nPos < strLine.GetLength() && strLine[nPos] != _T(' '))
                    nPos++;

                arrField[nIdx++] = strLine.Mid(nStart, nPos - nStart);
            }
            else
            {
                // final part file name
                arrField[nIdx++] = strLine.Right(strLine.GetLength() - nStart);
                break;
            }
        }

        // 必须至少 8 段才有效
        if (nIdx < 8)
            continue;

        // ==============================================
        // 提取字段
        // ==============================================
        info.ullFileSize = _wtoi64(arrField[4]);                      // 大小
        info.strModifyTime = arrField[5] + _T(" ") + arrField[6];   // 时间
        info.strFileFullPath = arrField[7];                         // 路径
        info.strFileName = arrField[7].Right(arrField[7].GetLength() - arrField[7].ReverseFind(_T('/')) - 1);

        info.bExist = true;

        vtrFiles.push_back(info);
    }
}
