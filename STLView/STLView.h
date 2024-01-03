// STLView.h : STLView アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error このファイルを PCH に含める前に、'stdafx.h' を含めてください。
#endif


#include  "resource.h"       // メイン シンボル
#include  "ExClass.h"


using namespace jbxl;
using namespace jbxwl;




//class DROPTAR;


// CSTLViewApp:
// このクラスの実装については、STLView.cpp を参照してください。
//

class CSTLViewApp : public CWinApp, public CAppCallBack
{
public:
	CSTLViewApp();
	~CSTLViewApp();

	CMultiDocTemplate*  pDocTemplBREP;
	

// オーバーライド
public:
	virtual BOOL InitInstance();
	void fileOpenBrep(CString);

// 実装
	afx_msg void OnAppAbout();

	DECLARE_MESSAGE_MAP()
//	afx_msg void OnFileOpenBrep();
//	afx_msg void OnFileSaveAs();

//	afx_msg void OnAppExit();
	afx_msg void OnFileOpen();
};





extern CSTLViewApp theApp;



