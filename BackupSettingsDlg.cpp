// BackupSettings.cpp: 实现文件
//

#include "pch.h"
#include "AdbBackupMaster.h"
#include "afxdialogex.h"
#include "BackupSettingsDlg.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "StringUtil.h"

// CBackupSettings 对话框

CString CBackupSettingsDlg::m_strAdbDir = _T("/sdcard/");
std::vector<AdbFile> CBackupSettingsDlg::m_vtrAdbFiles;


IMPLEMENT_DYNAMIC(CBackupSettingsDlg, CDialogEx)

CBackupSettingsDlg::CBackupSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BACKUP_SETTINGS, pParent)
{
	m_strBackupTo = _T("H:\\adb_test");

	TCHAR szExePath[MAX_PATH];
	GetModuleFileName(NULL, szExePath, MAX_PATH);
	CString strExeDir = szExePath;
	int nLastSlash = strExeDir.ReverseFind(_T('\\'));
	if (nLastSlash != -1)
	{
		strExeDir = strExeDir.Left(nLastSlash);
	}

	CString strConfigFilePath = strExeDir + _T("\\AdbBackupMaster.json");

	try
	{
		std::ifstream ifs(strConfigFilePath.GetString());
		if (ifs.is_open())
		{
			nlohmann::json jsonConfig;
			ifs >> jsonConfig;

			if (jsonConfig.contains("BackupTo"))
			{
				std::string strBackupToUtf8 = jsonConfig["BackupTo"].get<std::string>();
				m_strBackupTo = CA2T(strBackupToUtf8.c_str(), CP_UTF8);
			}

			if (jsonConfig.contains("BackupItems") && jsonConfig["BackupItems"].is_array())
			{
				m_vtrSelectedBackupItems.clear();
				for (const auto& item : jsonConfig["BackupItems"])
				{
					SelectedBackupItems backupItem;
					if (item.contains("FullPath"))
					{
						std::string strPathUtf8 = item["FullPath"].get<std::string>();
						backupItem.strFileFullPath = CA2T(strPathUtf8.c_str(), CP_UTF8);
					}
					if (item.contains("IsDir"))
					{
						backupItem.bIsDir = item["IsDir"].get<bool>();
					}
					m_vtrSelectedBackupItems.push_back(backupItem);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		CString strError = CA2T(e.what(), CP_UTF8);
		CString strInfo;
		strInfo.Format(_T("Failed to read config file: %s\n"), strError.GetString());
		MessageBox(strInfo, _T("Error"), MB_OK | MB_ICONERROR);
	}

	for (auto& item : m_vtrSelectedBackupItems)
	{
		item.strDstFullPath = CStringUtil::GetBackupDstFullPath(m_strBackupTo, item.strFileFullPath);
	}

	m_strJsonBackupTo = m_strBackupTo;
	m_vtrJsonSelectedBackupItems = m_vtrSelectedBackupItems;
	m_bBackupSettingsChanged = false;
}

CBackupSettingsDlg::~CBackupSettingsDlg()
{
}

void CBackupSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ADB_ROOT, m_editAdbDir);
	DDX_Control(pDX, IDC_LIST_ADB_EXPLORER, m_listAdbExplorer);
	DDX_Control(pDX, IDC_LIST_SELECTED_FOLDERS_FILES, m_listSelectedFoldersFiles);
	DDX_Control(pDX, IDC_EDIT_BACKUP_DIR, m_editBackupTo);
	DDX_Control(pDX, IDC_BUTTON_PARENT, m_btnParent);
	DDX_Control(pDX, IDC_BUTTON_REFRESH_EXPLORER, m_btnRefresh);
	DDX_Control(pDX, IDC_BUTTON_BACK_TO_ROOT, m_btnUpTop);
}


BEGIN_MESSAGE_MAP(CBackupSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH_EXPLORER, &CBackupSettingsDlg::OnBnClickedButtonRefreshExplorer)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CBackupSettingsDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CBackupSettingsDlg::OnBnClickedButtonRemove)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ADB_EXPLORER, &CBackupSettingsDlg::OnNMDblclkListAdbExplorer)
	ON_BN_CLICKED(IDC_BUTTON_PARENT, &CBackupSettingsDlg::OnBnClickedButtonParent)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_CLOSE, &CBackupSettingsDlg::OnBnClickedButtonSaveClose)
	ON_BN_CLICKED(IDCANCEL, &CBackupSettingsDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL, &CBackupSettingsDlg::OnBnClickedButtonRemoveAll)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CBackupSettingsDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_BACK_TO_ROOT, &CBackupSettingsDlg::OnBnClickedButtonBackToRoot)
