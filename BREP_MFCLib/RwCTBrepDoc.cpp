// RwCTBrepDoc.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
//#include "mfclib.h"
#include "RwCTBrepDoc.h"
#include "ReadFilesDLG.h"
#include "ProgressBarDLG.h"
#include "triBrep.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRwCTBrepDoc

IMPLEMENT_DYNCREATE(CRwCTBrepDoc, CExDocument)

CRwCTBrepDoc::CRwCTBrepDoc()
{
}



CRwCTBrepDoc::~CRwCTBrepDoc()
{
}




BEGIN_MESSAGE_MAP(CRwCTBrepDoc, CExDocument)
	//{{AFX_MSG_MAP(CRwCTBrepDoc)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





/////////////////////////////////////////////////////////////////////////////
// CRwCTBrepDoc 診断

#ifdef _DEBUG
void CRwCTBrepDoc::AssertValid() const
{
	CExDocument::AssertValid();
}

void CRwCTBrepDoc::Dump(CDumpContext& dc) const
{
	CExDocument::Dump(dc);
}
#endif //_DEBUG







/////////////////////////////////////////////////////////////////////////////
// CRwCTBrepDoc シリアライズ

void CRwCTBrepDoc::Serialize(CArchive& ar)
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
// CRwCTBrepDoc コマンド


BOOL CRwCTBrepDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!hasReadData) hasReadData = ReadDataFile((char*)lpszPathName);
	if (!hasReadData) return FALSE;

	if (!hasViewData) hasViewData = MakeViewData();
	if (!hasViewData) return FALSE;
	if (hasViewData) DEBUG_Error("OK");
	else  DEBUG_Error("NG");

	return TRUE;
}








BOOL  CRwCTBrepDoc::ReadDataFile(char* str)
{
	char  message[LMESG];
	char* fname = double_bs(str);
	char* fn    = get_file_name(fname);	// ファイル名部分
	char* err_fname = NULL;				// エラーを起こしたファイル名
	CmnHead  hd;

	// グローバルカウンタの設定
	CProgressBarDLG* counter = NULL;
	counter = new CProgressBarDLG(IDD_PROGBAR, NULL, TRUE);	// ディスパッチャー有効
	if (counter!=NULL) { 
		SetGlobalCounter(counter);
	}


	// マルチスライス読み込み
	if (multiSliceData) {
		// ファイル読み込み設定
		CReadFilesDLG* rsdlg = new CReadFilesDLG(fn);
		if (rsdlg==NULL) { freeNull(fname); return FALSE;}
		if (rsdlg->DoModal()!=IDOK) { 	// キャンセルボタン
			freeNull(fname); 
			pFrame->cancelOperation  = TRUE;
			pFrame->doneErrorMessage = TRUE;
			delete (rsdlg);
			return FALSE;
		}
		startNo	= rsdlg->fromNum;
		endNo	= rsdlg->toNum;
		Title	= rsdlg->fName;
		delete (rsdlg);

		if (counter!=NULL) counter->Start(0, "CRwCTBrepDoc: ファイル読み込み中");

		// データの読み込み
		hd = setHeaderDLG.getCmnHead();
		msGraph = readGraphicSlices<Word>((char*)(LPCSTR)Title, startNo, endNo, &hd, true);
		if (msGraph.err!=Xabs(endNo-startNo)+1 && msGraph.err>=0) {
			err_fname = numbering_name((char*)(LPCSTR)Title, startNo+msGraph.err-1+Sign(endNo-startNo));
			snprintf(message, LMESG-1, "CRwCTBrepDoc::ReadDataFile: 読み込みエラー  %s", err_fname);
			MessageBox(pFrame->m_hWnd, message, "警告: 処理は継続されます", 0);
			free(err_fname);
		}

		if (msGraph.isNull() && msGraph.err==-1) {
			err_fname = numbering_name((char*)(LPCSTR)Title, msGraph.zs);
		}
	}

	
	//	通常の読み込み．	
	else {
		if (counter!=NULL) counter->Start(0, "CRwCTBrepDoc: ファイル読み込み中");
		Title = fname;
		hd = setHeaderDLG.getCmnHead();
		msGraph = readGraphicFile<Word>(fname, &hd, true);
	}

	freeNull(fname);


	// グローバルカウンタの削除
	if (counter!=NULL) {
		counter->Stop();
		ClearGlobalCounter();
		delete counter;
	}


	// エラー処理
	if (msGraph.isNull()) {
		if (msGraph.err==-1) {
			if (multiSliceData) {
				char message[LMESG];
				snprintf(message, LMESG-1, "CRwCTBrepDoc::ReadDataFile: ファイルの読み込みエラー  %s", err_fname);
				MessageBox(pFrame->m_hWnd, message, "エラー", 0);
				free(err_fname);
			}
			else {
				MessageBox(pFrame->m_hWnd, "CRwCTBrepDoc::ReadDataFile: ファイルの読み込みエラー", "エラー", MB_OK);
			}
		}
		else if (msGraph.err==-2) {
			MessageBox(pFrame->m_hWnd, "CRwCTBrepDoc::ReadDataFile: メモリの確保エラー", "エラー",  MB_OK);
		}
		else if (msGraph.err==-3) {
			MessageBox(pFrame->m_hWnd, "CRwCTBrepDoc::ReadDataFile: ファイル読み込みがキャンセルされました", "キャンセル",  MB_OK);
			pFrame->cancelOperation = TRUE;
		}
		else {
			MessageBox(pFrame->m_hWnd, "CRwCTBrepDoc::ReadDataFile: 詳細不明なエラーが発生", "エラー", MB_OK);
		}
		pFrame->doneErrorMessage = TRUE;

		return FALSE;
	}
	return  TRUE;
}






