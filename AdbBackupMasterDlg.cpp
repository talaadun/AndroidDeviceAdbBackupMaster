
// AdbBackupMasterDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "AdbBackupMaster.h"
#include "AdbBackupMasterDlg.h"
#include "afxdialogex.h"
#include "BackupSettingsDlg.h"
#include "StringUtil.h"
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATE_PROCESSING_INFO							(WM_USER + 100)

#define WM_OBTAIN_THREAD_STATUS_CHANGED						(WM_USER + 101)
#define WM_OBTAIN_THREAD_UPDATE_INFO						(WM_USER + 102)
#define WM_BACKUP_THREAD_STATUS_CHANGED						(WM_USER + 103)
#define WM_BACKUP_ITEM_UPDATED								(WM_USER + 104)
#define WM_BACKUP_COUNT_SIZE_UPDATED						(WM_USER + 105)

#define OBTAIN_THREAD_STATUS_STARTED						(0)
#define OBTAIN_THREAD_STATUS_ABORTED						(1)
#define OBTAIN_THREAD_STATUS_COMPLETED						(2)

#define BACKUP_THREAD_STATUS_STARTED						(0)
#define BACKUP_THREAD_STATUS_ABORTED						(1)
#define BACKUP_THREAD_STATUS_COMPLETED						(2)


const UINT_PTR TIMER_ID_OBTAIN = 1001;
const UINT_PTR TIMER_ID_BACKUP = 1002;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CAdbBackupMasterDlg 对话框



CAdbBackupMasterDlg::CAdbBackupMasterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ADBBACKUPMASTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_APP);

	m_bObtaining = false;
	m_bStopObtaining = false;

	m_bBackuping = false;
	m_bStopBackuping = false;

	CBackupSettingsDlg dlg;
	m_strBackupDir = dlg.GetBackupTo();

	auto vtrBackupItems = dlg.GetFilesToBackup();
	for (const auto& item : vtrBackupItems)
	{
		BkItemPair pair;
		pair.bkiAdb.strFullPath = item.strFileFullPath;
		pair.bkiAdb.strFileName = item.strFileFullPath.Right(item.strFileFullPath.GetLength() - item.strFileFullPath.ReverseFind(_T('/')) - 1);
		pair.bkiAdb.bIsDir = item.bIsDir;
		pair.bkiAdb.ullSize = 0;
		pair.bkiAdb.strModifyTime = _T("");

		pair.eBackupFlag = item.bIsDir ? BackupFlagDirUnknown : BackupFlagFileUnknown;

		m_vtrBkItemPairs.push_back(pair);
	}
}

void CAdbBackupMasterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BACKUP_LIST, m_wndBackupFilesList);
	DDX_Control(pDX, IDC_EDIT_BACKUP_LIST_CURRENT_PATH, m_editBackupListCurrentPath);
	DDX_Control(pDX, IDC_STATIC_FILE_BACKUPED, m_lblFileBackuped);
	DDX_Control(pDX, IDC_STATIC_FILE_TOTAL, m_lblFileTotal);
	DDX_Control(pDX, IDC_STATIC_FILE_PROGRESS_PERCENT, m_lblFileProgressPercent);
	DDX_Control(pDX, IDC_STATIC_FILE_TIME_ELAPSED, m_lblFileTimeElapsed);
	DDX_Control(pDX, IDC_STATIC_FILE_TIME_REMAINING, m_lblFileTimeRemaining);
	DDX_Control(pDX, IDC_STATIC_SIZE_BACKUPED, m_lblSizeBackuped);
	DDX_Control(pDX, IDC_STATIC_SIZE_TOTAL, m_lblSizeTotal);
	DDX_Control(pDX, IDC_STATIC_PROGRESS_PERCENT, m_lblSizeProgressPercent);
	DDX_Control(pDX, IDC_STATIC_TIME_ELAPSED, m_lblSizeTimeElapsed);
	DDX_Control(pDX, IDC_STATIC_SIZE_TIME_REMAINING, m_lblSizeTimeRemaining);
	DDX_Control(pDX, IDC_EDIT_BACKUP_LIST_PROCESSING_INFO, m_editProcessInfo);
	DDX_Control(pDX, IDC_EDIT_BACKUP_LIST_LOCAL_ROOT, m_editBackupTo);
	DDX_Control(pDX, IDC_BUTTON_BACKUP_SETTINGS, m_btnBackupSettings);
	DDX_Control(pDX, IDC_BUTTON_OBTAIN_BACKUP_INFO, m_btnFetchBackupInfo);
	DDX_Control(pDX, IDC_BUTTON_BACKUP, m_btnStartBackup);
	DDX_Control(pDX, IDCANCEL, m_btnClose);
	DDX_Control(pDX, IDC_BUTTON_BACKUP_LIST_ROOT, m_btnUpTop);
	DDX_Control(pDX, IDC_BUTTON_BACKUP_LIST_PARENT, m_btnUp);
	DDX_Control(pDX, IDC_STATIC_FILE_SPEED, m_lblFileBackupSpeed);
	DDX_Control(pDX, IDC_STATIC_SIZE_SPEED, m_lblSizeBackupSpeed);
}

