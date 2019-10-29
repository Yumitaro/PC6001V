// OS依存の汎用ルーチン(主にUI用)

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>	// ie依存
#include <imagehlp.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "id_config.h"
#include "../common.h"
#include "../log.h"
#include "../osd.h"


////////////////////////////////////////////////////////////////
// スタティック変数
////////////////////////////////////////////////////////////////
static HANDLE hMutex;				// 多重起動チェック用のミューテックス




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




/*
////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// モジュールパス取得
//
// 引数:	なし
// 返値:	std::string&	取得した文字列への参照(UTF-8)
////////////////////////////////////////////////////////////////
const std::filesystem::path& OSD_GetModulePath( void )
{
	PRINTD( OSD_LOG, "[OSD][OSD_GetModulePath]" );
	
	static std::filesystem::path ModPath = "";	// モジュールパス保存用
	
	if( ModPath.empty() ){
		char str[PATH_MAX+1];
		
		if( GetModuleFileName( nullptr, str, sizeof(str) ) ){
			PathRemoveFileSpec( str );	// ファイル名とデリミタを削除
			PathAddBackslash( str );
			ModPath = str;
		}
	}
	PRINTD( OSD_LOG, "%s\n", ModPath.c_str() );
	
	return ModPath;
}
*/




////////////////////////////////////////////////////////////////
// ファイル操作関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// ファイルを開く
//
// 引数:	path			パス
//			mode			モード文字列への参照
// 返値:	FILE*			ファイルポインタ
////////////////////////////////////////////////////////////////
FILE* OSD_Fopen( const std::filesystem::path& path, const std::string& mode )
{
	PRINTD( OSD_LOG, "[OSD][OSD_Fopen] %s(%s) ", path.u8string().c_str(), mode.c_str() );
	
	char str[PATH_MAX+1];
	
	// Windowsの場合 native()はwchar_tなのでcharに変換
	std::wcstombs( str, path.c_str(), sizeof(str) );
	return fopen( str, mode.c_str() );
}