END_MESSAGE_MAP()


// CBackupSettings 消息处理程序

void CBackupSettingsDlg::OnBnClickedButtonRefreshExplorer()
{
	CString strAdbDir;
	m_editAdbDir.GetWindowTextW(strAdbDir);
	if (!strAdbDir.CompareNoCase(_T("/sdcard")))
	{
		strAdbDir = _T("/sdcard/");
	}

	CString strError;
	DWORD dwExitCode;
	if (CAdbExecutor::GetAllFilesInFolder(strAdbDir, m_vtrAdbFiles, strError, dwExitCode))
	{
		m_strAdbDir = strAdbDir;
		m_editAdbDir.SetWindowTextW(m_strAdbDir);
		UpdateAdbExplorerList();
	}
	else
	{
		CString strMsg;
		strMsg.Format(_T("Error occurred. Error: %s"), strError);
		MessageBox(strMsg, _T("Error"), MB_OK | MB_ICONERROR);
	}
}

BOOL CBackupSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//EnableToolTips(TRUE);

	// Android device
	m_editAdbDir.SetWindowText(m_strAdbDir);

	// PNG
	m_pngUpTop.Load(IDB_PNG_UP_TOP, AfxGetInstanceHandle());
	m_btnUpTop.SetImage((HBITMAP)m_pngUpTop, TRUE);
	m_btnUpTop.SetTooltip(_T("Back to root folder"));

	m_pngUp.Load(IDB_PNG_UP, AfxGetInstanceHandle());
	m_btnParent.SetImage((HBITMAP)m_pngUp, TRUE);
	m_btnParent.SetTooltip(_T("Back to upper folder"));

	m_pngRefresh.Load(IDB_PNG_REFRESH, AfxGetInstanceHandle());
	m_btnRefresh.SetImage((HBITMAP)m_pngRefresh, TRUE);
	m_btnRefresh.SetTooltip(_T("Refresh current folder"));


	m_ilSmallIconList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);

	CBitmap bm;
	bm.LoadBitmap(IDB_BITMAP_SMALL_ICONS);
	m_ilSmallIconList.Add(&bm, RGB(0, 0, 0));

	m_listAdbExplorer.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_listAdbExplorer.SetImageList(&m_ilSmallIconList, LVSIL_SMALL);

	m_listAdbExplorer.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 300);
	m_listAdbExplorer.InsertColumn(1, _T("Modified Date Time"), LVCFMT_LEFT, 150);
	m_listAdbExplorer.InsertColumn(2, _T("Size"), LVCFMT_RIGHT, 130);

	// Local
	m_editBackupTo.SetWindowText(m_strBackupTo);

	m_listSelectedFoldersFiles.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_listSelectedFoldersFiles.SetImageList(&m_ilSmallIconList, LVSIL_SMALL);

	m_listSelectedFoldersFiles.InsertColumn(0, _T("Source Name"), LVCFMT_LEFT, 300);
	m_listSelectedFoldersFiles.InsertColumn(1, _T("Destination Name"), LVCFMT_LEFT, 300);

	UpdateAdbExplorerList();
	UpdateSelectedFilesList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CBackupSettingsDlg::OnBnClickedButtonAdd()
{
	bool bHasIgnored = false;
	CString StrIgnoreInfo = _T("Following items are ignored:\r\n\r\n");

	for (int i = 0; i < m_listAdbExplorer.GetItemCount(); ++i)
	{
		if (m_listAdbExplorer.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			if (i < static_cast<int>(m_vtrAdbFiles.size()))
			{
				CString strFullPath = m_vtrAdbFiles[i].strFileFullPath;
				
				// Already in backup list?
				bool bExist = std::find_if(
					m_vtrSelectedBackupItems.begin(),
					m_vtrSelectedBackupItems.end(),
					[&strFullPath](const SelectedBackupItems& item)
					{
						return item.strFileFullPath == strFullPath;
					}
				) != m_vtrSelectedBackupItems.end();
				
				if (bExist)
				{
					bHasIgnored = true;
					CString strInfo;
					strInfo.Format(_T("%s: already in backup list. \r\n"), strFullPath);
					StrIgnoreInfo += strInfo;
					continue;
				}

				// Is parent folder of a item in backup list?
				bool bIsParentDir = std::find_if(
					m_vtrSelectedBackupItems.begin(),
					m_vtrSelectedBackupItems.end(),
					[&strFullPath](const SelectedBackupItems& item)
					{
						return (item.strFileFullPath.GetLength() > strFullPath.GetLength()
							&& item.strFileFullPath.Find(strFullPath) == 0
							&& item.strFileFullPath[strFullPath.GetLength()] == _T('/'));
					}
				) != m_vtrSelectedBackupItems.end();

				if (bIsParentDir)
				{
					bHasIgnored = true;
					CString strInfo;
					strInfo.Format(_T("%s: a sub-path already in backup list. \r\n"), strFullPath);
					StrIgnoreInfo += strInfo;
					continue;
				}

				// Is sub path of a dir in backup list?
				bool bIsSubPath = std::find_if(
					m_vtrSelectedBackupItems.begin(),
					m_vtrSelectedBackupItems.end(),
					[&strFullPath](const SelectedBackupItems& item)
					{
						return (strFullPath.GetLength() > item.strFileFullPath.GetLength()
							&& strFullPath.Find(item.strFileFullPath) == 0
							&& strFullPath[item.strFileFullPath.GetLength()] == _T('/'));
					}
				) != m_vtrSelectedBackupItems.end();

				if (bIsSubPath)
				{
					bHasIgnored = true;
					CString strInfo;
					strInfo.Format(_T("%s: upper-path already in backup list. \r\n"), strFullPath);
					StrIgnoreInfo += strInfo;
					continue;
				}

				CString strDstFullPath = CStringUtil::GetBackupDstFullPath(m_strBackupTo, strFullPath);
				m_vtrSelectedBackupItems.push_back(SelectedBackupItems{ strFullPath, m_vtrAdbFiles[i].bIsDir, strDstFullPath });
			}
		}
	}
	
	std::sort(m_vtrSelectedBackupItems.begin(), m_vtrSelectedBackupItems.end(), [](const SelectedBackupItems& a, const SelectedBackupItems& b)
		{
			if (a.bIsDir != b.bIsDir)
			{
				return a.bIsDir > b.bIsDir;
			}

			return a.strFileFullPath.CompareNoCase(b.strFileFullPath) < 0;
		});

	UpdateSelectedFilesList();

	if (bHasIgnored)
		MessageBox(StrIgnoreInfo, _T("Items Ignored"), MB_OK | MB_ICONINFORMATION);
}

