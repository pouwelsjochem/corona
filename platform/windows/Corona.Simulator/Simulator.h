//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Simulator.h : header file for CSimulatorApp, CRecentDirList, CSimDocManager, CLuaFileDlg
// main header for Simulator application
//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#if USE_JNI
#include "Rtt_JavaHost.h"
#endif
#include "afxcmn.h"
#include "Interop\Ipc\Process.h"
#include <memory>

// CSimulatorApp:
// See Simulator.cpp for the implementation of this class
//

#define SIM_SHAKE_AMOUNT		5	// in pixels
#define SIM_SHAKE_REPS			7
#define SIM_SHAKE_PERIOD		80	// msec

// Define all registry items here
#define REGISTRY_SECTION _T("Preferences")
#define REGISTRY_WORKINGDIR _T("WorkingDir")
#define REGISTRY_DEVICE _T("Device")
#define REGISTRY_XPOS _T("XPos")
#define REGISTRY_YPOS _T("YPos")
#define REGISTRY_CONSOLE_LEFT _T("ConsoleLeft")
#define REGISTRY_CONSOLE_TOP _T("ConsoleTop")
#define REGISTRY_CONSOLE_RIGHT _T("ConsoleRight")
#define REGISTRY_CONSOLE_BOTTOM _T("ConsoleBottom")
#define REGISTRY_SHOWWEBBUILD _T("ShowWebBuild")
#define REGISTRY_SHOWLINUXBUILD _T("ShowLinuxBuild")
#define REGISTRY_SHOWWIN32BUILD _T("ShowWin32Build")
#define REGISTRY_DM_FIRST_RUN_COMPLETE _T("dmFirstRunComplete")
#define REGISTRY_LAST_RUN_SUCCEEDED _T("lastRunSucceeded")

// Define all registry item defaults here
// default for WORKINGDIR is sample directory
#define REGISTRY_DEVICE_DEFAULT ""
#define REGISTRY_XPOS_DEFAULT 0
#define REGISTRY_YPOS_DEFAULT 0
#define REGISTRY_AUTOOPEN_DEFAULT 0
#define REGISTRY_SHOWWEBBUILD_DEFAULT 0
#define REGISTRY_SHOWLINUXBUILD_DEFAULT 0
#define REGISTRY_DM_FIRST_RUN_COMPLETE_DEFAULT 0

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimulatorApp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSimulatorApp : public CWinApp
{
public:
	CSimulatorApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

public:
    // Hold values to be written to/from Registry.  CSimulatorView has the real versions.
	CString GetDisplayName() { return m_sDisplayName; }
	void PutDisplayName( CString sDevice )  { m_sDisplayName = sDevice; }
	CRecentFileList* GetRecentFileList() { return m_pRecentFileList; }
	void PutWP(const WINDOWPLACEMENT& newval);
	bool ShouldShowWebBuildDlg();
	bool ShouldShowLinuxBuildDlg();
	int IsStopBuildRequested() { return m_isStopBuildRequested; }
	void SetStopBuildRequested(int stopBuildRequested)  { m_isStopBuildRequested = stopBuildRequested; }

	bool IsDebugModeEnabled() { return m_isDebugModeEnabled; }
	bool IsLuaExitAllowed() { return m_isLuaExitAllowed; }
    CString GetWorkingDir();
    void SetWorkingDir( CString sDir );
	CString GetApplicationDir();
	CString GetResourceDir();
    CString GetSampleDir();
	static bool CheckPathExists(LPCTSTR path);
	static bool CheckDirExists(LPCTSTR dirName);
	static bool InitJavaPaths();
	static bool CheckJarPath( CString jarPath );
	std::shared_ptr<Interop::Ipc::Process> GetOutputViewerProcessPointer() { return m_outputViewerProcessPointer; }

protected:
	BOOL AuthorizeInstance();

	ULONG_PTR m_gdiplusToken;
    CString m_sDisplayName;
	WINDOWPLACEMENT m_WP;
	bool m_isDebugModeEnabled;
	bool m_isLuaExitAllowed;
	bool m_isConsoleEnabled;
	CString m_sApplicationDir;
    CString m_sResourceDir;
    CString m_sSampleDir;
#if USE_JNI
	Rtt::JavaHostTerminator m_javaHostTerminator;  // auto-terminate Java
#endif
	std::shared_ptr<Interop::Ipc::Process> m_outputViewerProcessPointer;
	BOOL m_isStopBuildRequested;
};

extern CSimulatorApp theApp;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// CRecentDirList - Displays directory rather than "main.lua" in File menu recently used list
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRecentDirList : public CRecentFileList {
public:
        CRecentDirList(UINT nStart, LPCTSTR lpszSection,
                LPCTSTR lpszEntryFormat, int nSize,
                int nMaxDispLen = AFX_ABBREV_FILENAME_LEN)
                : CRecentFileList(nStart, lpszSection,lpszEntryFormat, nSize,
                nMaxDispLen) {}
        BOOL GetDisplayName( CString &strName, int nIndex, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const;
};  // class CRecentDirList

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimDocManager - Allows setting initial directory for Open File dialog
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDocManager is an undocumented MFC class
class CSimDocManager : public CDocManager
{
    public:
       CSimDocManager();
       CSimDocManager( CSimDocManager &);
 
       BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);

	   void SetInitialDir( CString sDir ) { m_sInitialDir = sDir; }
	   CString GetInitialDir() { return m_sInitialDir; }     

    protected:
       CString m_sInitialDir;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLuaFileDialog - Open File Dialog which selects only main.lua
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLuaFileDialog : public CFileDialog {
public:
	CLuaFileDialog(BOOL bOpenFileDialog = TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = CLuaFileDialog::szCustomDefExt,
		LPCTSTR lpszFileName = CLuaFileDialog::szCustomDefFileName,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = CLuaFileDialog::szCustomDefFilter,
		CWnd* pParentWnd = NULL, 
	    DWORD dwSize = 0,
		BOOL bVistaStyle = TRUE );

static CString szCustomDefFilter;
static CString szCustomDefExt;
static CString szCustomDefFileName;

//	virtual void OnFolderChange( );
	virtual BOOL OnFileNameOK( );

};
