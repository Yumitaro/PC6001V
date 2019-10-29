#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <filesystem>
#include <string>

#include "typedef.h"
#include "vsurface.h"

#include "p6vm.h"

////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class DSP6 {
protected:
	VM6* vm;
	
	HWINDOW Wh;				// ウィンドウハンドル
	
	bool SetScreenSurface();				// スクリーンサーフェス作成
	
public:
	DSP6( VM6* );							// コンストラクタ
	~DSP6();								// デストラクタ
	
	bool Init();							// 初期化
	void SetIcon( const int );				// アイコン設定
	
	bool ResizeScreen();					// スクリーンサイズ変更
	
	void DrawScreen();						// 画面更新
	void SnapShot( const std::filesystem::path& );	// スナップショット
	
	int ScreenX() const;					// 有効スクリーン幅取得
	int ScreenY() const;					// 有効スクリーン高さ取得
	
	HWINDOW GetWindowHandle();				// ウィンドウハンドル取得
};


#endif	// GRAPH_H_INCLUDED
