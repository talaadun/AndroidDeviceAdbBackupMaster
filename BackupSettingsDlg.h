#pragma once
#include "afxdialogex.h"
#include <vector>
#include "AdbExecutor.h"


typedef struct _SelectedBackupItems
{
	CString     strFileFullPath;
	bool        bIsDir;
	CString		strDstFullPath;

	bool operator==(const _SelectedBackupItems& other) const
	{
		return (strFileFullPath == other.strFileFullPath) && (bIsDir == other.bIsDir);
	}

	bool operator!=(const _SelectedBackupItems& other) const
	{
		return !(*this == other);
	}
} SelectedBackupItems;


// CBackupSettings 对话框

class CBackupSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBackupSettingsDlg)

public:
	CBackupSettingsDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBackupSettingsDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BACKUP_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonRefreshExplorer();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnNMDblclkListAdbExplorer(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonParent();
	afx_msg void OnBnClickedButtonSaveClose();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonRemoveAll();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonBackToRoot();


	bool IsBackupSettingsChanged();
	LPCTSTR GetBackupTo();
	std::vector<SelectedBackupItems>& GetFilesToBackup();

private:
	void UpdateAdbExplorerList();
	void UpdateSelectedFilesList();

private:
	CImageList							m_ilSmallIconList;

	CEdit								m_editAdbDir;
	static CString						m_strAdbDir;
	static std::vector<AdbFile>			m_vtrAdbFiles;

	CEdit								m_editBackupTo;
	CString								m_strBackupTo;
	std::vector<SelectedBackupItems>	m_vtrSelectedBackupItems;

	CListCtrl							m_listAdbExplorer;
	CListCtrl							m_listSelectedFoldersFiles;


	// Store data read from json file, for comparation when save & close
	CString								m_strJsonBackupTo;
	std::vector<SelectedBackupItems>	m_vtrJsonSelectedBackupItems;

	CPngImage							m_pngUpTop;
	CMFCButton							m_btnUpTop;

	CPngImage							m_pngUp;
	CMFCButton							m_btnParent;

	CPngImage							m_pngRefresh;
	CMFCButton							m_btnRefresh;

	bool								m_bBackupSettingsChanged;
};