BEGIN_MESSAGE_MAP(CAdbBackupMasterDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_UPDATE_PROCESSING_INFO, OnUpdateProcessingInfo)
	ON_MESSAGE(WM_OBTAIN_THREAD_STATUS_CHANGED, OnObtainingThreadStatusChanged)
	ON_MESSAGE(WM_OBTAIN_THREAD_UPDATE_INFO, OnObtainingThreadUpdateInfo)
	ON_MESSAGE(WM_BACKUP_THREAD_STATUS_CHANGED, OnBackupThreadStatusChanged)
	ON_MESSAGE(WM_BACKUP_ITEM_UPDATED, OnBackupItemUpdated)
	ON_MESSAGE(WM_BACKUP_COUNT_SIZE_UPDATED, OnBackupCountSizeUpdated)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_BACKUP_LIST, &CAdbBackupMasterDlg::OnNMDblclkListBackupFile)
	ON_BN_CLICKED(IDC_BUTTON_BACKUP_SETTINGS, &CAdbBackupMasterDlg::OnBnClickedButtonBackupSettings)
	ON_BN_CLICKED(IDC_BUTTON_OBTAIN_BACKUP_INFO, &CAdbBackupMasterDlg::OnBnClickedButtonObtainBackupInfo)
	ON_BN_CLICKED(IDC_BUTTON_BACKUP_LIST_PARENT, &CAdbBackupMasterDlg::OnBnClickedButtonBackupListParent)
	ON_BN_CLICKED(IDC_BUTTON_BACKUP, &CAdbBackupMasterDlg::OnBnClickedButtonBackup)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_BACKUP_LIST_ROOT, &CAdbBackupMasterDlg::OnBnClickedButtonBackupListRoot)
	ON_BN_CLICKED(IDCANCEL, &CAdbBackupMasterDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CAdbBackupMasterDlg 消息处理程序

BOOL CAdbBackupMasterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// PNG
	m_pngUpTop.Load(IDB_PNG_UP_TOP, AfxGetInstanceHandle());
	m_btnUpTop.SetImage((HBITMAP)m_pngUpTop, TRUE);
	m_btnUpTop.SetTooltip(_T("Back to root folder"));

	m_pngUp.Load(IDB_PNG_UP, AfxGetInstanceHandle());
	m_btnUp.SetImage((HBITMAP)m_pngUp, TRUE);
	m_btnUp.SetTooltip(_T("Back to upper folder"));

	m_editBackupTo.SetWindowText(m_strBackupDir);

	m_ilSmallIconList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);

	CBitmap bm;
	bm.LoadBitmap(IDB_BITMAP_SMALL_ICONS);
	m_ilSmallIconList.Add(&bm, RGB(0, 0, 0));
	m_wndBackupFilesList.SetImageList(&m_ilSmallIconList, LVSIL_SMALL);

	m_wndBackupFilesList.SetExtendedStyle(/*LVS_EX_GRIDLINES |*/ LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

	m_wndBackupFilesList.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 300);
	m_wndBackupFilesList.InsertColumn(1, _T("Backed-up"), LVCFMT_LEFT, 80);
	m_wndBackupFilesList.InsertColumn(2, _T("Added Files"), LVCFMT_LEFT, 120);
	m_wndBackupFilesList.InsertColumn(3, _T("Updated Files"), LVCFMT_LEFT, 120);
	m_wndBackupFilesList.InsertColumn(4, _T("Type Mismatch"), LVCFMT_LEFT, 90);
	m_wndBackupFilesList.InsertColumn(5, _T("Error"), LVCFMT_LEFT, 90);
	m_wndBackupFilesList.InsertColumn(6, _T("Added size"), LVCFMT_RIGHT, 100);
	m_wndBackupFilesList.InsertColumn(7, _T("Updated size"), LVCFMT_RIGHT, 100);
	m_wndBackupFilesList.InsertColumn(8, _T("Status"), LVCFMT_LEFT, 400);

	UpdateBackupListCtrl();
	ResetBackupProgressData();
	ShowBackupProgressData();

	m_btnBackupSettings.EnableWindow(TRUE);
	m_btnFetchBackupInfo.EnableWindow(TRUE);
	m_btnStartBackup.EnableWindow(FALSE);
	m_btnClose.EnableWindow(TRUE);
	EnableSysCloseButton(TRUE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CAdbBackupMasterDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAdbBackupMasterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAdbBackupMasterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CAdbBackupMasterDlg::OnUpdateProcessingInfo(WPARAM wParam, LPARAM lParam)
{
	// 在这里处理线程发送的消息
	CString* pStrInfo = (CString*)lParam;

	AddProcessInfo(*pStrInfo);

	delete pStrInfo;

	return 0;
}

LRESULT CAdbBackupMasterDlg::OnObtainingThreadStatusChanged(WPARAM wParam, LPARAM lParam)
{
	int nStatus = (int)wParam;
	if (nStatus == OBTAIN_THREAD_STATUS_STARTED)
	{
		m_bObtaining = true;
		m_bStopObtaining = false;

		m_btnFetchBackupInfo.SetWindowText(_T("Stop fetching"));
		m_btnFetchBackupInfo.EnableWindow(TRUE);
	}
	else if (nStatus == OBTAIN_THREAD_STATUS_ABORTED)
	{
		m_vtrCurrentDirIndex.clear();
		UpdateBackupListCtrl();

		m_bObtaining = false;
		m_bStopObtaining = false;

		m_wndBackupFilesList.EnableWindow(TRUE);

		AddProcessInfo(_T("Fetching aborted."));

		m_btnFetchBackupInfo.SetWindowText(_T("Fetch Backup Info"));
		m_btnFetchBackupInfo.EnableWindow(TRUE);

		m_btnBackupSettings.EnableWindow(TRUE);
		m_btnStartBackup.EnableWindow(FALSE);
		m_btnClose.EnableWindow(TRUE);
		EnableSysCloseButton(TRUE);
	}
	else
	{
		m_vtrCurrentDirIndex.clear();
		UpdateBackupListCtrl();

		m_bObtaining = false;
		m_bStopObtaining = false;

		m_wndBackupFilesList.EnableWindow(TRUE);

		AddProcessInfo(_T("Fetching completed."));

		m_btnFetchBackupInfo.SetWindowText(_T("Fetch Backup Info"));
		m_btnFetchBackupInfo.EnableWindow(TRUE);

		m_btnBackupSettings.EnableWindow(TRUE);
		m_btnStartBackup.EnableWindow(TRUE);
		m_btnClose.EnableWindow(TRUE);
		EnableSysCloseButton(TRUE);
	}

	return 0;
}

LRESULT CAdbBackupMasterDlg::OnObtainingThreadUpdateInfo(WPARAM wParam, LPARAM lParam)
{
	// Update Total file count and size
	m_ullTotalFile += wParam;
	m_ullTotalSize += lParam;

	m_lblFileTotal.SetWindowText(CStringUtil::GetDisplayStrForNumber(m_ullTotalFile));
	m_lblSizeTotal.SetWindowText(CStringUtil::GetSizeStrInKB(m_ullTotalSize));

	return 0;
}

LRESULT CAdbBackupMasterDlg::OnBackupThreadStatusChanged(WPARAM wParam, LPARAM lParam)
{
	int nStatus = (int)wParam;
	if (nStatus == BACKUP_THREAD_STATUS_STARTED)
	{
		m_bBackuping = true;
		m_bStopBackuping = false;

		m_btnStartBackup.SetWindowText(_T("Stop Backup"));
		m_btnStartBackup.EnableWindow(TRUE);

		m_lblFileTimeRemaining.SetWindowText(_T(""));
		m_lblSizeTimeRemaining.SetWindowText(_T(""));

		m_ullBackupStartTime = GetTickCount64();
		SetTimer(TIMER_ID_BACKUP, 1000, NULL);
	}
	else if (nStatus == BACKUP_THREAD_STATUS_ABORTED)
	{
		m_bBackuping = false;
		m_bStopBackuping = false;

		AddProcessInfo(_T("Backup aborted."));

		m_btnStartBackup.SetWindowText(_T("Start Backup"));
		m_btnStartBackup.EnableWindow(FALSE);

		m_btnBackupSettings.EnableWindow(TRUE);
		m_btnFetchBackupInfo.EnableWindow(TRUE);
		m_btnClose.EnableWindow(TRUE);
		EnableSysCloseButton(TRUE);

		KillTimer(TIMER_ID_BACKUP);
	}
	else
	{
		m_bBackuping = false;
		m_bStopBackuping = false;

		AddProcessInfo(_T("Backup completed."));

		m_btnStartBackup.SetWindowText(_T("Start Backup"));
		m_btnStartBackup.EnableWindow(TRUE);

		m_btnBackupSettings.EnableWindow(TRUE);
		m_btnFetchBackupInfo.EnableWindow(TRUE);
		m_btnClose.EnableWindow(TRUE);
		EnableSysCloseButton(TRUE);

		KillTimer(TIMER_ID_BACKUP);
	}

	return 0;
}

LRESULT CAdbBackupMasterDlg::OnBackupItemUpdated(WPARAM wParam, LPARAM lParam)
{
	BackupItemUpdated* pUpdated = (BackupItemUpdated*)lParam;

	std::vector<BkItemPair>* pVtr = &m_vtrBkItemPairs;
	for (auto nItemIndex : pUpdated->vtrDirIndex)
	{
		(*pVtr)[nItemIndex].nFileBackupedCount += pUpdated->nFileAddBackupedCount + pUpdated->nFileUpdateBackupedCount;
		(*pVtr)[nItemIndex].nFileAddBackupedCount += pUpdated->nFileAddBackupedCount;
		(*pVtr)[nItemIndex].nFileUpdateBackupedCount += pUpdated->nFileUpdateBackupedCount;
		(*pVtr)[nItemIndex].nFileBackupErrorCount += pUpdated->nFileBackupErrorCount;

		pVtr = &((*pVtr)[nItemIndex].vtrChild);
	}

	auto nItemIndex = pUpdated->nItemIndex;

	(*pVtr)[nItemIndex].eBackupFlag = pUpdated->eBackupFlag;
	(*pVtr)[nItemIndex].strStatus = pUpdated->strStatus;

	(*pVtr)[nItemIndex].nFileBackupedCount += pUpdated->nFileAddBackupedCount + pUpdated->nFileUpdateBackupedCount;
	(*pVtr)[nItemIndex].nFileAddBackupedCount += pUpdated->nFileAddBackupedCount;
	(*pVtr)[nItemIndex].nFileUpdateBackupedCount += pUpdated->nFileUpdateBackupedCount;
	(*pVtr)[nItemIndex].nFileBackupErrorCount += pUpdated->nFileBackupErrorCount;


	// Update current displayed list item if needed
	bool bNeedUpdate = true;
	if (m_vtrCurrentDirIndex.size() <= pUpdated->vtrDirIndex.size())
	{
		for (int i = 0; i < m_vtrCurrentDirIndex.size(); i++)
		{
			if (m_vtrCurrentDirIndex[i] == pUpdated->vtrDirIndex[i])
				continue;
			else
			{
				bNeedUpdate = false;
				break;
			}
		}
	}
	else
		bNeedUpdate = false;

	if (bNeedUpdate)
	{
		// Get current list
		std::vector<BkItemPair>* pVtr = &m_vtrBkItemPairs;
		for (auto index : m_vtrCurrentDirIndex)
		{
			pVtr = &((*pVtr)[index].vtrChild);
		}

		// Get item index in current list
		int nItemIndex = -1;
		if (m_vtrCurrentDirIndex.size() == pUpdated->vtrDirIndex.size())
			nItemIndex = pUpdated->nItemIndex;
		else
			nItemIndex = pUpdated->vtrDirIndex[m_vtrCurrentDirIndex.size()];

		int nIconIndex = GetIconIndex((*pVtr)[nItemIndex].eBackupFlag);
		m_wndBackupFilesList.SetItem(nItemIndex, 0, LVIF_IMAGE, NULL, nIconIndex, 0, 0, 0);

		if ((*pVtr)[nItemIndex].nFileBackupedCount > 0)
		{
			m_wndBackupFilesList.SetItem(nItemIndex, 1, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileBackuped), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(nItemIndex, 1, CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileBackupedCount));
		}

		if ((*pVtr)[nItemIndex].nFileAddCount > 0)
			m_wndBackupFilesList.SetItemText(nItemIndex, 2, CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileAddBackupedCount) + _T(" / ") + CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileAddCount));

		if ((*pVtr)[nItemIndex].nFileUpdateCount > 0)
			m_wndBackupFilesList.SetItemText(nItemIndex, 3, CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileUpdateBackupedCount) + _T(" / ") + CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileUpdateCount));

		if ((*pVtr)[nItemIndex].nFileBackupErrorCount > 0)
		{
			m_wndBackupFilesList.SetItem(nItemIndex, 5, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileError), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(nItemIndex, 5, CStringUtil::GetDisplayStrForNumber((*pVtr)[nItemIndex].nFileBackupErrorCount));
		}


		CString strStatus;
		strStatus.Format(_T("%s"), (*pVtr)[nItemIndex].strStatus);
		m_wndBackupFilesList.SetItemText(nItemIndex, 8, strStatus);
	}

	delete pUpdated;

	return 0;
}

