// OS依存の汎用ルーチン(主にUI用)

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>	// ie依存
#include <imagehlp.h>

#ifdef	USEFILESYSTEM
#include <filesystem>
#endif

#include <fstream>
#include <string>
#include <unordered_map>

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
// パスの末尾にデリミタを追加
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_AddDelimiter( P6VPATH& path )
{
#ifdef	USEFILESYSTEM
	path /= "";
#else
	OSD_UTF8toSJIS( path );
	
	char str[path.length()+2];
	
	std::strcpy( str, P6VPATH2STR( path ).c_str() );
	PathAddBackslash( str );
	path = str;
	
	OSD_SJIStoUTF8( path );
#endif
}


////////////////////////////////////////////////////////////////
// パスの末尾のデリミタを削除
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_DelDelimiter( P6VPATH& path )
{
#ifdef	USEFILESYSTEM
	if( path.filename().empty() ) path = path.parent_path();
#else
	OSD_UTF8toSJIS( path );
	
	char str[path.length()+1];
	
	std::strcpy( str, P6VPATH2STR( path ).c_str() );
	PathRemoveBackslash( str );
	path = str;
	
	OSD_SJIStoUTF8( path );
#endif

}


////////////////////////////////////////////////////////////////
// 相対パス化
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_RelativePath( P6VPATH& path )
{
#ifdef	USEFILESYSTEM
	if( path.empty() ) return;
	
	std::error_code ec;
	P6VPATH p = std::filesystem::proximate( path, OSD_GetConfigPath(), ec );
	if( ec ) return;
	
	// ../なら絶対パス化
	if( P6VPATH2STR( p ).length() >= 2 && P6VPATH2STR( p ).substr( 0, 2 ) == ".." )
		OSD_AbsolutePath( p );
	
	path = p;
#else
	char str[PATH_MAX+1];
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	if( tpath.empty() || PathIsRelative( tpath.c_str() ) ) return;
	
	std::string mpath = OSD_GetConfigPath();
	OSD_UTF8toSJIS( mpath );
	
	DWORD attr = GetFileAttributes( tpath.c_str() ) & FILE_ATTRIBUTE_DIRECTORY;
	PathRelativePathTo( str, mpath.c_str(), attr, tpath.c_str(), attr );
	path = str;
	OSD_SJIStoUTF8( path );
	
	if( path.length() >= 2 && path.substr( 0, 3 ) == "..\\" )
		OSD_AbsolutePath( path );
	else if( path.substr( 0, 2 ) == ".\\" )
		path = &path.c_str()[2];
#endif
}


////////////////////////////////////////////////////////////////
// 絶対パス化
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_AbsolutePath( P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][OSD_AbsolutePath] %s -> ", P6VPATH2STR( path ).c_str() );
	
#ifdef	USEFILESYSTEM
	if( path.empty() ) return;
	
	P6VPATH p = path;
	
	// 既に絶対パスなら正規化のみ実施
	if( p.is_relative() && !p.has_root_name() )	// Windowsの場合, "C:"は is_relative()==true らしい
		p = OSD_GetConfigPath() / p;
	
	// パスを結合して正規化
	path = std::filesystem::weakly_canonical( p );
#else
	char str[PATH_MAX+1];
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	if( tpath.empty() || !PathIsRelative( tpath.c_str() ) ) return;
	
	std::string mpath = OSD_GetConfigPath();
	OSD_UTF8toSJIS( mpath );
	
	PathCombine( str, mpath.c_str(), tpath.c_str() );
	path = str;
	OSD_SJIStoUTF8( path );
#endif
	
	PRINTD( OSD_LOG, "%s\n", P6VPATH2STR( path ).c_str() );
}


////////////////////////////////////////////////////////////////
// パス結合
//
// 引数:	cpath			結合後パス
//			path1			パス1
//			path2			パス2
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_AddPath( P6VPATH& cpath, const P6VPATH& path1, const P6VPATH& path2 )
{
#ifdef	USEFILESYSTEM
	// パスを結合
	cpath = path1 / path2;
#else
	char str[PATH_MAX+1];
	
	std::string tpath1 = path1;
	std::string tpath2 = path2;
	OSD_UTF8toSJIS( tpath1 );
	OSD_UTF8toSJIS( tpath2 );
	
	PathCombine( str, tpath1.c_str(), tpath2.c_str() );
	cpath = str;
	OSD_SJIStoUTF8( cpath );
#endif
}


