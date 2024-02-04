// MainFrm.cpp : CMainFrame クラスの実装
//

#include "stdafx.h"

#include "STLView.h"
#include "MainFrm.h"
#include "WinTools.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace jbxl;
using namespace jbxwl;


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
//	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_WM_DROPFILES()
//	ON_COMMAND(ID_APP_EXIT, OnAppExit)
END_MESSAGE_MAP()


static UINT indicators[] =
{
	ID_SEPARATOR,           // ステータス ライン インジケータ
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame コンストラクション/デストラクション
CMainFrame::CMainFrame()
{
	// TODO: メンバ初期化コードをここに追加してください。
}


CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
/*	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("ツール バーの作成に失敗しました。\n");
		return -1;      // 作成できませんでした。
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("ステータス バーの作成に失敗しました。\n");
		return -1;      // 作成できませんでした。
	}
	// TODO: ツール バーをドッキング可能にしない場合は、これらの 3 行を削除してください。
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
*/

	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CMDIFrameWnd::PreCreateWindow(cs)) return FALSE;

	cs.style |= WS_CLIPCHILDREN;
	cs.style &= ~(FWS_PREFIXTITLE | FWS_ADDTOTITLE); 

	return TRUE;
}


// CMainFrame 診断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}


void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}


#endif //_DEBUG


// CMainFrame メッセージ ハンドラ

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	// TODO : ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

	int count = DragQueryFile(hDropInfo, 0xFFFFFFFF, 0, 0);

	for (int i=0; i<count; i++) {
		int size = DragQueryFile(hDropInfo, i, 0, 0) + 1;
		if (size<=0) break;

		TCHAR* buf = new TCHAR[size];
		DragQueryFile(hDropInfo, i, buf, size);
		
		theApp.fileOpenBrep(buf);
		delete[] buf;
	}

	//CMDIFrameWnd::OnDropFiles(hDropInfo);
}