LRESULT CAdbBackupMasterDlg::OnBackupCountSizeUpdated(WPARAM wParam, LPARAM lParam)
{
	auto ullElapsedTimeInMS = GetTickCount64() - m_ullBackupStartTime;

	// Update Total file count and size
	m_ullFileBackuped += wParam;
	m_ullSizeBackuped += lParam;

	m_lblFileBackuped.SetWindowText(CStringUtil::GetDisplayStrForNumber(m_ullFileBackuped));
	m_lblSizeBackuped.SetWindowText(CStringUtil::GetSizeStrInKB(m_ullSizeBackuped));

	// Progress
	if (m_ullTotalFile > 0)
	{
		ULONGLONG ullProgress = m_ullFileBackuped * 100 / m_ullTotalFile;
		CString strProgress;
		strProgress.Format(_T("%llu%%"), ullProgress);
		m_lblFileProgressPercent.SetWindowText(strProgress);
	}

	if (m_ullTotalSize > 0)
	{
		ULONGLONG ullProgress = m_ullSizeBackuped * 100 / m_ullTotalSize;
		CString strProgress;
		strProgress.Format(_T("%llu%%"), ullProgress);
		m_lblSizeProgressPercent.SetWindowText(strProgress);
	}

	// Speed
	CString strSpeed;
	strSpeed.Format(_T("%.3f/min"), m_ullFileBackuped / ((double)ullElapsedTimeInMS / 1000 / 60));
	m_lblFileBackupSpeed.SetWindowText(strSpeed);

	strSpeed.Format(_T("%.3f MB/s"), m_ullSizeBackuped / ((double)ullElapsedTimeInMS / 1000) / (1024 * 1024));
	m_lblSizeBackupSpeed.SetWindowText(strSpeed);

	// Time remaining
	if (m_ullFileBackuped > 0 && m_ullTotalFile > 0)
	{
		ULONGLONG ullTimeRemaining = ceil((double)ullElapsedTimeInMS * (m_ullTotalFile - m_ullFileBackuped) / m_ullFileBackuped);
		CString strTime = CStringUtil::GetTimeString(ullTimeRemaining);
		m_lblFileTimeRemaining.SetWindowText(strTime);
	}

	if (m_ullSizeBackuped > 0 && m_ullTotalSize > 0)
	{
		ULONGLONG ullTimeRemaining = ceil((double)ullElapsedTimeInMS * (m_ullTotalSize - m_ullSizeBackuped) / m_ullSizeBackuped);
		CString strTime = CStringUtil::GetTimeString(ullTimeRemaining);
		m_lblSizeTimeRemaining.SetWindowText(strTime);
	}

	return 0;
}