////////////////////////////////////////////////////////////////
// パスからフォルダ名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列
////////////////////////////////////////////////////////////////
const std::string OSD_GetFolderNamePart( const P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][OSD_GetFolderNamePart]\n" );
	
#ifdef	USEFILESYSTEM
	P6VPATH p = path;
	
	return P6VPATH2STR( p.remove_filename() );
#else
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	PathRemoveFileSpec( str );
	
	tpath = str;
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
#endif
}


////////////////////////////////////////////////////////////////
// パスからファイル名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列(UTF-8)
////////////////////////////////////////////////////////////////
const std::string OSD_GetFileNamePart( const P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][OSD_GetFileNamePart]\n" );
	
#ifdef	USEFILESYSTEM
	return P6VPATH2STR( path.filename() );
#else
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	tpath = PathFindFileName( str );
	
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
#endif
}


////////////////////////////////////////////////////////////////
// パスから拡張子名を取得
//
// 引数:	path			パス
// 返値:	std::string		取得した文字列(UTF-8)
////////////////////////////////////////////////////////////////
const std::string OSD_GetFileNameExt( const P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][OSD_GetFileNameExt]\n" );
	
#ifdef	USEFILESYSTEM
	std::string ext = P6VPATH2STR( path.extension() );
	ext.erase( ext.begin() );
	return ext;
#else
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	tpath = PathFindExtension( str );
	if( tpath.front() == '.' ) tpath.erase( tpath.begin() );
	
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
#endif
}


////////////////////////////////////////////////////////////////
// 拡張子名を変更
//
// 引数:	path			パス
//			ext				新しい拡張子への参照
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_ChangeFileNameExt( P6VPATH& path, const std::string& ext )
{
	PRINTD( OSD_LOG, "[OSD][OSD_ChangeFileNameExt] %s -> %s\n", OSD_GetFileNameExt( path ).c_str(), ext.c_str() );
	
#ifdef	USEFILESYSTEM
	path.replace_extension( P6VSTR2PATH( ext ) );
	return OSD_GetFileNameExt( path ) == ext;
#else
	OSD_UTF8toSJIS( path );
	
	std::string text = '.' + ext;
	OSD_UTF8toSJIS( text );
	
	char str[path.length()+text.length()+1];
	
	std::strcpy( str, path.c_str() );
	bool res = PathRenameExtension( str, text.c_str() );
	
	path = str;
	OSD_SJIStoUTF8( path );
	
	return res;
#endif
}




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
FILE* OSD_Fopen( const P6VPATH& path, const std::string& mode )
{
	PRINTD( OSD_LOG, "[OSD][OSD_Fopen] %s(%s) ", P6VPATH2STR( path ).c_str(), mode.c_str() );
	
#ifdef	USEFILESYSTEM
	char str[PATH_MAX+1];
	
	// Windowsの場合 native()はwchar_tなのでcharに変換
	std::wcstombs( str, path.c_str(), sizeof(str) );
	return fopen( str, mode.c_str() );
#else
	std::string str1 = P6VPATH2STR( path );
	std::string str2 = mode;
	OSD_UTF8toSJIS( str1 );
	OSD_UTF8toSJIS( str2 );
	
	return fopen( str1.c_str(), str2.c_str() );
#endif
}


////////////////////////////////////////////////////////////////
// ファイルストリームを開く
//
// 引数:	fs				ファイルストリームへの参照
//			path			パス
//			mode			モード
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_FSopen( std::fstream& fs, const P6VPATH& path, const std::ios_base::openmode mode )
{
	PRINTD( OSD_LOG, "[OSD][OSD_FSopen] %s\n", P6VPATH2STR( path ).c_str() );
	
#ifdef	USEFILESYSTEM
	fs.open( path, mode );
	return fs.is_open() && fs.good();
#else
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	fs.open( tpath, mode );
	return fs.is_open() && fs.good();
#endif
}