/*
////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// パスの末尾にデリミタを追加
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void AddDelimiter( std::filesystem::path& path )
{
	path /= "";
}


////////////////////////////////////////////////////////////////
// パスの末尾のデリミタを削除
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void DelDelimiter( std::filesystem::path& path )
{
	if( path.filename().empty() ) path = path.parent_path();
}


////////////////////////////////////////////////////////////////
// 相対パス化
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void RelativePath( std::filesystem::path& path )
{
	if( path.empty() ) return;
	
	std::error_code ec;
	std::filesystem::path p = std::filesystem::proximate( path, OSD_GetModulePath(), ec );
	if( ec ) return;
	
	// ../なら絶対パス化
	if( p.u8string().length() >= 2 && p.u8string().substr( 0, 2 ) == ".." )
		AbsolutePath( p );
	
	path = p;
}


////////////////////////////////////////////////////////////////
// 絶対パス化
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void AbsolutePath( std::filesystem::path& path )
{
	PRINTD( OSD_LOG, "[OSD][AbsolutePath] %s -> ", path.u8string().c_str() );
	
	if( path.empty() ) return;
	
	std::filesystem::path p = path;
	
	// 既に絶対パスなら正規化のみ実施
	if( p.is_relative() && !p.has_root_name() )	// Windowsの場合, "C:"は is_relative()==true らしい
		p = OSD_GetModulePath() / p;
	
	// パスを結合して正規化
	path = std::filesystem::weakly_canonical( p );
	
	PRINTD( OSD_LOG, "%s\n", path.u8string().c_str() );
}


////////////////////////////////////////////////////////////////
// パス結合
//
// 引数:	cpath			結合後パス
//			path1			パス1
//			path2			パス2
// 返値:	なし
////////////////////////////////////////////////////////////////
void AddPath( std::filesystem::path& cpath, const std::filesystem::path& path1, const std::filesystem::path& path2 )
{
	// パスを結合
	cpath = path1 / path2;
}


////////////////////////////////////////////////////////////////
// パスからフォルダ名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列
////////////////////////////////////////////////////////////////
const std::string GetFolderNamePart( const std::filesystem::path& path )
{
	PRINTD( OSD_LOG, "[OSD][GetFolderNamePart]\n" );
	
	std::filesystem::path p = path;
	
	return p.remove_filename().u8string();
}


////////////////////////////////////////////////////////////////
// パスからファイル名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列(UTF-8)
////////////////////////////////////////////////////////////////
const std::string GetFileNamePart( const std::filesystem::path& path )
{
	PRINTD( OSD_LOG, "[OSD][GetFileNamePart]\n" );
	
	return path.filename().u8string();
}


////////////////////////////////////////////////////////////////
// パスから拡張子名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列(UTF-8)
////////////////////////////////////////////////////////////////
const std::string GetFileNameExt( const std::filesystem::path& path )
{
	PRINTD( OSD_LOG, "[OSD][GetFileNameExt]\n" );
	
	std::string ext = path.extension().u8string();
	ext.erase( ext.begin() );
	return ext;
}


////////////////////////////////////////////////////////////////
// 拡張子名を変更
//
// 引数:	path			パス
//			ext				新しい拡張子への参照
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool ChangeFileNameExt( std::filesystem::path& path, const std::string& ext )
{
	PRINTD( OSD_LOG, "[OSD][ChangeFileNameExt] %s -> %s\n", GetFileNameExt( path ).c_str(), ext.c_str() );
	
	path.replace_extension( std::filesystem::u8path( ext ) );
	return GetFileNameExt( path ) == ext;
}




////////////////////////////////////////////////////////////////
// ファイル操作関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// ファイルストリームを開く
//
// 引数:	fs				ファイルストリームへの参照
//			path			パス
//			mode			モード
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool FSopen( std::fstream& fs, const std::filesystem::path& path, const std::ios_base::openmode mode )
{
	PRINTD( OSD_LOG, "[OSD][FSopen] %s\n", path.u8string().c_str() );
	
	fs.open( path, mode );
	return fs.is_open() && fs.good();
}


////////////////////////////////////////////////////////////////
// フォルダを作成
//
// 引数:	path			パス
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool CreateFolder( const std::filesystem::path& path )
{
	PRINTD( OSD_LOG, "[OSD][CreateFolder] %s\n", path.u8string().c_str() );
	
	try{
		std::error_code ec;
		
		std::filesystem::path tpath = path;
		AbsolutePath( tpath );
		
		// モジュールパスより外側には作成しない
		if( tpath.u8string().compare( 0, OSD_GetModulePath().u8string().length(), OSD_GetModulePath().u8string() ) ) return false;
		
		return std::filesystem::create_directories( tpath, ec );
	}
	catch( ... ){
		return false;
	}
}


////////////////////////////////////////////////////////////////
// ファイルの存在チェック
//
// 引数:	fullpath		パス
// 返値:	bool			true:存在する false:存在しない
////////////////////////////////////////////////////////////////
bool FileExist( const std::filesystem::path& fullpath )
{
	PRINTD( OSD_LOG, "[OSD][FileExist] %s\n", fullpath.u8string().c_str() );
	
	try{
		return std::filesystem::exists( std::filesystem::status( fullpath ) );
	}
	catch( ... ){
		return false;
	}
}


////////////////////////////////////////////////////////////////
// ファイルサイズ取得
//
// 引数:	fullpath		パス
// 返値:	DWORD			ファイルサイズ
////////////////////////////////////////////////////////////////
DWORD GetFileSize( const std::filesystem::path& fullpath )
{
	try{
		return std::filesystem::file_size( fullpath );
	}
	catch( ... ){
		return 0;
	}
}


////////////////////////////////////////////////////////////////
// ファイルの読取り専用チェック
//
// 引数:	fullpath		パス
// 返値:	bool			true:読取り専用 false:読み書き
////////////////////////////////////////////////////////////////
bool FileReadOnly( const std::filesystem::path& fullpath )
{
	PRINTD( OSD_LOG, "[OSD][FileReadOnly] %s\n", fullpath.u8string().c_str() );
	
	try{
		std::filesystem::perms perm = std::filesystem::status( fullpath ).permissions();
		return ( perm & ( std::filesystem::perms::owner_write |
						  std::filesystem::perms::group_write |
						  std::filesystem::perms::others_write ) ) == std::filesystem::perms::none ? true : false;
	}
	catch( ... ){
		return false;
	}
}
*/



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
bool OSD_CreateFont( const std::filesystem::path& hfile, const std::filesystem::path& zfile, int size )
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