void CAdbBackupMasterDlg::OnNMDblclkListBackupFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	int nItemIndex = pNMItemActivate->iItem;

	std::vector<BkItemPair>* pVtr = &m_vtrBkItemPairs;
	for (auto index : m_vtrCurrentDirIndex)
	{
		pVtr = &((*pVtr)[index].vtrChild);
	}

	if (nItemIndex >= 0 && nItemIndex < static_cast<int>(pVtr->size()))
	{
		if ((*pVtr)[nItemIndex].bkiAdb.bIsDir)
		{
			m_vtrCurrentDirIndex.push_back(nItemIndex);
			UpdateBackupListCtrl();
		}
	}

	*pResult = 0;
}

void CAdbBackupMasterDlg::OnBnClickedButtonBackupSettings()
{
	CBackupSettingsDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		if (dlg.IsBackupSettingsChanged())
		{
			m_strBackupDir = dlg.GetBackupTo();
			m_editBackupTo.SetWindowText(m_strBackupDir);

			auto vtrBackupItems = dlg.GetFilesToBackup();
			m_vtrBkItemPairs.clear();
			for (const auto& item : vtrBackupItems)
			{
				BkItemPair pair;
				pair.bkiAdb.strFullPath = item.strFileFullPath;
				pair.bkiAdb.strFileName = item.strFileFullPath.Right(item.strFileFullPath.GetLength() - item.strFileFullPath.ReverseFind(_T('/')) - 1);
				pair.bkiAdb.bIsDir = item.bIsDir;
				pair.bkiAdb.ullSize = 0;
				pair.bkiAdb.strModifyTime = _T("");

				pair.eBackupFlag = item.bIsDir ? BackupFlagDirUnknown : BackupFlagFileUnknown;

				m_vtrBkItemPairs.push_back(pair);
			}

			m_vtrCurrentDirIndex.clear();
			UpdateBackupListCtrl();

			m_btnStartBackup.EnableWindow(FALSE);
		}
	}
}