void CBackupSettingsDlg::OnBnClickedButtonRemove()
{
	// Must remove from bottom to top
	for (int i = m_listSelectedFoldersFiles.GetItemCount() - 1 ; i >= 0; --i)
	{
		if (m_listSelectedFoldersFiles.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			m_vtrSelectedBackupItems.erase(m_vtrSelectedBackupItems.begin() + i);
		}
	}

	UpdateSelectedFilesList();
}

void CBackupSettingsDlg::OnNMDblclkListAdbExplorer(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	int nItemIndex = pNMItemActivate->iItem;
	
	if (nItemIndex >= 0 && nItemIndex < static_cast<int>(m_vtrAdbFiles.size()))
	{
		const AdbFile& file = m_vtrAdbFiles[nItemIndex];
		
		if (file.bIsDir)
		{
			CString strFolderPath = file.strFileFullPath;
			
			CString strError;
			DWORD dwExitCode;
			if (CAdbExecutor::GetAllFilesInFolder(strFolderPath, m_vtrAdbFiles, strError, dwExitCode))
			{
				m_strAdbDir = strFolderPath;
				m_editAdbDir.SetWindowText(m_strAdbDir);
				UpdateAdbExplorerList();
			}
			else
			{
				CString strMsg;
				strMsg.Format(_T("Error occurred. Error: %s"), strError);
				MessageBox(strMsg, _T("Error"), MB_OK | MB_ICONERROR);
			}
		}
	}
	
	*pResult = 0;
}

