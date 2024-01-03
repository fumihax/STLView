// BREPDoc.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"

#include "BREPDoc.h"
#include "STL.h"
#include "TriBrep.h"
#include "ProgressBarDLG.h"
#include "WinTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


using namespace jbxl;
using namespace jbxwl;


/////////////////////////////////////////////////////////////////////////////
// CBREPDoc

IMPLEMENT_DYNCREATE(CBREPDoc, CExDocument)

CBREPDoc::CBREPDoc()
{
	Solid   = new BREP_SOLID();
//	Shell   = new BREP_SHELL(Solid);
//	Facet   = new BREP_FACET(Shell);
//	Contour = new BREP_CONTOUR(Facet);

	ChangeData = false;
	CallSave   = false;
}



BOOL CBREPDoc::OnNewDocument()
{
	if (!CExDocument::OnNewDocument()) return FALSE;

	return TRUE;
}



CBREPDoc::~CBREPDoc()
{
	if (ChangeData && !CallSave) SaveFile(false);

	CString  tname = Title + _T(" データ削除中");
	CProgressBarDLG* counter = new CProgressBarDLG(IDD_PROGBAR, (LPCTSTR)tname);
	Solid->counter = counter;
	counter->Start(100);
	//DEBUG_ERR("BREP_FACET  = %d", Solid->facetno);
	//DEBUG_ERR("BREP_VERTEX = %d", Solid->vertexno);

	delete Solid;		// Shellと verticesのdeleteでカウンター表示

	if (counter!=NULL) {
		counter->PutFill();
		delete counter;
	}
}



BEGIN_MESSAGE_MAP(CBREPDoc, CExDocument)
	//{{AFX_MSG_MAP(CBREPDoc)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CBREPDoc 診断

#ifdef _DEBUG
void CBREPDoc::AssertValid() const
{
	CExDocument::AssertValid();
}


void CBREPDoc::Dump(CDumpContext& dc) const
{
	CExDocument::Dump(dc);
}
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// CBREPDoc シリアライズ

void CBREPDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: この位置に保存用のコードを追加してください
	}
	else
	{
		// TODO: この位置に読み込み用のコードを追加してください
	}
}




/////////////////////////////////////////////////////////////////////////////
// CBREPDoc コマンド

BOOL CBREPDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	// TODO: この位置に固有の作成用コードを追加してください
	TCHAR  message[LMESG];

	hasReadData = FALSE;
	hasViewData = FALSE;

//	if (!CExDocument::OnOpenDocument(lpszPathName)) return FALSE;
	if (lpszPathName==NULL) return  FALSE;

	pFrame->GetMDIFrame()->RedrawWindow();

	long int fno, vfno;
	char* mbstr = ts2mbs(lpszPathName);
	STLData* stldata = ReadSTLFile(mbstr, &fno);
	::free(mbstr);
	if (stldata==NULL) return FALSE;
	hasReadData = TRUE;

	SetVertexTolerance(1.0e-8);
	SetAbsVertexTolerance(1.0e-6);

	Solid->counter = new CProgressBarDLG(IDD_PROGBAR, _T("データ読み込み中")); 
	Solid->counter->Start(30);				// 全目盛り30でスタート
	Solid->counter->MakeChildCounter(29);	// 目盛り29で チャイルドカウンタを作成

	vfno = CreateTriSolidFromSTL(Solid, stldata, fno, true);
	freeSTL(stldata);
	DEBUG_INFO("Facet No.= %d/%d", vfno, fno);
	Solid->counter->DeleteChildCounter();

	if (vfno<=0) {
		if (vfno==-1) {
			MessageBox(pFrame->m_hWnd, _T("CBREPDoc::OnOpenDocument: ソリッドへのポインタが NULLです．"), _T("エラー"), MB_OK);
		}
		else if (vfno==-3) {
			MessageBox(pFrame->m_hWnd, _T("CBREPDoc::OnOpenDocument: キャンセルされました．"), _T("キャンセル"), MB_OK);
			pFrame->cancelOperation = TRUE;
		}
		pFrame->doneErrorMessage = TRUE;
		Solid->counter->Stop();
		delete ((CProgressBarDLG*)(Solid->counter));
		Solid->counter = NULL;
		return FALSE;
	}
	Solid->facetno = vfno;

	sntprintf(message, LMESG, _T("全データ数： %7ld\n有効データ： %7ld\n不正データ： %7ld\n不足エッジ： %7zd\n多重エッジ： %7zd\n"),
					 fno, vfno, fno-vfno, Solid->shortage_wings.size(), Solid->surplus_contours.size());
	MessageBox(pFrame->m_hWnd, message, _T("BREP_SOLID データレポート"), MB_ICONQUESTION);

	Solid->counter->MakeChildCounter(1); // 目盛り1を BREPViewへ．deleteはBREPViewが行う． 
	hasViewData = TRUE;

	return TRUE;
}



