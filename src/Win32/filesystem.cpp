// OS依存の汎用ルーチン(ファイル処理)

#include <windows.h>
#include <shlwapi.h>	// ie依存
#include <imagehlp.h>

#include <fstream>
#include <string>

#include "../log.h"
#include "../osd.h"


////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// パスの末尾にデリミタを追加
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_AddDelimiter( P6VPATH& path )
{
	OSD_UTF8toSJIS( path );
	
	char str[path.length()+2];
	
	std::strcpy( str, P6VPATH2STR( path ).c_str() );
	PathAddBackslash( str );
	path = str;
	
	OSD_SJIStoUTF8( path );
}


////////////////////////////////////////////////////////////////
// パスの末尾のデリミタを削除
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_DelDelimiter( P6VPATH& path )
{
	OSD_UTF8toSJIS( path );
	
	char str[path.length()+1];
	
	std::strcpy( str, P6VPATH2STR( path ).c_str() );
	PathRemoveBackslash( str );
	path = str;
	
	OSD_SJIStoUTF8( path );
}


////////////////////////////////////////////////////////////////
// 相対パス化
//
// 引数:	path			パス
// 返値:	なし
////////////////////////////////////////////////////////////////
void OSD_RelativePath( P6VPATH& path )
{
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
	
	char str[PATH_MAX+1];
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	if( tpath.empty() || !PathIsRelative( tpath.c_str() ) ){
		PRINTD( OSD_LOG, "Already Absolute\n" );
		return;
	}
	
	std::string mpath = OSD_GetConfigPath();
	OSD_UTF8toSJIS( mpath );
	
	PathCombine( str, mpath.c_str(), tpath.c_str() );
	path = str;
	OSD_SJIStoUTF8( path );
	
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
	char str[PATH_MAX+1];
	
	std::string tpath1 = path1;
	std::string tpath2 = path2;
	OSD_UTF8toSJIS( tpath1 );
	OSD_UTF8toSJIS( tpath2 );
	
	PathCombine( str, tpath1.c_str(), tpath2.c_str() );
	cpath = str;
	OSD_SJIStoUTF8( cpath );
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
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	PathRemoveFileSpec( str );
	
	tpath = str;
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
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
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	tpath = PathFindFileName( str );
	
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
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
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	tpath = PathFindExtension( str );
	if( tpath.front() == '.' ) tpath.erase( tpath.begin() );
	
	OSD_SJIStoUTF8( tpath );
	
	return tpath;
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
	
	OSD_UTF8toSJIS( path );
	
	std::string text = '.' + ext;
	OSD_UTF8toSJIS( text );
	
	char str[path.length()+text.length()+1];
	
	std::strcpy( str, path.c_str() );
	bool res = PathRenameExtension( str, text.c_str() );
	
	path = str;
	OSD_SJIStoUTF8( path );
	
	return res;
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
	
	std::string str1 = P6VPATH2STR( path );
	std::string str2 = mode;
	OSD_UTF8toSJIS( str1 );
	OSD_UTF8toSJIS( str2 );
	
	return fopen( str1.c_str(), str2.c_str() );
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
	
	std::string tpath = path;
	OSD_UTF8toSJIS( tpath );
	
	fs.open( tpath, mode );
	return fs.is_open() && fs.good();
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
	
	if( path.length() > PATH_MAX ) return false;
	
	std::string tpath = path;
	OSD_AddDelimiter( tpath );	// 念のため(MakeSureDirectoryPathExists関数の要求による)
	OSD_AbsolutePath( tpath );
	PRINTD( OSD_LOG, "-> %s\n", P6VPATH2STR( tpath ).c_str() );
	
	// 設定ファイルパスより外側には作成しない
	if( !tpath.compare( 0, OSD_GetConfigPath().length(), OSD_GetConfigPath() ) ){
		OSD_UTF8toSJIS( tpath );
		return MakeSureDirectoryPathExists( tpath.c_str() );
	}
	return false;
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
}