void CAdbBackupMasterDlg::UpdateBackupListCtrl()
{
	CString strPath;

	std::vector<BkItemPair>* pVtr = &m_vtrBkItemPairs;
	for (auto index : m_vtrCurrentDirIndex)
	{
		strPath = (*pVtr)[index].bkiAdb.strFullPath;
		pVtr = &((*pVtr)[index].vtrChild);
	}

	m_editBackupListCurrentPath.SetWindowText(strPath);

	m_wndBackupFilesList.DeleteAllItems();

	CString strText;
	for (size_t i = 0; i < (*pVtr).size(); ++i)
	{
		int nIconIndex = GetIconIndex((*pVtr)[i].eBackupFlag);

		if (m_vtrCurrentDirIndex.size() == 0)
			m_wndBackupFilesList.InsertItem(i, (*pVtr)[i].bkiAdb.strFullPath, nIconIndex);
		else
			m_wndBackupFilesList.InsertItem(i, (*pVtr)[i].bkiAdb.strFileName, nIconIndex);

		if ((*pVtr)[i].nFileBackupedCount > 0)
		{
			m_wndBackupFilesList.SetItem(i, 1, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileBackuped), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(i, 1, CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileBackupedCount));
		}

		if ((*pVtr)[i].nFileAddCount > 0)
		{
			m_wndBackupFilesList.SetItem(i, 2, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileAdd), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(i, 2, CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileAddBackupedCount) + _T(" / ") + CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileAddCount));
		}

		if ((*pVtr)[i].nFileUpdateCount > 0)
		{
			m_wndBackupFilesList.SetItem(i, 3, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileUpdate), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(i, 3, CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileUpdateBackupedCount) + _T(" / ") + CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileUpdateCount));
		}

		if ((*pVtr)[i].nFileTypeDifferentCount > 0)
		{
			m_wndBackupFilesList.SetItem(i, 4, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileTypeDifferent), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(i, 4, CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileTypeDifferentCount));
		}

		if ((*pVtr)[i].nFileBackupErrorCount > 0)
		{
			m_wndBackupFilesList.SetItem(i, 5, LVIF_IMAGE, NULL, GetIconIndex(BackupFlagFileError), 0, 0, 0);
			m_wndBackupFilesList.SetItemText(i, 5, CStringUtil::GetDisplayStrForNumber((*pVtr)[i].nFileBackupErrorCount));
		}

		CString strAddSize = CStringUtil::GetSizeStrInKB((*pVtr)[i].nAddSize);
		m_wndBackupFilesList.SetItemText(i, 6, strAddSize);

		CString strUpdateSize = CStringUtil::GetSizeStrInKB((*pVtr)[i].nUpdateSize);
		m_wndBackupFilesList.SetItemText(i, 7, strUpdateSize);

		CString strStatus;
		strStatus.Format(_T("%s"), (*pVtr)[i].strStatus);
		m_wndBackupFilesList.SetItemText(i, 8, strStatus);
	}
}

void CAdbBackupMasterDlg::OnBnClickedButtonObtainBackupInfo()
{
	if (!m_bObtaining)
	{
		m_btnBackupSettings.EnableWindow(FALSE);
		m_btnFetchBackupInfo.EnableWindow(FALSE);
		m_btnStartBackup.EnableWindow(FALSE);
		m_btnClose.EnableWindow(FALSE);
		EnableSysCloseButton(FALSE);

		// Clear data
		for (auto& pair : m_vtrBkItemPairs)
		{
			// DO NOT Reset pair.bkiAdb!!!
			pair.bkiLocal.Reset();
			pair.Reset();
		}

		m_vtrCurrentDirIndex.clear();
		UpdateBackupListCtrl();

		ResetBackupProgressData();
		ShowBackupProgressData();

		m_wndBackupFilesList.EnableWindow(FALSE);

		m_editProcessInfo.SetWindowText(_T(""));


		std::thread t(&CAdbBackupMasterDlg::ObtainBackupInfoImpl, this);
		t.detach();
	}
	else
	{
		m_btnFetchBackupInfo.EnableWindow(FALSE);

		m_bStopObtaining = true;
	}
}

void CAdbBackupMasterDlg::ObtainBackupInfoImpl()
{
	PostMessage(WM_OBTAIN_THREAD_STATUS_CHANGED, OBTAIN_THREAD_STATUS_STARTED, 0);

	// Get selected items info
	for (auto& pair : m_vtrBkItemPairs)
	{
		if (m_bStopObtaining)
			break;

		bool bExist = true;
		bool bIsDirOnADB = true;
		CString strError;
		DWORD dwCode = 0;
		if (CAdbExecutor::GetPathInfo(
			pair.bkiAdb.strFullPath,
			bExist,
			bIsDirOnADB,
			pair.bkiAdb.ullSize,
			pair.bkiAdb.strModifyTime,
			strError,
			dwCode))
		{
			if (!bExist)
			{
				pair.eBackupFlag = pair.bkiAdb.bIsDir ? BackupFlagDirError : BackupFlagFileError;
				pair.strStatus = _T("Does not exist on Android device");

			}
			else
			{
				if (pair.bkiAdb.bIsDir != bIsDirOnADB)
				{
					pair.eBackupFlag = BackupFlagDirTypeDifferent;
					pair.strStatus.Format(_T("It's a %s on Android devie, not %s when selected"), bIsDirOnADB ? _T("dir") : _T("file"), pair.bkiAdb.bIsDir ? _T("dir") : _T("file"));
				}
				else
				{
					if (pair.bkiAdb.bIsDir)
					{
						ObtainDirInfo(pair);
					}
					else
					{
						ObtainFileInfo(pair);
					}
				}
			}
		}
		else
		{
			pair.eBackupFlag = pair.bkiAdb.bIsDir ? BackupFlagDirError : BackupFlagFileError;
			pair.strStatus.Format(_T("Failed to get dir/file info on Android device. Error: %s"), strError);
		}
	}

	if (m_bStopObtaining)
		PostMessage(WM_OBTAIN_THREAD_STATUS_CHANGED, OBTAIN_THREAD_STATUS_ABORTED, 0);
	else
		PostMessage(WM_OBTAIN_THREAD_STATUS_CHANGED, OBTAIN_THREAD_STATUS_COMPLETED, 0);
}

