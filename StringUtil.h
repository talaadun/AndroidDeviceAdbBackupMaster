#pragma once

class CStringUtil
{
public:

	static void SplitString(const CString& str, LPCTSTR strSplit, CStringArray& arr);
	static CString GetDisplayStrForNumber(__int64 ullNumber);
	static CString GetSizeStrInKB(__int64 nSize);

	static CString GetTimeString(ULONGLONG ullTimeInMS);

	static CString GetBackupDstFullPath(const CString& strBackupTo, const CString& strSrcFullPath);
};