//
// hasViewData==TRUE なら MakeViewData()は実行されない．
//
/*
BOOL  CRwCTBrepDoc::MakeViewData()
{
	char  message[LMESG];
	STLData* stldata;
	long int fno, vfno;
//	int  i, j, k;

	hasViewData = FALSE;
	SetVertexTolerance (1.0e-8);
	SetAbsVertexTolerance(1.0e-6);

	vfno = 0;
	// 数え方を工夫する必要あり
	for (int i=0; i<msGraph.xs*msGraph.ys*msGraph.zs; i++) {
		if (msGraph.gp[i]!=msGraph.zero) vfno++;
	}
	stldata = (STLData*)malloc(sizeof(STLData)*vfno);
	if (stldata==NULL) {
		MessageBox(pFrame->m_hWnd, "CRwCTBrepDoc::MakeViewData: STLメモリ確保エラー．", "エラー", MB_OK);
		return FALSE;
	}
	//DEBUG_Info("Facet No.= %d", vfno);

	fno = 0;
	for (int z=0; z<msGraph.zs; z++) {
		for (int y=0; y<msGraph.ys; y++) {
			for (int x=0; x<msGraph.xs; x++) {
	
				
			
			
			}
		}
	}



//	vfno = CreateSolidFromSTL(stldata, fno, Solid, true,  true);	///// ContouersRing, WingsRingも作成
	vfno = CreateSolidFromSTL(stldata, fno, Solid, false, false);	////  SurplusRing, ShortageRingも作成
	free(stldata);
	DEBUG_Info("Facet No.= %d/%d", vfno, fno);
//	Solid->counter->DeleteChildCounter();

	if (vfno<=0) {
		if (vfno==-1) {
			MessageBox(pFrame->m_hWnd,"CRwCTBrepDoc::MakeViewData: ソリッドへのポインタが NULLです．", "エラー", MB_OK);
		}
		else if (vfno==-3) {
			MessageBox(pFrame->m_hWnd,"CRwCTBrepDoc::MakeViewData: キャンセルされました．", "キャンセル", MB_OK);
			pFrame->cancelOperation = TRUE;
		}
		pFrame->doneErrorMessage = TRUE;
	//	Solid->counter->Stop();
		delete ((CProgressBarDLG*)(Solid->counter));
	//	Solid->counter = NULL;
		return FALSE;
	}
	Solid->facetno = vfno;

	snprintf(message, LMESG, "ファイル中データ： %7d\n有効データ：　　　%7d\n不正データ：　　　%7d\n多重エッジ：　　　%7d\n不足エッジ：　　　%7d\n",
					 fno, vfno, fno-vfno, Solid->surplus_contours.size(), Solid->shortage_wings.size());
	MessageBox(pFrame->m_hWnd, message, "CRwCTBrepDoc::データレポート", MB_OK);

//	Solid->counter->MakeChildCounter(1); // 目盛り1を BREPViewへ．deleteはBREPViewが行う． 
	hasViewData = TRUE;

	return TRUE;
}
*/