void CAdbBackupMasterDlg::ObtainDirInfo(BkItemPair& dirPair)
{
	if (dirPair.eBackupFlag == BackupFlagDirTypeDifferent || dirPair.eBackupFlag == BackupFlagDirError)
		return;

	CString* pStrInfo = new CString;
	pStrInfo->Format(_T("Checking %s"), dirPair.bkiAdb.strFullPath);
	PostMessage(WM_UPDATE_PROCESSING_INFO, 0, (LPARAM)pStrInfo);

	dirPair.bkiLocal.strFullPath = GetLocalItemPath(dirPair.bkiAdb.strFullPath);
	dirPair.bkiLocal.strFileName = dirPair.bkiAdb.strFileName;

	GetLocalItemInfo(dirPair.bkiLocal);

	if (dirPair.bkiLocal.bExist && !dirPair.bkiLocal.bIsDir)
	{
		dirPair.eBackupFlag = BackupFlagDirTypeDifferent;
		dirPair.strStatus = _T("Type mismatch: dir on Android device, file in backup dir");
		return;
	}

	std::vector<AdbFile> vtrAdbFiles;
	CString strError;
	DWORD dwExitCode;
	if (CAdbExecutor::GetAllFilesInFolder(dirPair.bkiAdb.strFullPath, vtrAdbFiles, strError, dwExitCode))
	{
		dirPair.vtrChild.reserve(vtrAdbFiles.size());

		bool bHasNoNeedBackup = false;
		bool bHasNeedBackup = false;
		bool bHasError = false;

		for (const auto& adbItem : vtrAdbFiles)
		{
			if (m_bStopObtaining)
				return;

			BkItemPair child;

			child.bkiAdb.strFullPath = adbItem.strFileFullPath;
			child.bkiAdb.strFileName = adbItem.strFileName;
			child.bkiAdb.bIsDir = adbItem.bIsDir;
			child.bkiAdb.ullSize = adbItem.ullFileSize;
			child.bkiAdb.strModifyTime = adbItem.strModifyTime;

			if (adbItem.bIsDir)
			{
				child.eBackupFlag = BackupFlagDirUnknown;
				ObtainDirInfo(child);
			}
			else
			{
				child.eBackupFlag = BackupFlagFileUnknown;
				ObtainFileInfo(child);
			}

			dirPair.nAddSize					+= child.nAddSize;
			dirPair.nUpdateSize					+= child.nUpdateSize;

			dirPair.nFileBackupedCount			+= child.nFileBackupedCount;
			dirPair.nFileAddCount				+= child.nFileAddCount;
			dirPair.nFileUpdateCount			+= child.nFileUpdateCount;
			dirPair.nFileTypeDifferentCount		+= child.nFileTypeDifferentCount;
			dirPair.nFileBackupErrorCount		+= child.nFileBackupErrorCount;

			dirPair.vtrChild.push_back(child);
		}

		dirPair.eBackupFlag = BackupFlagDirNormal;
	}
	else
	{
		dirPair.eBackupFlag = BackupFlagDirError;

		strError.Replace(_T("\r\n"), _T(". "));
		dirPair.strStatus.Format(_T("Failed to get dir/file info on Android device. Error: %s"), strError);
	}

}

void CAdbBackupMasterDlg::ObtainFileInfo(BkItemPair& filePair)
{
	CString* pStrInfo = new CString;
	pStrInfo->Format(_T("Checking %s"), filePair.bkiAdb.strFullPath);
	PostMessage(WM_UPDATE_PROCESSING_INFO, 0, (LPARAM)pStrInfo);

	filePair.bkiLocal.strFullPath = GetLocalItemPath(filePair.bkiAdb.strFullPath);
	filePair.bkiLocal.strFileName = filePair.bkiAdb.strFileName;

	GetLocalItemInfo(filePair.bkiLocal);

	if (filePair.bkiLocal.bExist && filePair.bkiLocal.bIsDir)
	{
		filePair.eBackupFlag = BackupFlagFileTypeDifferent;
		filePair.strStatus = _T("Type mismatch: file on Android device, dir in backup dir");
		filePair.nFileTypeDifferentCount++;
		return;
	}

	if (!filePair.bkiLocal.bExist)
	{
		filePair.eBackupFlag = BackupFlagFileAdd;
		filePair.nAddSize = filePair.bkiAdb.ullSize;
		filePair.nFileAddCount++;

		PostMessage(WM_OBTAIN_THREAD_UPDATE_INFO, filePair.nFileAddCount, filePair.nAddSize);

		return;
	}

	if (filePair.bkiAdb.strModifyTime > filePair.bkiLocal.strModifyTime)
	{
		filePair.eBackupFlag = BackupFlagFileUpdate;
		filePair.nUpdateSize = filePair.bkiAdb.ullSize;
		filePair.nFileUpdateCount++;

		PostMessage(WM_OBTAIN_THREAD_UPDATE_INFO, filePair.nFileUpdateCount, filePair.nUpdateSize);

		return;
	}
	else if (filePair.bkiAdb.strModifyTime == filePair.bkiLocal.strModifyTime && filePair.bkiAdb.ullSize > filePair.bkiLocal.ullSize)
	{
		filePair.eBackupFlag = BackupFlagFileUpdate;
		filePair.strStatus = "Modified date time is the same, but file size on Android device is larger";
		filePair.nUpdateSize = filePair.bkiAdb.ullSize;
		filePair.nFileUpdateCount++;

		PostMessage(WM_OBTAIN_THREAD_UPDATE_INFO, filePair.nFileUpdateCount, filePair.nUpdateSize);

		return;
	}
	else
	{
		filePair.eBackupFlag = BackupFlagFileBackuped;
		filePair.nFileBackupedCount++;
		return;
	}
}

void CAdbBackupMasterDlg::GetLocalItemInfo(BkLocalItem& local)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(local.strFullPath, &findData);

	// 1. 不存在
	if (hFind == INVALID_HANDLE_VALUE)
	{
		local.bExist = false;
		return;
	}

	local.bExist = true;
	local.bIsDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	local.ullSize = ((ULONGLONG)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;

	SYSTEMTIME st;
	FILETIME localFt;

	FileTimeToLocalFileTime(&findData.ftLastWriteTime, &localFt);   // 转本地时间
	FileTimeToSystemTime(&localFt, &st);       // 转系统时间

	local.strModifyTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	FindClose(hFind);
}

CString CAdbBackupMasterDlg::GetLocalItemPath(const CString& adbPath)
{
	return CStringUtil::GetBackupDstFullPath(m_strBackupDir, adbPath);
}

