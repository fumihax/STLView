// STLView.cpp : アプリケーションのクラス動作を定義します。
//

#include  "stdafx.h"

#include  "STLView.h"
#include  "MainFrm.h"

#include  "Tools.h"
#include  "Dx9.h"
#include  "Graph.h"
#include  "WinTools.h"

#include  "BREPView.h"
#include  "BREPDoc.h"
#include  "BREPFrame.h"
#include  "RwCTBrepDoc.h"
#include  "BREP_MFCLib.h"
#include  "MessageBoxDLG.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace jbxl;
using namespace jbxwl;



// CSTLViewApp
BEGIN_MESSAGE_MAP(CSTLViewApp, CWinApp)
	//{{AFX_MSG_MAP(CSTLViewApp)
	ON_COMMAND(ID_APP_ABOUT, &CSTLViewApp::OnAppAbout)
	// 標準のファイル基本ドキュメント コマンド
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// 標準の印刷セットアップ コマンド
	//ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	//	ON_COMMAND(ID_FILE_OPEN, OnFileOpenBrep)
	//	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	//ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(ID_FILE_OPEN, &CSTLViewApp::OnFileOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





// CSTLViewApp コンストラクション
CSTLViewApp::CSTLViewApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}




CSTLViewApp::~CSTLViewApp()
{
	//RevokeDragDrop(m_pMainWnd->m_hWnd);
	Dx9ReleaseInterface();
}



// 唯一の CSTLViewApp オブジェクトです。

CSTLViewApp theApp;



// CSTLViewApp 初期化

BOOL CSTLViewApp::InitInstance()
{
	// アプリケーション　マニフェストが　visual スタイルを有効にするために、
	// ComCtl32.dll バージョン 6　以降の使用を指定する場合は、
	// Windows XP に　InitCommonControls() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	/*
	InitCommonControls();

	CWinApp::InitInstance();

	// OLE ライブラリを初期化します。
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}*/

	//
	AfxEnableControlContainer();
	Enable3dControlsStatic();	// MFC と静的にリンクしている場合にはここを呼び出してください．

	// 標準初期化
	// これらの機能を使わずに、最終的な実行可能ファイルのサイズを縮小したい場合は、
	// 以下から、不要な初期化ルーチンを
	// 削除してください。
	// 設定が格納されているレジストリ キーを変更します。
	// TODO: この文字列を、会社名または組織名などの、
	// 適切な文字列に変更してください。
	SetRegistryKey(_T("STL Viewer by NSL"));
	
	LoadStdProfileSettings(4);  // 標準の INI ファイルのオプションをロードします (MRU を含む)
	
	// アプリケーション用のドキュメント テンプレートを登録します。ドキュメント テンプレート
	//  はドキュメント、フレーム ウィンドウとビューを結合するために機能します。
	CMultiDocTemplate* pDocTemplate;


	pDocTemplate = new CMultiDocTemplate(
		IDR_STLViewTYPE,
		RUNTIME_CLASS(CBREPDoc),
		RUNTIME_CLASS(CBREPFrame), 
		RUNTIME_CLASS(CBREPView));

	if (!pDocTemplate) return FALSE;
	AddDocTemplate(pDocTemplate);
	pDocTemplBREP = pDocTemplate;
	

	// メイン MDI フレーム ウィンドウを作成します。
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
	m_pMainWnd = pMainFrame;


	Dx9DividePrimitiveMode = TRUE;
	m_pMainWnd->DragAcceptFiles(TRUE);

//	g_pDropTar = new DROPTAR(m_pMainWnd->m_hWnd);
//	RegisterDragDrop(m_pMainWnd->m_hWnd, g_pDropTar);
	

	// 接尾辞が存在する場合にのみ DragAcceptFiles を呼び出します。
	//  MDI アプリケーションでは、この呼び出しは、m_pMainWnd を設定した直後に発生しなければなりません。
	// DDE、file open など標準のシェル コマンドのコマンドラインを解析します。
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);


	// デフォルトで書類オープン無しにする．
	if (cmdInfo.m_nShellCommand==CCommandLineInfo::FileNew) {
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}


	// コマンド ラインで指定されたディスパッチ コマンドです。アプリケーションが
	// /RegServer、/Register、/Unregserver または /Unregister で起動された場合、 False を返します。
	//if (!ProcessShellCommand(cmdInfo)) return FALSE;

	//
	setSystemLocale();


	// メイン ウィンドウが初期化されたので、表示と更新を行います。
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// DirectX9 の初期化
	BOOL rslt = Dx9CreateInterface(this);
	if (!rslt) {
		MessageBox(m_pMainWnd->m_hWnd, _T("DirectX9の初期化に失敗"), _T("エラー"), MB_OK);
		return FALSE;
	}
//	Dx9DividePrimitiveMode = TRUE;


	return TRUE;
	// この後，メッセージループに入る．
}




/////////////////////////////////////////////////////////////////////////////
//
// アプリケーションのバージョン情報で使われる CAboutDlg ダイアログ
//
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};


CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}


void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// ダイアログを実行するためのアプリケーション コマンド
void CSTLViewApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}





// CSTLViewApp メッセージ ハンドラ

/////////////////////////////////////////////////////////////////////////////
//
// BREP
//

//
// 「Open BREP」
//
void CSTLViewApp::OnFileOpen()
{
	CString fname = EasyGetOpenFileName(_T("BREPファイルを開く"), m_pMainWnd->m_hWnd);
	if (fname.IsEmpty()) return;

/*
	CExFrame* pfrm = CreateDocFrmView(pDocTemplBREP, this);
	int ret = ExecDocFrmView(pfrm, (char*)(LPCSTR)fname);

	ExecDocFrmViewError(m_pMainWnd->m_hWnd, ret); 
	if (ret==0) pfrm->pView->TimerStart();
/**/

	fileOpenBrep(fname);

	return;
}





void CSTLViewApp::fileOpenBrep(CString fname)
{
	CExFrame* pfrm = CreateDocFrmView(pDocTemplBREP, this);
	int ret = ExecDocFrmView(pfrm, fname);

	ExecDocFrmViewError(m_pMainWnd->m_hWnd, ret); 
	if (ret==0) pfrm->pView->TimerStart();	// あまり小さいと AboutBox が上がらない

	return;
}