void CBackupSettingsDlg::UpdateAdbExplorerList()
{
	m_listAdbExplorer.DeleteAllItems();

	for (size_t i = 0; i < m_vtrAdbFiles.size(); ++i)
	{
		const AdbFile& newFile = m_vtrAdbFiles[i];

		int nImageIndex = m_vtrAdbFiles[i].bIsDir ? 10 : 0;
		int nItem = m_listAdbExplorer.InsertItem(i, newFile.strFileName, nImageIndex);

		m_listAdbExplorer.SetItemText(nItem, 1, newFile.strModifyTime);

		if (!newFile.bIsDir)
		{
			m_listAdbExplorer.SetItemText(nItem, 2, CStringUtil::GetSizeStrInKB(newFile.ullFileSize));
		}
	}

}

void CBackupSettingsDlg::UpdateSelectedFilesList()
{
	m_listSelectedFoldersFiles.DeleteAllItems();

	for (size_t i = 0; i < m_vtrSelectedBackupItems.size(); ++i)
	{
		int nImageIndex = m_vtrSelectedBackupItems[i].bIsDir ? 10 : 0;
		m_listSelectedFoldersFiles.InsertItem(i, m_vtrSelectedBackupItems[i].strFileFullPath, nImageIndex);
		m_listSelectedFoldersFiles.SetItemText(i, 1,  m_vtrSelectedBackupItems[i].strDstFullPath);
	}
}

void CBackupSettingsDlg::OnBnClickedButtonParent()
{
	CString strCurrentPath = m_strAdbDir;

	if (!strCurrentPath.CompareNoCase(_T("/")))
		return;

	if (!strCurrentPath.CompareNoCase(_T("/sdcard/")))
		return;

	CString strParentPath;
	int nPos = strCurrentPath.ReverseFind(_T('/'));
	if (nPos > 0)
	{
		strParentPath = strCurrentPath.Left(nPos);
		if (!strParentPath.CompareNoCase(_T("/sdcard")))
			strParentPath = _T("/sdcard/");
	}
	else
		strParentPath = _T("/");

	CString strError;
	DWORD dwExitCode;
	if (CAdbExecutor::GetAllFilesInFolder(strParentPath, m_vtrAdbFiles, strError, dwExitCode))
	{
		m_strAdbDir = strParentPath;
		m_editAdbDir.SetWindowText(m_strAdbDir);
		UpdateAdbExplorerList();
	}
	else
	{
		CString strMsg;
		strMsg.Format(_T("Error occurred. Error: %s"), strError);
		MessageBox(strMsg, _T("Error"), MB_OK | MB_ICONERROR);
	}
}

std::vector<SelectedBackupItems>& CBackupSettingsDlg::GetFilesToBackup()
{
	return m_vtrSelectedBackupItems;
}

bool CBackupSettingsDlg::IsBackupSettingsChanged()
{
	return m_bBackupSettingsChanged;
}

LPCTSTR CBackupSettingsDlg::GetBackupTo()
{
	return m_strBackupTo;
}

