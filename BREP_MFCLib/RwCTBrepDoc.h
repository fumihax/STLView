#if !defined(AFX_RWCTBREPDOC_H__2351950C_5EB3_495C_8241_91762BE52624__INCLUDED_)
#define AFX_RWCTBREPDOC_H__2351950C_5EB3_495C_8241_91762BE52624__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/**
CRwCTBrepDoc クラス

	CT画像ファイルを読み込む．CExDocumentクラスを継承．

	予め cmnHeadにデータがある場合は，hasReadData=TRUE にする．
	multiSliceData==TRUE の場合は，マルチスライス読み込みモードになり3Dデータとして読み込む．
	読み込み部分は CRwCTDoc.cpp と同じ

	画像データは 最初 msGraphに読み込まれ, MakeViewData()により STLデータへ変換される．
*/




#include  "ExClass.h"
#include  "BREPDoc.h"
//#include  "CTio.h"




/////////////////////////////////////////////////////////////////////////////
// CRwCTBrepDoc ドキュメント

class CRwCTBrepDoc : public CBREPDoc	// CExDocument
{
protected:
	CRwCTBrepDoc();           // 動的生成に使用されるプロテクト コンストラクタ。
	DECLARE_DYNCREATE(CRwCTBrepDoc)


// アトリビュート
public:

// オペレーション
public:
	virtual BOOL	ReadDataFile(char* str);
	virtual BOOL	MakeViewData();



// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CRwCTBrepDoc)
	public:
	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされます。
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CRwCTBrepDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// メッセージ マップ関数の生成
protected:
	//{{AFX_MSG(CRwCTBrepDoc)
		// メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_RWCTBREPDOC_H__2351950C_5EB3_495C_8241_91762BE52624__INCLUDED_)
