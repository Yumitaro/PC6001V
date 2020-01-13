// OS依存の汎用ルーチン(主にUI用)

#include <windows.h>
#include <shlwapi.h>	// ie依存

#include <string>

#include "id_config.h"
#include "../common.h"
#include "../log.h"
#include "../osd.h"


#define	DIR_CONFIG		"P6V"	// 設定ファイルフォルダ


////////////////////////////////////////////////////////////////
// スタティック変数
////////////////////////////////////////////////////////////////
static HANDLE hMutex;			// 多重起動チェック用のミューテックス
static P6VPATH ConfigPath = "";	// 設定ファイルパス保存用



////////////////////////////////////////////////////////////////
// 初期化
//
// 引数:	なし
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_Init( void )
{
	return OSD_Init_Sub();
}


////////////////////////////////////////////////////////////////
// 終了処理
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_Quit( void )
{
	PRINTD( OSD_LOG, "[OSD][OSD_Quit]\n" );
	
	// Mutex を開放する
	ReleaseMutex( hMutex );
	
	OSD_Quit_Sub();
}


////////////////////////////////////////////////////////////////
// 多重起動チェック
//
// 引数:	なし
// 返値:	bool			true:起動済み false:未起動
////////////////////////////////////////////////////////////////
bool OSD_IsWorking( void )
{
	// メインウィンドウクラスの名前
	#define	MWCLASS	"p6vmainw"
	
	PRINTD( OSD_LOG, "[OSD][OSD_IsWorking]\n" );
	
	HANDLE hCheckMutex = OpenMutex( MUTEX_ALL_ACCESS, false, MWCLASS );
	if( hCheckMutex ){
		CloseHandle( hCheckMutex );
		return true;
	}
	hMutex = CreateMutex( nullptr, 0, MWCLASS );
	return false;
}




////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 設定ファイルパス取得
//
// 引数:	なし
// 返値:	P6VPATH&		取得した文字列への参照(UTF-8)
////////////////////////////////////////////////////////////////
const P6VPATH& OSD_GetConfigPath( void )
{
	PRINTD( OSD_LOG, "[OSD][OSD_GetConfigPath]" );
	
	if( ConfigPath.empty() ){
		// パス取得バッファ
		char str[PATH_MAX+1];
		
		// マイドキュメントを取得する場合
//		if( SHGetSpecialFolderPath( nullptr, str, CSIDL_PERSONAL, 0 ) ){
//			ConfigPath = str;
//			OSD_AddPath( ConfigPath, ConfigPath, P6VSTR2PATH( DIR_CONFIG ) );
//			OSD_AddDelimiter( ConfigPath );
//		}
		// モジュールパスを取得する場合
		if( GetModuleFileName( nullptr, str, sizeof(str) ) ){
			PathRemoveFileSpec( str );	// ファイル名とデリミタを削除
			PathAddBackslash( str );
			ConfigPath = str;
		}
	}
	PRINTD( OSD_LOG, "%s\n", P6VPATH2STR( ConfigPath ).c_str() );
	
	return ConfigPath;
}