////////////////////////////////////////////////////////////////
// ファイルサイズ取得
//
// 引数:	fullpath		パス
// 返値:	DWORD			ファイルサイズ
////////////////////////////////////////////////////////////////
DWORD OSD_GetFileSize( const P6VPATH& fullpath )
{
	HANDLE hFile;
	DWORD fsize;
	
	if( fullpath.empty() ) return 0;
	
	hFile = CreateFile( fullpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if( hFile == INVALID_HANDLE_VALUE ) return 0;
	
	fsize = GetFileSize( hFile, nullptr );
	
	CloseHandle( hFile );
	
	return fsize;
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
	
	std::string tpath = fullpath;
	OSD_UTF8toSJIS( tpath );
	
	char str[tpath.length()+1];
	
	std::strcpy( str, tpath.c_str() );
	PathRemoveBackslash( str );
	
	DWORD fa = GetFileAttributes( str );
	
	if( fa & FILE_ATTRIBUTE_READONLY ) return true;
	else                               return false;
}


////////////////////////////////////////////////////////////////
// ファイル名を変更
//
// 引数:	fullpath1		変更元のパス
//			fullpath2		変更するパス
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_FileRename( const P6VPATH& fullpath1, const P6VPATH& fullpath2 )
{
	PRINTD( OSD_LOG, "[OSD][OSD_FileRename] %s -> %s\n", P6VPATH2STR( fullpath1 ).c_str() P6VPATH2STR( fullpath2 ).c_str() );
	
	return MoveFile( P6VPATH2STR( fullpath1 ).c_str(), P6VPATH2STR( fullpath2 ).c_str() );
}


////////////////////////////////////////////////////////////////
// ファイルを削除
//
// 引数:	fullpath		パス
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_FileDelete( const P6VPATH& fullpath )
{
	PRINTD( OSD_LOG, "[OSD][OSD_FileDelete] %s\n", P6VPATH2STR( fullpath ).c_str() );
	
	return DeleteFile( P6VPATH2STR( fullpath ).c_str() );
}


////////////////////////////////////////////////////////////////
// ファイルを探す
//
// 引数:	path			パス
//			file			探すファイル名
//			folders			見つかったパスを格納するvectorへの参照
//			size			ファイルサイズ (0:チェックしない)
// 返値:	bool			true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_FindFile( const P6VPATH& path, const P6VPATH& file, std::vector<P6VPATH>& files, size_t size )
{
	HANDLE hFind;
	WIN32_FIND_DATA fdata;
	
	std::string sfile = OSD_GetFileNamePart( file );
	std::transform( sfile.begin(), sfile.end(), sfile.begin(), ::tolower );	// 小文字
	
	P6VPATH dpath = path;
	OSD_AddDelimiter( dpath );
	OSD_AddPath( dpath, dpath, "*" );
	
	hFind = FindFirstFile( P6VPATH2STR( dpath ).c_str(), &fdata );
	
	if( hFind == INVALID_HANDLE_VALUE ){
		FindClose( hFind );
		return false;
	}
	
	do{
		std::string ff = fdata.cFileName;
		
		if( ff != "." && ff != ".." ){
			if( fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){	// ディレクトリの場合
				P6VPATH npath = path;
				OSD_AddPath( npath, npath, STR2P6VPATH( ff ) );
				OSD_AddDelimiter( npath );
				
				OSD_FindFile( npath, file, files );	// 再帰
			}else{														// ファイルの場合
				P6VPATH fpath;
				OSD_AddPath( fpath, path, STR2P6VPATH( ff ) );
				
				std::string tfile = OSD_GetFileNamePart( fpath );
				std::transform( tfile.begin(), tfile.end(), tfile.begin(), ::tolower );	// 小文字
				
				if( tfile == sfile && (!size || OSD_GetFileSize( fpath ) == size) ){
					files.emplace_back( fpath );
				}
			}
		}
	}while( FindNextFile( hFind, &fdata ) );
	
	FindClose( hFind );
	
	return true;
}