void CBackupSettingsDlg::OnBnClickedButtonSaveClose()
{
	m_editBackupTo.GetWindowText(m_strBackupTo);

	if (m_strBackupTo != m_strJsonBackupTo || m_vtrSelectedBackupItems != m_vtrJsonSelectedBackupItems)
	{
		m_bBackupSettingsChanged = true;

		TCHAR szExePath[MAX_PATH];
		GetModuleFileName(NULL, szExePath, MAX_PATH);
		CString strExeDir = szExePath;
		int nLastSlash = strExeDir.ReverseFind(_T('\\'));
		if (nLastSlash != -1)
		{
			strExeDir = strExeDir.Left(nLastSlash);
		}

		CString strConfigFilePath = strExeDir + _T("\\AdbBackupMaster.json");

		try
		{
			nlohmann::json jsonConfig;

			std::string strBackupToUtf8 = CT2A(m_strBackupTo.GetString(), CP_UTF8);
			jsonConfig["BackupTo"] = strBackupToUtf8;

			nlohmann::json jsonItems = nlohmann::json::array();
			for (const auto& item : m_vtrSelectedBackupItems)
			{
				nlohmann::json jsonItem;
				std::string strPathUtf8 = CT2A(item.strFileFullPath.GetString(), CP_UTF8);
				jsonItem["FullPath"] = strPathUtf8;
				jsonItem["IsDir"] = item.bIsDir;
				jsonItems.push_back(jsonItem);
			}
			jsonConfig["BackupItems"] = jsonItems;

			std::ofstream ofs(strConfigFilePath.GetString());
			if (ofs.is_open())
			{
				ofs << std::setw(4) << jsonConfig << std::endl;
				ofs.close();
			}
		}
		catch (const std::exception& e)
		{
			CString strError = CA2T(e.what(), CP_UTF8);
			CString strInfo;
			strInfo.Format(_T("Failed to write config file: %s\n"), strError.GetString());
			MessageBox(strInfo, _T("Error"), MB_OK | MB_ICONERROR);
		}
	}

	CDialogEx::OnOK();
}

void CBackupSettingsDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}

void CBackupSettingsDlg::OnBnClickedButtonRemoveAll()
{
	m_vtrSelectedBackupItems.clear();
	UpdateSelectedFilesList();
}

void CBackupSettingsDlg::OnBnClickedButtonBrowse()
{
	CFolderPickerDialog dlg;
	
	if (dlg.DoModal() == IDOK)
	{
		CString strSelectedPath = dlg.GetFolderPath();
		if (m_strBackupTo != strSelectedPath)
		{
			m_strBackupTo = strSelectedPath;
			m_editBackupTo.SetWindowText(m_strBackupTo);

			for (auto& item : m_vtrSelectedBackupItems)
			{
				item.strDstFullPath = CStringUtil::GetBackupDstFullPath(m_strBackupTo, item.strFileFullPath);
			}

			UpdateSelectedFilesList();
		}
	}
}

void CBackupSettingsDlg::OnBnClickedButtonBackToRoot()
{
	if (!m_strAdbDir.CompareNoCase(_T("/sdcard/")))
	{
		return;
	}

	if (!m_strAdbDir.CompareNoCase(_T("/")))
	{
		return;
	}

	CString strRoot;
	if (m_strAdbDir.GetLength() > 8 && m_strAdbDir.Find(_T("/sdcard/")) == 0)
	{
		strRoot = _T("/sdcard/");
	}
	else
	{
		strRoot = _T("/");
	}

	CString strError;
	DWORD dwExitCode;
	if (CAdbExecutor::GetAllFilesInFolder(strRoot, m_vtrAdbFiles, strError, dwExitCode))
	{
		m_strAdbDir = strRoot;
		m_editAdbDir.SetWindowText(m_strAdbDir);
		UpdateAdbExplorerList();
	}
	else
	{
		CString strMsg;
		strMsg.Format(_T("Error occurred. Error: %s"), strError);
		MessageBox(strMsg, _T("Error"), MB_OK | MB_ICONERROR);
	}
}