////////////////////////////////////////////////////////////////
// フォルダを作成
//
// 引数:	path			パス
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_CreateFolder( const P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][OSD_CreateFolder] %s\n", P6VPATH2STR( path ).c_str() );
	
#ifdef	USEFILESYSTEM
	try{
		std::error_code ec;
		
		P6VPATH tpath = path;
		OSD_AbsolutePath( tpath );
		
		// 設定ファイルパスより外側には作成しない
		if( P6VPATH2STR( tpath ).compare( 0, P6VPATH2STR( OSD_GetConfigPath() ).length(), P6VPATH2STR( OSD_GetConfigPath() ) ) ) return false;
		
		return std::filesystem::create_directories( tpath, ec );
	}
	catch( std::filesystem::filesystem_error& ){
		return false;
	}
#else
	if( path.length() > PATH_MAX ) return false;
	
	std::string tpath = path;
	OSD_AddDelimiter( tpath );	// 念のため
	OSD_AbsolutePath( tpath );
	
	// 設定ファイルパスより外側には作成しない
	if( !tpath.compare( 0, OSD_GetConfigPath().length(), OSD_GetConfigPath() ) ){
		OSD_UTF8toSJIS( tpath );
		return MakeSureDirectoryPathExists( tpath.c_str() );
	}
	return false;
#endif
}


////////////////////////////////////////////////////////////////
// ファイルの存在チェック
//
// 引数:	fullpath		パス
// 返値:	bool			true:存在する false:存在しない
////////////////////////////////////////////////////////////////
bool OSD_FileExist( const P6VPATH& fullpath )
{
	PRINTD( OSD_LOG, "[OSD][OSD_FileExist] %s\n", P6VPATH2STR( fullpath ).c_str() );
	
#ifdef	USEFILESYSTEM
	try{
		return std::filesystem::exists( std::filesystem::status( fullpath ) );
	}
	catch( std::filesystem::filesystem_error& ){
		return false;
	}
#else
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	bool ret = false;
	
	std::string tpath = fullpath;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	PathRemoveBackslash( str );
	
	hFind = FindFirstFile( str, &FindFileData );
	if( hFind != INVALID_HANDLE_VALUE ) ret = true;
	FindClose( hFind );
	
	return ret;
#endif
}


////////////////////////////////////////////////////////////////
// ファイルサイズ取得
//
// 引数:	fullpath		パス
// 返値:	DWORD			ファイルサイズ
////////////////////////////////////////////////////////////////
DWORD OSD_GetFileSize( const P6VPATH& fullpath )
{
#ifdef	USEFILESYSTEM
	try{
		return std::filesystem::file_size( fullpath );
	}
	catch( std::filesystem::filesystem_error& ){
		return 0;
	}
#else
	HANDLE hFile;
	DWORD fsize;
	
	if( fullpath.empty() ) return 0;
	
	hFile = CreateFile( fullpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if( hFile == INVALID_HANDLE_VALUE ) return 0;
	
	fsize = GetFileSize( hFile, nullptr );
	
	CloseHandle( hFile );
	
	return fsize;
#endif
}


////////////////////////////////////////////////////////////////
// ファイルの読取り専用チェック
//
// 引数:	fullpath		パス
// 返値:	bool			true:読取り専用 false:読み書き
////////////////////////////////////////////////////////////////
bool OSD_FileReadOnly( const P6VPATH& fullpath )
{
	PRINTD( OSD_LOG, "[OSD][OSD_FileReadOnly] %s\n", P6VPATH2STR( fullpath ).c_str() );
	
#ifdef	USEFILESYSTEM
	try{
		std::filesystem::perms perm = std::filesystem::status( fullpath ).permissions();
		return ( perm & ( std::filesystem::perms::owner_write |
						  std::filesystem::perms::group_write |
						  std::filesystem::perms::others_write ) ) == std::filesystem::perms::none ? true : false;
	}
	catch( std::filesystem::filesystem_error& ){
		return false;
	}
#else
	std::string tpath = fullpath;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	PathRemoveBackslash( str );
	
	DWORD fa = GetFileAttributes( str );
	
	if( fa & FILE_ATTRIBUTE_READONLY ) return true;
	else                               return false;
#endif
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