void CAdbBackupMasterDlg::AddProcessInfo(const CString& strInfo)
{
	CString strOld;
	m_editProcessInfo.GetWindowText(strOld);
	strOld += _T("\r\n") + strInfo;
	if (strOld.GetLength() > 2000)
	{
		strOld = strOld.Right(2000);
	}

	m_editProcessInfo.SetWindowText(strOld);
	m_editProcessInfo.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

void CAdbBackupMasterDlg::ResetBackupProgressData()
{
	m_ullFileBackuped = 0;
	m_ullTotalFile = 0;
	m_ullFileTimeElapsed = 0;

	m_ullSizeBackuped = 0;
	m_ullTotalSize = 0;
	m_ullSizeTimeElapsed = 0;
}

void CAdbBackupMasterDlg::ShowBackupProgressData()
{
	CString str;

	// File
	str.Format(_T("%llu"), m_ullFileBackuped);
	m_lblFileBackuped.SetWindowText(str);

	str.Format(_T("%llu"), m_ullTotalFile);
	m_lblFileTotal.SetWindowText(str);

	if (m_ullTotalFile == 0)
	{
		m_lblFileProgressPercent.SetWindowText(_T("0%"));
	}
	else
	{
		str.Format(_T("%llu%%"), m_ullFileBackuped * 100 / m_ullTotalFile);
		m_lblFileProgressPercent.SetWindowText(str);
	}

	m_lblFileBackupSpeed.SetWindowText(_T("0.000/min"));
	m_lblFileTimeElapsed.SetWindowText(_T("0:00:00"));
	m_lblFileTimeRemaining.SetWindowText(_T("0:00:00"));

	// Size
	str.Format(_T("%llu"), m_ullSizeBackuped);
	m_lblSizeBackuped.SetWindowText(str);

	str.Format(_T("%llu"), m_ullTotalSize);
	m_lblSizeTotal.SetWindowText(str);

	if (m_ullTotalSize == 0)
	{
		m_lblSizeProgressPercent.SetWindowText(_T("0%"));
	}
	else
	{
		str.Format(_T("%llu%%"), m_ullSizeBackuped * 100 / m_ullTotalSize);
		m_lblSizeProgressPercent.SetWindowText(str);
	}

	m_lblSizeBackupSpeed.SetWindowText(_T("0.000 MB/s"));
	m_lblSizeTimeElapsed.SetWindowText(_T("0:00:00"));
	m_lblSizeTimeRemaining.SetWindowText(_T("0:00:00"));
}

void CAdbBackupMasterDlg::OnBnClickedButtonBackupListParent()
{
	if (m_vtrCurrentDirIndex.size() == 0)
		return;

	m_vtrCurrentDirIndex.pop_back();
	UpdateBackupListCtrl();
}

void CAdbBackupMasterDlg::OnBnClickedButtonBackup()
{
	if (!m_bBackuping)
	{
		m_btnBackupSettings.EnableWindow(FALSE);
		m_btnFetchBackupInfo.EnableWindow(FALSE);
		m_btnStartBackup.EnableWindow(FALSE);
		m_btnClose.EnableWindow(FALSE);
		EnableSysCloseButton(FALSE);

		m_editProcessInfo.SetWindowText(_T(""));

		m_ullFileBackuped = 0; 
		m_ullSizeBackuped = 0;

		std::thread t(&CAdbBackupMasterDlg::BackupImpl, this);
		t.detach();
	}
	else
	{
		m_btnStartBackup.EnableWindow(FALSE);

		m_bStopBackuping = true;
	}
}

void CAdbBackupMasterDlg::BackupImpl()
{
	PostMessage(WM_BACKUP_THREAD_STATUS_CHANGED, BACKUP_THREAD_STATUS_STARTED, 0);

	// To avoid GUI thread and this thread read/write same data
	auto vtrTempPairs = m_vtrBkItemPairs;

	std::vector<int> vtrDirIndex;

	for (int nIndex = 0; nIndex < vtrTempPairs.size(); nIndex++)
	{
		if (m_bStopBackuping)
			break;

		if (vtrTempPairs[nIndex].bkiAdb.bIsDir)
		{
			BackupDir(vtrTempPairs[nIndex], vtrDirIndex, nIndex);
		}
		else
		{
			BackupFile(vtrTempPairs[nIndex], vtrDirIndex, nIndex);
		}
	}

	if (m_bStopBackuping)
		PostMessage(WM_BACKUP_THREAD_STATUS_CHANGED, BACKUP_THREAD_STATUS_ABORTED, 0);
	else
		PostMessage(WM_BACKUP_THREAD_STATUS_CHANGED, BACKUP_THREAD_STATUS_COMPLETED, 0);
}

bool CAdbBackupMasterDlg::BackupDir(BkItemPair& dirPair, std::vector<int> vtrParentDirIndex, int nItemIndex)
{
	if (dirPair.nFileAddCount <= 0 && dirPair.nFileUpdateCount <= 0)
		return true;

	BackupItemUpdated* pBackuping = new BackupItemUpdated;
	pBackuping->vtrDirIndex = vtrParentDirIndex;
	pBackuping->nItemIndex = nItemIndex;
	pBackuping->eBackupFlag = BackupFlagDirBackuping;
	PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pBackuping);

	auto vtrCurrentDirIndex = vtrParentDirIndex;
	vtrCurrentDirIndex.push_back(nItemIndex);

	for (int i = 0; i < dirPair.vtrChild.size(); i++)
	{
		if (m_bStopBackuping)
			break;

		if (dirPair.vtrChild[i].bkiAdb.bIsDir)
		{
			BackupDir(dirPair.vtrChild[i], vtrCurrentDirIndex, i);
		}
		else
		{
			BackupFile(dirPair.vtrChild[i], vtrCurrentDirIndex, i);
		}
	}

	BackupItemUpdated* pFinished = new BackupItemUpdated;
	pFinished->vtrDirIndex = vtrParentDirIndex;
	pFinished->nItemIndex = nItemIndex;
	pFinished->eBackupFlag = BackupFlagDirNormal;
	PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pFinished);

	return true;
}