BOOL  CRwCTBrepDoc::MakeViewData()
{

	char  message[LMESG];
	STLData* stldata; 
	long int fno, vfno;
	int  x, y, z , i;


	hasViewData = FALSE;
	SetVertexTolerance (1.0e-8);
	SetAbsVertexTolerance(1.0e-6);

	vfno = 0;
	// 数え方を工夫する必要あり
	for (i=0; i<msGraph.xs*msGraph.ys*msGraph.zs; i++) {
		if (msGraph.gp[i]!=msGraph.zero) vfno++;
	}
	vfno = vfno*100;

	MSGraph<Word> msWorkF;
	msWorkF.set(msGraph.xs, msGraph.ys, msGraph.zs, 24);

	MSGraph<Word> msWorkE;
	msWorkE.set(msGraph.xs, msGraph.ys, msGraph.zs, 24);


	stldata = (STLData*)malloc(sizeof(STLData)*vfno);
	if (stldata==NULL) {
		MessageBox(pFrame->m_hWnd,"CRwCTBrepDoc::MakeViewData: STLメモリ確保エラー．","エラー",MB_OK);
		return FALSE;
	}
	//DEBUG_Info("Facet No.= %d", vfno);

	fno = 0;
	for (z=1; z<msGraph.zs-1; z++) {
		for (y=1; y<msGraph.ys-1; y++) {
			for (x=1; x<msGraph.xs-1; x++) {
            
				if (msGraph.point(x, y, z)!=msGraph.zero) {				  
					if (msGraph.point(x+1, y, z)!=msGraph.zero) {					  
						if (msGraph.point(x, y+1, z)!=msGraph.zero) {						  
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11111111(all)
											}
											else {
												//11111110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11111101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;

											}
											else {
												//11111100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11111011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//11111010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11111001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//11111000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11110111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//11110110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11110101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//11110100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11110011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//11110010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11110001
											}
											else {
												//11110000
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11101111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//11101110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11101101
											}
											else {
												//11101100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11101011
											}
											else {
												//11101010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11101001
											}
											else {
												//11101000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11100111
											}
											else {
												//11100110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11100101
											}
											else {
												//11100100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11100011
											}
											else {
												//11100010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11100001
											}
											else {
												//11100000
											}
										}
									}
								}
							}
						}
						else {
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11011111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//11011110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11011101

											}
											else {
												//11011100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11011011
											}
											else {
												//11011010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11011001
											}
											else {
												//11011000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11010111
											}
											else {
												//11010110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11010101
											}
											else {
												//11010100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11010011
											}
											else {
												//11010010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11010001
											}
											else {
												//11010000
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11001111
											}
											else {
												//11001110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11001101
											}
											else {
												//11001100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11001011
											}
											else {
												//11001010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11001001
											}
											else {
												//11001000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11000111
											}
											else {
												//11000110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11000101
											}
											else {
												//11000100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11000011
											}
											else {
												//11000010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//11000001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//11000000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
						}
					}
					else {
						if (msGraph.point(x, y+1, z)!=msGraph.zero) {						  
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10111111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;	
											}
											else {
												//10111110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10111101
											}
											else {
												//10111100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10111011
											}
											else {
												//10111010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10111001
											}
											else {
												//10111000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10110111
											}
											else {
												//10110110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10110101
											}
											else {
												//10110100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10110011
											}
											else {
												//10110010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10110001
											}
											else {
												//10110000
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10101111
											}
											else {
												//10101110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10101101
											}
											else {
												//10101100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10101011
											}
											else {
												//10101010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10101001
											}
											else {
												//10101000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10100111
											}
											else {
												//10100110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10100101
											}
											else {
												//10100100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10100011
											}
											else {
												//10100010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10100001
											}
											else {
												//10100000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
						}
						else {
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10011111
											}
											else {
												//10011110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10011101
											}
											else {
												//10011100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10011011
											}
											else {
												//10011010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10011001
											}
											else {
												//10011000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10010111
											}
											else {
												//10010110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10010101
											}
											else {
												//10010100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10010011
											}
											else {
												//10010010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10010001
											}
											else {
												//10010000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10001111
											}
											else {
												//10001110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10001101
											}
											else {
												//10001100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10001011
											}
											else {
												//10001010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10001001
											}
											else {
												//10001000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10000111
											}
											else {
												//10000110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10000101
											}
											else {
												//10000100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10000011
											}
											else {
												//10000010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//10000001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//10000000												
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
						}
					}
				}
				else {
					if (msGraph.point(x+1, y, z)!=msGraph.zero) {					  
						if (msGraph.point(x, y+1, z)!=msGraph.zero) {						  
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01111111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//01111110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01111101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//01111100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01111011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//01111010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01111001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//01111000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01110111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//01110110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01110101
											}
											else {
												//01110100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01110011
											}
											else {
												//01110010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01110001
											}
											else {
												//01110000
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01101111
											}
											else {
												//01101110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01101101
											}
											else {
												//01101100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01101011
											}
											else {
												//01101010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01101001
											}
											else {
												//01101000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01100111
											}
											else {
												//01100110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01100101
											}
											else {
												//01100100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01100011
											}
											else {
												//01100010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01100001
											}
											else {
												//01100000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;	
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
						}
						else {
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01011111
											}
											else {
												//01011110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01011101
											}
											else {
												//01011100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01011011
											}
											else {
												//01011010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01011001
											}
											else {
												//01011000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01010111
											}
											else {
												//01010110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01010101
											}
											else {
												//01010100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01010011
											}
											else {
												//01010010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01010001
											}
											else {
												//01010000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;	
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01001111
											}
											else {
												//01001110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01001101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//01001100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01001011
											}
											else {
												//01001010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01001001
											}
											else {
												//01001000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01000111
											}
											else {
												//01000110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01000101
											}
											else {
												//01000100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01000011
											}
											else {
												//01000010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//01000001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;	
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//01000000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;												

											}
										}
									}
								}
							}
						}
					}
					else {
						if (msGraph.point(x, y+1, z)!=msGraph.zero) {						  
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00111111
											}
											else {
												//00111110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00111101
											}
											else {
												//00111100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00111011
											}
											else {
												//00111010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00111001
											}
											else {
												//00111000
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00110111
											}
											else {
												//00110110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00110101
											}
											else {
												//00110100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00110011
											}
											else {
												//00110010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00110001
											}
											else {
												//00110000
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00101111
											}
											else {
												//00101110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00101101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1
													;
															  
												fno++;
											}
											else {
												//00101100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00101011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//00101010
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00101001
											}
											else {
												//00101000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;

											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00100111
											}
											else {
												//00100110
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00100101
											}
											else {
												//00100100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00100011
											}
											else {
												//00100010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00100001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//00100000												
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
							}
						}
						else {
							if (msGraph.point(x, y, z+1)!=msGraph.zero) {							  
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00011111
											}
											else {
												//00011110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00011101
											}
											else {
												//00011100
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00011011
											}
											else {
												//00011010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00011001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//00011000
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00010111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00010110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00010101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00010100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00010011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00010010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00010001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//00010000												
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
									}
								}
							}
							else {
								if (msGraph.point(x+1, y+1, z)!=msGraph.zero) {								  
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00001111

											}
											else {
												//00001110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00001101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00001100
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00001011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00001010
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00001001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
											else {
												//00001000												
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+1;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z;
															  
												fno++;
											}
										}
									}
								}
								else {
									if (msGraph.point(x+1, y, z+1)!=msGraph.zero) {									  
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00000111
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;

											}
											else {
												//00000110
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00000101
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;

												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+0.5;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
											else {
												//00000100											
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+0.5;
												stldata[fno].vect[ 8] = z+1;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y;
												stldata[fno].vect[11] = z+0.5;
															  
												fno++;
											}
										}
									}
									else {
										if (msGraph.point(x, y+1, z+1)!=msGraph.zero) {										  
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00000011
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+0.5;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//00000010											
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x;
												stldata[fno].vect[ 4] = y+0.5;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+0.5;
												stldata[fno].vect[10] = y+1;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
										}
										else {																					
											if (msGraph.point(x+1, y+1, z+1)!=msGraph.zero) {
												//00000001
												stldata[fno].vect[0] = stldata[fno].vect[1] = stldata[fno].vect[2] = 0.0;

												stldata[fno].vect[ 3] = x+0.5;
												stldata[fno].vect[ 4] = y+1;
												stldata[fno].vect[ 5] = z+1;
												stldata[fno].vect[ 6] = x+1;
												stldata[fno].vect[ 7] = y+1;
												stldata[fno].vect[ 8] = z+0.5;
												stldata[fno].vect[ 9] = x+1;
												stldata[fno].vect[10] = y+0.5;
												stldata[fno].vect[11] = z+1;
															  
												fno++;
											}
											else {
												//00000000(no)
											}
										}
									}
								}
							}
						}
					}
				}



			}
		}
	}