void  CBREPDoc::SolidRepair(int method)
{
	CProgressBarDLG* counter = new CProgressBarDLG(IDD_PROGBAR, _T("修正処理中"), FALSE, pFrame);
	Solid->counter = counter;
	counter->Start(105);

	// false と true. どちらを先に実行するか？
	counter->MakeChildCounter(40);
	ContoursRepair(method, true); 
	counter->DeleteChildCounter();

	counter->MakeChildCounter(60);
	ContoursRepair(method, false);
	counter->DeleteChildCounter();


	CreateSurplusContoursList(Solid);
	CreateShortageWingsList(Solid);

	if (!Solid->shortage_wings.empty() || !Solid->surplus_contours.empty()){
		ShowSolidInfo();
//		DEBUG_Error("BREPDoc: 補填不能データ %d 個", Solid->shortage_wings.size());
	}

	if (Solid->contours.empty()) hasViewData = FALSE;
	Solid->CloseData();

	counter->MakeChildCounter(5);	// BREPView用 deleteはBREPViewが行う． 
	
	ChangeData = true;
	CallSave   = false;
	return;
}


 
void  CBREPDoc::ContoursRepair(int method, bool mode)
{ 	
	CProgressBarDLG* counter = NULL;
	if (Solid->counter!=NULL) counter = (CProgressBarDLG*)(Solid->counter->GetUsableCounter());
	if (counter!=NULL) counter->SetMax(200);

	// 重複Contourの削除
	CreateSurplusContoursList(Solid); 
	counter->MakeChildCounter(10);
	DeleteSurplusContours(Solid);
	counter->DeleteChildCounter();

	// 直線に並んだ Edgeの削除
	CreateShortageWingsList(Solid);
	counter->MakeChildCounter(10);
	DeleteStraightEdges(Solid);
	counter->DeleteChildCounter();

	// 不足Contourの充填
	CreateShortageWingsList(Solid);
	counter->MakeChildCounter(140);
	FillShortageWings(Solid, method, mode);		// 充填
	counter->DeleteChildCounter();
	counter->MakeChildCounter(10);
	DeleteShortageWings(Solid);					// 孤立Edge(Wing)の削除
	counter->DeleteChildCounter();

	CreateShortageWingsList(Solid);				// Listの作り直し
	counter->MakeChildCounter(30);
 	FillShortageWings(Solid, method, mode);		// 念のためもう一回充当
	counter->DeleteChildCounter();

	return;
}



void CBREPDoc::SaveFile(bool mode) 
{
	TCHAR buf[LMESG];
	CString fname;

	CallSave = true;

	if (pFrame==NULL) return;	// フレームは既に削除されている．

	wchar_t* ttl = (wchar_t*)ts2mbs(Title);
	if (mode) sntprintf(buf, LMESG, _T("%s （アスキー） 保存用ファイルを指定する"), ttl);
	else      sntprintf(buf, LMESG, _T("%s （バイナリ） 保存用ファイルを指定する"), ttl);
	free(ttl);

	fname = EasyGetSaveFileName(buf, _T(""), pFrame->m_hWnd);
	if (fname.IsEmpty()) return;

	int ret = MessageBox(pFrame->m_hWnd, (LPCTSTR)fname, _T("保存確認"), MB_OKCANCEL);
	if (ret!=IDOK) return;
	
	char* mbstr = ts2mbs(fname);
	ret = WriteSTLFile(mbstr, Solid, mode);	// で保存
	::free(mbstr);
	
	if (ret<=0) { 
		//CString fn  = get_file_name_t(fname);	// ファイル名部分
		wchar_t* fn = (wchar_t*)ts2mbs(get_file_name_t(fname));
		sntprintf(buf, LMESG, _T("書き込みファイルのオープンエラー．%s "), fn);
		MessageBox(pFrame->m_hWnd, buf, _T("エラー"), MB_OK);
		free(fn);
	}
	else {
		ChangeData = false;
	}
}



// Solid の状態を表示する．
void CBREPDoc::ShowSolidInfo()
{
	//BREP_SOLID* Solid = ((CBREPDoc*)pDoc)->Solid;
	TCHAR message[LMESG];

	sntprintf(message, LMESG, _T("不足エッジ %zd 個, 多重エッジ %zd 個  "), 
			Solid->shortage_wings.size(), Solid->surplus_contours.size());
	//MessageBox(message, "CBREPDoc::データレポート", MB_OK);
	MessageBox(pFrame->m_hWnd, message, _T("BREP_SOLID データレポート"), MB_ICONQUESTION);
}