////////////////////////////////////////////////////////////////
// その他の雑関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// フォントファイル作成
//
// 引数:	hfile			半角フォントファイルパス
//			zfile			全角フォントファイルパス
//			size			文字サイズ(半角文字幅ピクセル数)
// 返値:	bool			true:作成成功 false:作成失敗
////////////////////////////////////////////////////////////////
bool OSD_CreateFont( const P6VPATH& hfile, const P6VPATH& zfile, int size )
{
	int ret = 0;
	int Wscr = size * 2 * 192;
	int Hscr = size * 2 *  48;
	VRect srec;
	
	srec.x = 0;
	srec.y = 0;
	srec.w = Wscr;
	srec.h = Hscr;
	
	// デバイスコンテキスト取得
	HDC hBmpDC = CreateCompatibleDC( nullptr );
	
	// フォント作成
	HFONT NewFont = CreateFont( size * 2, size,
								FW_DONTCARE, FW_DONTCARE, FW_REGULAR,
								false, false, false,
								SHIFTJIS_CHARSET,
								OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
								DEFAULT_QUALITY,
								FF_MODERN | FIXED_PITCH, "" );
	HFONT OldFont = (HFONT)SelectObject( hBmpDC, NewFont );
	SetTextColor( hBmpDC, RGB(255,255,255) );
	SetBkColor(   hBmpDC, RGB(  0,  0,  0) );
	
	// 作業用ビットマップ作成
	BYTE* ppixel;	// ピクセルデータ
	char bmbuf[sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*2];
	BITMAPINFO &bi        = *(BITMAPINFO*)&bmbuf;
	BITMAPINFOHEADER &bih = bi.bmiHeader;
	ZeroMemory( &bih, sizeof(bmbuf) );
	
	RGBQUAD* bc       = bi.bmiColors;
	bc[1].rgbBlue     = 255;
	bc[1].rgbGreen    = 255;
	bc[1].rgbRed      = 255;
	bc[1].rgbReserved = 0;
	
	bih.biSize        = sizeof(BITMAPINFOHEADER);
	bih.biWidth       = Wscr;
	bih.biHeight      = -Hscr;
	bih.biPlanes      = 1;
	bih.biBitCount    = 1;
	bih.biCompression = BI_RGB;
	
	HBITMAP hBmp = CreateDIBSection( nullptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&ppixel, 0, 0 );
	ZeroMemory( ppixel, Wscr * Hscr * bih.biBitCount / 8 );
	
	SelectObject( hBmpDC, hBmp );
	SelectObject( hBmpDC, GetStockObject( BLACK_BRUSH ) );
	
	// フォント描画
	BYTE strk[2] = { 0x00, 0x00 };
	// 半角
	if( !hfile.empty() ){
		Rectangle( hBmpDC, 0, 0, Wscr, Hscr );
		for( int y=0; y<2; y++ ){
			for( int x=32; x<128; x++ ){
				strk[0] = x + y * 128;
				TextOut( hBmpDC, x*size, y*size*2, (char*)strk, 1 );
			}
		}
		srec.w = size  *128;
		srec.w = size  *192;
		srec.h = size*2*  2;
		if( !SaveImgData( hfile, ppixel, 1, Wscr, Hscr, &srec ) ) ret++;
	}
	
	// 全角
	if( !zfile.empty() ){
		Rectangle( hBmpDC, 0, 0, Wscr, Hscr );
		// 上位 0x81-0x9f, 0xe0-0xef
		// 下位 0x40-0x7e, 0x80-0xfc
		for( int y=1; y<48; y++ ){
			for( int x=0; x<189; x++ ){
				strk[0] = y + (y<32 ? 0x80 : 0xc0);
				strk[1] = x + 0x40;
				TextOut( hBmpDC, x*size*2, y*size*2, (char*)strk, 2 );
				x += strk[1]==0x7e ? 1 : 0;
			}
		}
		if( !SaveImgData( zfile, ppixel, 1, Wscr, Hscr, nullptr ) ) ret++;
	}
	
	SelectObject( hBmpDC, OldFont );
	DeleteObject( NewFont );
	DeleteObject( hBmp );
	DeleteDC( hBmpDC );
	
	if( ret ) return false;
	else	  return true;
}


// mittBlog  UTF8なstring入れたらShiftJISなstring出てくる関数作った
// http://sayahamitt.net/utf8%e3%81%aastring%e5%85%a5%e3%82%8c%e3%81%9f%e3%82%89shiftjis%e3%81%aastring%e5%87%ba%e3%81%a6%e3%81%8f%e3%82%8b%e9%96%a2%e6%95%b0%e4%bd%9c%e3%81%a3%e3%81%9f/
////////////////////////////////////////////////////////////////
// ShiftJIS -> UTF-8
//
// 引数:	str			文字列バッファへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_SJIStoUTF8( std::string& str )
{
	// Unicodeへ変換後の文字列長を得る
	int lUtf16 = MultiByteToWideChar( CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0 );
	if( !lUtf16 ) return false;
	
	// 必要な分だけUnicode文字列のバッファを確保
	wchar_t bufUtf16[lUtf16+1];
	
	// ShiftJISからUnicodeへ変換
	if( MultiByteToWideChar( CP_THREAD_ACP, 0, str.c_str(), -1, bufUtf16, lUtf16 ) != lUtf16 )
		return false;
	
	// UTF8へ変換後の文字列長を得る
	int lUtf8 = WideCharToMultiByte( CP_UTF8, 0, bufUtf16, -1, nullptr, 0, nullptr, nullptr );
	if( !lUtf8 ) return false;
	
	// 必要な分だけUTF8文字列のバッファを確保
	char bufUtf8[lUtf8+1];
	
	// UnicodeからUTF8へ変換
	if( WideCharToMultiByte( CP_UTF8, 0, bufUtf16, -1, bufUtf8, lUtf8, nullptr, nullptr ) != lUtf8 )
		return false;
	
	str = bufUtf8;
	
	return true;
}


////////////////////////////////////////////////////////////////
// UTF-8 -> ShiftJIS
//
// 引数:	str			文字列バッファへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_UTF8toSJIS( std::string& str )
{
	// Unicodeへ変換後の文字列長を得る
	int lUtf16 = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, nullptr, 0 );
	if( !lUtf16 ) return false;
	
	// 必要な分だけUnicode文字列のバッファを確保
	wchar_t bufUtf16[lUtf16+1];
	
	// UTF8からUnicodeへ変換
	if( MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, bufUtf16, lUtf16 ) != lUtf16 )
		return false;
	
	// ShiftJISへ変換後の文字列長を得る
	int lSJIS = WideCharToMultiByte( CP_THREAD_ACP, 0, bufUtf16, -1, nullptr, 0, nullptr, nullptr );
	if( !lSJIS ) return false;
	
	// 必要な分だけShiftJIS文字列のバッファを確保
	char bufSJIS[lSJIS+1];
	
	// UnicodeからShiftJISへ変換
	if( WideCharToMultiByte( CP_THREAD_ACP, 0, bufUtf16, -1, bufSJIS, lSJIS, nullptr, nullptr ) != lSJIS )
		return false;
	
	str = bufSJIS;
	
	return true;
}