//	vfno = CreateSolidFromSTL(stldata, fno, Solid, true,  true);	/*//// ContouersRing, WingsRingも作成
	vfno = CreateSolidFromSTL(stldata, fno, Solid, false, false);	/**/// SurplusRing, ShortageRingも作成
	free(stldata);
	DEBUG_Info("Facet No.= %d/%d", vfno, fno);
//	Solid->counter->DeleteChildCounter();

	if (vfno<=0) {
		if (vfno==-1) {
			MessageBox(pFrame->m_hWnd,"CRwCTBrepDoc::MakeViewData: ソリッドへのポインタが NULLです．","エラー",MB_OK);
		}
		else if (vfno==-3) {
			MessageBox(pFrame->m_hWnd,"CRwCTBrepDoc::MakeViewData: キャンセルされました．", "キャンセル",MB_OK);
			pFrame->cancelOperation = TRUE;
		}
		pFrame->doneErrorMessage = TRUE;
	//	Solid->counter->Stop();
		delete ((CProgressBarDLG*)(Solid->counter));
	//	Solid->counter = NULL;
		return FALSE;
	}
	Solid->facetno = vfno;

	snprintf(message, LMESG, "ファイル中データ： %7d\n有効データ：　　　%7d\n不正データ：　　　%7d\n多重エッジ：　　　%7d\n不足エッジ：　　　%7d\n",
					 fno, vfno, fno-vfno, Solid->surplus_contours.size(), Solid->shortage_wings.size());
	MessageBox(pFrame->m_hWnd, message, "CRwCTBrepDoc::データレポート", MB_OK);

//	Solid->counter->MakeChildCounter(1); // 目盛り1を BREPViewへ．deleteはBREPViewが行う． 
	hasViewData = TRUE;

	msWorkF.free();

	msWorkE.free();
	return TRUE;
}


