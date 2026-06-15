#include "pch.h"
#include "StringUtil.h"

void CStringUtil::SplitString(const CString& str, LPCTSTR strSplit, CStringArray& arr)
{
    arr.RemoveAll(); // 清空结果

    int pos = 0;
    CString token = str.Tokenize(strSplit, pos);

    while (!token.IsEmpty())
    {
        arr.Add(token);
        token = str.Tokenize(strSplit, pos);
    }
}

CString CStringUtil::GetDisplayStrForNumber(__int64 ullNumber)
{
    CString strNum;
    strNum.Format(_T("%I64u"), ullNumber);

    int len = strNum.GetLength();
    for (int i = len - 3; i > 0; i -= 3)
    {
        strNum.Insert(i, _T(','));
    }

    return strNum;
}

CString CStringUtil::GetSizeStrInKB(__int64 nSize)
{
    CString strSize;

    ULONGLONG nKB = nSize / 1024 + (nSize % 1024 == 0 ? 0 : 1);
    strSize.Format(_T("%I64u"), nKB);

    int len = strSize.GetLength();
    for (int i = len - 3; i > 0; i -= 3)
    {
        strSize.Insert(i, _T(','));
    }

    return strSize + _T(" KB");
}

CString CStringUtil::GetTimeString(ULONGLONG ullTimeInMS)
{
    DWORD dwTotalSec = static_cast<DWORD>(ceil(static_cast<double>(ullTimeInMS) / 1000.0));

    int nHour = dwTotalSec / 3600;
    int nMinute = (dwTotalSec % 3600) / 60;
    int nSecond = dwTotalSec % 60;

    CString strTime;
    strTime.Format(_T("%d:%02d:%02d"), nHour, nMinute, nSecond);

    return strTime;
}

CString CStringUtil::GetBackupDstFullPath(const CString& strBackupTo, const CString& strSrcFullPath)
{
    CString strRelPath;
    if (strSrcFullPath.GetAt(0) == _T('/'))
    {
        strRelPath = strSrcFullPath.Right(strSrcFullPath.GetLength() - 1);
        strRelPath.Replace(_T("/"), _T("\\"));
    }
    else
    {
        strRelPath = strSrcFullPath;
        strRelPath.Replace(_T("/"), _T("\\"));
    }

    if (strBackupTo.GetAt(strBackupTo.GetLength() - 1) == _T('\\'))
        return strBackupTo + strRelPath;
    else
        return strBackupTo + _T("\\") + strRelPath;
}
