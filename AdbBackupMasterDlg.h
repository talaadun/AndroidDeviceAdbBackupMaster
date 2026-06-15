
// AdbBackupMasterDlg.h: 头文件
//

#pragma once
#include <vector>
#include "AdbExecutor.h"

typedef struct _BkAdbItem
{
	CString     strFullPath;
	CString     strFileName;
	bool        bIsDir;
	ULONGLONG	ullSize;
	CString     strModifyTime;

	_BkAdbItem()
	{
		Reset();
	}

	void Reset()
	{
		strFullPath = _T("");
		strFileName = _T("");
		bIsDir = false;
		ullSize = 0;
		strModifyTime = _T("");
	}
} BkAdbItem;

typedef struct _BkLocalItem
{
	CString     strFullPath;
	CString     strFileName;
	bool		bExist;
	bool        bIsDir;
	ULONGLONG	ullSize;
	CString     strModifyTime;

	_BkLocalItem()
	{
		Reset();
	}

	void Reset()
	{
		strFullPath = _T("");
		strFileName = _T("");
		bExist = false;
		bIsDir = false;
		ullSize = 0;
		strModifyTime = _T("");
	}
} BkLocalItem;


enum BackupFlag
{
	// 0 is normal
	BackupFlagFileUnknown = 1,
	BackupFlagFileBackuped,
	BackupFlagFileAdd,
	BackupFlagFileUpdate,
	BackupFlagFileTypeDifferent,
	BackupFlagFileBackuping,
	BackupFlagFileError,

	BackupFlagDirNormal = 10,
	BackupFlagDirUnknown,
	BackupFlagDirTypeDifferent,
	BackupFlagDirError,
	BackupFlagDirBackuping,
};

typedef struct _BkItemPair
{
	BkAdbItem		bkiAdb;
	BkLocalItem		bkiLocal;

	BackupFlag		eBackupFlag;
	CString			strStatus;

	ULONGLONG		nAddSize;				// Dir: total add size; file: file size if add, 0 otherwise
	ULONGLONG		nUpdateSize;			// Dir: total update size; file: file size if update, 0 otherwise

	ULONGLONG		nFileBackupedCount;
	ULONGLONG		nFileAddCount;			// Dir: total add file count. File: 1 if add, 0 otherwise
	ULONGLONG		nFileUpdateCount;		// Dir: total update file count. File: 1 if update, 0 otherwise
	ULONGLONG		nFileTypeDifferentCount;
	ULONGLONG		nFileBackupErrorCount;

	ULONGLONG		nFileAddBackupedCount;
	ULONGLONG		nFileUpdateBackupedCount;

	std::vector<_BkItemPair>	vtrChild;

	_BkItemPair()
	{
		Reset();
	}

	void Reset()
	{
		if (bkiAdb.bIsDir)
			eBackupFlag = BackupFlagDirUnknown;
		else
			eBackupFlag = BackupFlagFileUnknown;

		strStatus = _T("");

		nAddSize = 0;
		nUpdateSize = 0;

		nFileBackupedCount = 0;
		nFileAddCount = 0;
		nFileUpdateCount = 0;
		nFileTypeDifferentCount = 0;
		nFileBackupErrorCount = 0;

		nFileAddBackupedCount = 0;
		nFileUpdateBackupedCount = 0;

		vtrChild.clear();
	}
} BkItemPair;

typedef struct _BackupItemUpdated
{
	std::vector<int>	vtrDirIndex;
	int					nItemIndex;

	BackupFlag			eBackupFlag;
	CString				strStatus;

	ULONGLONG			nFileAddBackupedCount;
	ULONGLONG			nFileUpdateBackupedCount;
	ULONGLONG			nFileBackupErrorCount;

	_BackupItemUpdated()
	{
		nItemIndex = -1;
		eBackupFlag = (BackupFlag)-1;
		nFileAddBackupedCount = 0;
		nFileUpdateBackupedCount = 0;
		nFileBackupErrorCount = 0;
	}
} BackupItemUpdated;


// CAdbBackupMasterDlg 对话框
class CAdbBackupMasterDlg : public CDialogEx
{
// 构造
public:
	CAdbBackupMasterDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADBBACKUPMASTER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnUpdateProcessingInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnObtainingThreadStatusChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnObtainingThreadUpdateInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBackupThreadStatusChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBackupItemUpdated(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBackupCountSizeUpdated(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButtonBackupSettings();
	afx_msg void OnNMDblclkListBackupFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonObtainBackupInfo();
	afx_msg void OnBnClickedButtonBackupListParent();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonBackupListRoot();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonBackup();
	DECLARE_MESSAGE_MAP()

protected:
	HICON m_hIcon;

private:
	void UpdateBackupListCtrl();

	void ObtainBackupInfoImpl();
	void ObtainDirInfo(BkItemPair& dirPair);
	void ObtainFileInfo(BkItemPair& filePair);

	void GetLocalItemInfo(BkLocalItem& local);
	CString GetLocalItemPath(const CString& adbPath);

	void AddProcessInfo(const CString& strInfo);
	void ResetBackupProgressData();
	void ShowBackupProgressData();

	void BackupImpl();
	bool BackupDir(BkItemPair& dirPair, std::vector<int> vtrDirIndex, int nItemIndex);
	bool BackupFile(BkItemPair& filePair, std::vector<int> vtrDirIndex, int nItemIndex);

	BOOL CreateMultipleDirectory(CString strPath);
	int GetIconIndex(BackupFlag eBackupFlag);

	void EnableSysCloseButton(BOOL bEnable = TRUE);

private:
	CImageList				m_ilSmallIconList;

	CString					m_strBackupDir;
	std::vector<AdbFile>	m_vtrFilesToBackup;

	std::vector<BkItemPair>	m_vtrBkItemPairs;

	std::vector<int>		m_vtrCurrentDirIndex;


	CMFCListCtrl			m_wndBackupFilesList;
	CEdit					m_editBackupListCurrentPath;

	ULONGLONG				m_ullFileBackuped;
	ULONGLONG				m_ullTotalFile;
	ULONGLONG				m_ullFileTimeElapsed;

	ULONGLONG				m_ullSizeBackuped;
	ULONGLONG				m_ullTotalSize;
	ULONGLONG				m_ullSizeTimeElapsed;

	CStatic					m_lblFileBackuped;
	CStatic					m_lblFileTotal;
	CStatic					m_lblFileProgressPercent;
	CStatic					m_lblFileBackupSpeed;
	CStatic					m_lblFileTimeElapsed;
	CStatic					m_lblFileTimeRemaining;

	CStatic					m_lblSizeBackuped;
	CStatic					m_lblSizeTotal;
	CStatic					m_lblSizeProgressPercent;
	CStatic					m_lblSizeBackupSpeed;
	CStatic					m_lblSizeTimeElapsed;
	CStatic					m_lblSizeTimeRemaining;

	CEdit					m_editProcessInfo;

	bool					m_bObtaining;
	bool					m_bStopObtaining;

	ULONGLONG				m_ullBackupStartTime;
	bool					m_bBackuping;
	bool					m_bStopBackuping;

	CEdit					m_editBackupTo;

	CButton					m_btnBackupSettings;
	CButton					m_btnFetchBackupInfo;
	CButton					m_btnStartBackup;
	CButton					m_btnClose;

	CPngImage				m_pngUpTop;
	CMFCButton				m_btnUpTop;

	CPngImage				m_pngUp;
	CMFCButton				m_btnUp;
};