bool CAdbBackupMasterDlg::BackupFile(BkItemPair& filePair, std::vector<int> vtrParentDirIndex, int nItemIndex)
{
	if (filePair.eBackupFlag != BackupFlagFileAdd && filePair.eBackupFlag != BackupFlagFileUpdate)
		return true;

	CString* pStrInfo = new CString;
	pStrInfo->Format(_T("Backing up file: %s, size: %s bytes"), filePair.bkiAdb.strFullPath, CStringUtil::GetDisplayStrForNumber(filePair.bkiAdb.ullSize));
	PostMessage(WM_UPDATE_PROCESSING_INFO, 0, (LPARAM)pStrInfo);

	BackupItemUpdated* pBackuping = new BackupItemUpdated;
	pBackuping->vtrDirIndex = vtrParentDirIndex;
	pBackuping->nItemIndex = nItemIndex;
	pBackuping->eBackupFlag = BackupFlagFileBackuping;
	PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pBackuping);

	CString strLocalFolder = filePair.bkiLocal.strFullPath.Left(filePair.bkiLocal.strFullPath.ReverseFind(_T('\\')));
	if (!CreateMultipleDirectory(strLocalFolder))
	{
		BackupItemUpdated* pError = new BackupItemUpdated;
		pError->vtrDirIndex = vtrParentDirIndex;
		pError->nItemIndex = nItemIndex;
		pError->eBackupFlag = BackupFlagFileError;
		pError->strStatus.Format(_T("Failed to create dir: %s"), strLocalFolder);
		pError->nFileBackupErrorCount++;
		PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pError);

		return false;
	}

	auto startTime = GetTickCount64();

	std::vector<CString> vtrAdbItems;
	vtrAdbItems.push_back(filePair.bkiAdb.strFullPath);

	BackupItemUpdated* pFinished = new BackupItemUpdated;
	pFinished->vtrDirIndex = vtrParentDirIndex;
	pFinished->nItemIndex = nItemIndex;

	CString strError;
	DWORD dwExitCode = 0;

	// NOTE: if file path has chinese charactor, then cannot use file -> folder style, like: "adb pull /sdcard/XXX.txt E:\backup_dir".
	// This is because adb take UTF-8 as default, it takes the input string as in UTF-8 format. But there is a workaroud, we can
	// use file -> file style, like "adb pull /sdcard/XXX.txt E:\backup_dir\XXX.txt", by this way, it works fine.
	if (CAdbExecutor::BackupFiles(vtrAdbItems, filePair.bkiLocal.strFullPath, strError, dwExitCode))
	{
		pFinished->eBackupFlag = BackupFlagFileBackuped;
		if (filePair.eBackupFlag == BackupFlagFileAdd)
		{
			pFinished->nFileAddBackupedCount++;
		}
		else
		{
			pFinished->nFileUpdateBackupedCount++;
		}

		auto elapsedTimeInMs = GetTickCount64() - startTime;
		pFinished->strStatus.Format(_T("%.3f s, %.3f MB/s"), (double)elapsedTimeInMs / 1000,
			(double)(filePair.nAddSize + filePair.nUpdateSize) / ((double)elapsedTimeInMs / 1000) / (1024 * 1024));

		PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pFinished);
		PostMessage(WM_BACKUP_COUNT_SIZE_UPDATED, filePair.nFileAddCount + filePair.nFileUpdateCount, filePair.nAddSize + filePair.nUpdateSize);

		return true;
	}
	else
	{
		pFinished->eBackupFlag = BackupFlagFileError;
		pFinished->nFileBackupErrorCount++;
		pFinished->strStatus.Format(_T("Error occurred when backing up file: %s. Error: %s"), filePair.bkiAdb.strFullPath, strError);
		PostMessage(WM_BACKUP_ITEM_UPDATED, 0, (LPARAM)pFinished);

		CString* pStrInfo = new CString;
		pStrInfo->Format(_T("Error occurred when backing up file: %s. Error: %s"), filePair.bkiAdb.strFullPath, strError);
		PostMessage(WM_UPDATE_PROCESSING_INFO, 0, (LPARAM)pStrInfo);

		return false;
	}

}

BOOL CAdbBackupMasterDlg::CreateMultipleDirectory(CString strPath)
{
	// 1. 去掉末尾反斜杠
	if (strPath.Right(1) == _T("\\"))
		strPath = strPath.Left(strPath.GetLength() - 1);

	// 2. 已经是目录 → 成功
	if (PathIsDirectory(strPath))
		return TRUE;

	// 3. 关键：如果这一级是【文件】→ 失败！
	if (PathFileExists(strPath))
		return FALSE;

	// 4. 递归创建父目录
	int nSplit = strPath.ReverseFind(_T('\\'));
	if (nSplit <= 0)
		return FALSE; // 非法路径

	CString strParent = strPath.Left(nSplit);
	if (!CreateMultipleDirectory(strParent))
		return FALSE;

	// 5. 创建当前目录
	return CreateDirectory(strPath, NULL);
}

int CAdbBackupMasterDlg::GetIconIndex(BackupFlag eBackupFlag)
{
	switch (eBackupFlag)
	{
	case BackupFlagFileUnknown:
		return 1;
	case BackupFlagFileBackuped:
		return 2;
	case BackupFlagFileAdd:
		return 3;
	case BackupFlagFileUpdate:
		return 4;
	case BackupFlagFileTypeDifferent:
		return 5;
	case BackupFlagFileBackuping:
		return 6;
	case BackupFlagFileError:
		return 7;
	case BackupFlagDirNormal:
		return 10;
	case BackupFlagDirUnknown:
		return 11;
	case BackupFlagDirTypeDifferent:
		return 12;
	case BackupFlagDirError:
		return 13;
	case BackupFlagDirBackuping:
		return 14;
	default:
		ASSERT(FALSE);
		return 1;
	}
}

void CAdbBackupMasterDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_ID_BACKUP)
	{
		auto ullIntervalInMS = GetTickCount64() - m_ullBackupStartTime;
		CString strTime = CStringUtil::GetTimeString(ullIntervalInMS);
		m_lblFileTimeElapsed.SetWindowText(strTime);
		m_lblSizeTimeElapsed.SetWindowText(strTime);
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CAdbBackupMasterDlg::OnBnClickedButtonBackupListRoot()
{
	if (m_vtrCurrentDirIndex.size() == 0)
		return;

	m_vtrCurrentDirIndex.clear();
	UpdateBackupListCtrl();
}

void CAdbBackupMasterDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}

void CAdbBackupMasterDlg::EnableSysCloseButton(BOOL bEnable)
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		if (bEnable)
			pSysMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		else
			pSysMenu->EnableMenuItem(SC_CLOSE, MF_GRAYED | MF_DISABLED);
	}
}
