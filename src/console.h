#ifndef CONSOLE_H_INCLUDED
#define CONSOLE_H_INCLUDED

#include <string>

#include "typedef.h"
#include "vsurface.h"

#define	FSIZE		(6)	// 半角フォントの幅

// 色定義
#define	FC_BLACK	0
#define	FC_DBLUE	1
#define	FC_DGREEN	2
#define	FC_DCYAN	3
#define	FC_DRED		4
#define	FC_DMAGENTA	5
#define	FC_DYELLOW	6
#define	FC_GRAY		7
#define	FC_DGRAY	8
#define	FC_BLUE		9
#define	FC_GREEN	10
#define	FC_CYAN		11
#define	FC_RED		12
#define	FC_MAGENTA	13
#define	FC_YELLOW	14
#define	FC_WHITE	15


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class JFont {
protected:
	static VSurface ZFont;		// 全角フォントデータサーフェス
	static VSurface HFont;		// 半角フォントデータサーフェス
	
	static int zWidth, zHeight;	// 全角文字の幅,高さ
	static int hWidth, hHeight;	// 半角文字の幅,高さ

public:
	JFont();
	~JFont();
	
	static bool OpenFont( const P6VPATH&, const P6VPATH& );	// フォントファイルを開く
	
	static int FontWidth(){ return hWidth; }	// フォントの幅取得(半角)
	static int FontHeight(){ return hHeight; }	// フォントの高さ取得(半角)
};


class ZCons : public JFont, public VSurface {
protected:
	VRect con;					// 描画範囲
	int Xmax,Ymax;				// 縦横最大文字数(半角)
	int x,y;					// カーソル位置
	BYTE fgc,bgc;				// 描画色と背景色
	std::string Caption;		// キャプション
	
	void DrawFrame();							// 枠描画
	void ScrollUp();							// スクロールアップ
	
	void PutCharh( int, int, BYTE, BYTE,  BYTE );	// 半角文字描画
	void PutCharz( int, int, WORD, BYTE,  BYTE );	// 全角文字描画
	
	void sprintc( const std::string& );			// 文字列描画(制御文字対応)
	void sprintr( const std::string& );			// 文字列描画(右詰め)

public:
	ZCons();
	virtual ~ZCons();
	
	bool Init   ( int, int, const std::string&, BYTE = FC_WHITE, BYTE = FC_BLACK );	// 初期化(文字数でサイズ指定)
	bool InitRes( int, int, const std::string&, BYTE = FC_WHITE, BYTE = FC_BLACK );	// 初期化(解像度でサイズ指定)
	void SetColor( BYTE, BYTE );				// 描画色設定
	void SetColor( BYTE );
	
	void Locate( int, int );					// カーソル位置設定
	void LocateR( int, int );					// カーソル位置設定(相対座標)
	void Cls();									// 画面消去
	
	void PutCharH( BYTE );						// 半角1文字描画
	void PutCharZ( WORD );						// 全角1文字描画(SJIS)
	
	template <typename ... Args> void Printf( const std::string& fmt, Args ... args )	// 書式付文字列描画
	{
		std::vector<char> buf( std::snprintf( nullptr, 0, fmt.c_str(), args ... ) + 1 );
		std::snprintf( &buf[0], buf.size(), fmt.c_str(), args ... );
		sprintc( std::string( buf.data(), buf.data() + buf.size() - 1 ) );
	}
	
	template <typename ... Args> void PrintfR( const std::string& fmt, Args ... args )	// 書式付文字列描画(右詰め)
	{
		std::vector<char> buf( std::snprintf( nullptr, 0, fmt.c_str(), args ... ) + 1 );
		std::snprintf( &buf[0], buf.size(), fmt.c_str(), args ... );
		sprintr( std::string( buf.data(), buf.data() + buf.size() - 1 ) );
	}
	
	int GetXline();								// 横最大文字数取得
	int GetYline();								// 縦最大文字数取得
};


#endif	// CONSOLE_H_INCLUDED
