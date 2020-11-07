// OS依存の汎用ルーチン(主にUI用)

#include <cstdlib>
#include <cstring>
#include <string>

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>	// ie依存

#include "id_config.h"
#include "id_icon.h"
#include "../id_menu.h"
#include "../pc6001v.h"

#include "../breakpoint.h"
#include "../common.h"
#include "../config.h"
#include "../disk.h"
#include "../error.h"
#include "../graph.h"
#include "../joystick.h"
#include "../log.h"
#include "../memory.h"
#include "../osd.h"
#include "../p6el.h"
#include "../schedule.h"
#include "../tape.h"
#include "../vdg.h"


///////////////////////////////////////////////////////////
// ローカル関数定義
///////////////////////////////////////////////////////////
static bool OsdReadINI(  HWND, int );			// 設定を読込む
static bool OsdWriteINI( HWND, int );			// 設定を保存する

static INT_PTR CALLBACK OsdCnfgProc1(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProc2(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProc3(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProc4(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProc5(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProc6(   HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProcCol( HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK OsdCnfgProcEtc( HWND, UINT, WPARAM, LPARAM );
static INT_PTR CALLBACK VerInfoProc(    HWND, UINT, WPARAM, LPARAM );

static void OsdOwnerDrawBtn( HDC, RECT, COLORREF );	// オーナードローボタン 描画

///////////////////////////////////////////////////////////
// スタティック変数
///////////////////////////////////////////////////////////
static CFG6 ecfg;				// 環境設定オブジェクト(編集用)
static int model = 60;			// 機種
static BROWSEINFO OBI;			// FolderDiaog_Win32()で使用する情報を格納する構造体




///////////////////////////////////////////////////////////
// リソースから文字列取得
///////////////////////////////////////////////////////////
const char* GetTextConv( HINSTANCE hinst, int id )
{
	static char txt[1024] = "";
	
	if( !LoadString( hinst, id, txt, sizeof(txt)-1 ) ) return nullptr;
	
	return txt;
}



///////////////////////////////////////////////////////////
// ポップアップメニュー表示
///////////////////////////////////////////////////////////
void EL6::ShowPopupMenu( int x, int y )
{
	// MF_BYPOSITION用
	enum {	MSYSTEM = 0,	// システム
			MSEP1,			// ----------
			MTAPE,			// TAPE
			MDISK,			// DISK
			MEXTROM,		// 拡張ROM
			MCONT,			// コントローラ
			MCONFIG,		// 設定
			MDEBUG,			// デバッグ
			MSEP2,			// ----------
			MHELP			// ヘルプ
		};
	
	POINT pt;
	
	pt.x = x;
	pt.y = y;
	HWND hwnd = (HWND)OSD_GetWindowHandle( GetWindowHandle() );
	
	if( GetMenu( hwnd ) ) return;
	
	HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr( hwnd, GWLP_HINSTANCE);
	HMENU hm  = LoadMenu( hinst, MAKEINTRESOURCE( ID_MENU ) );
	HMENU hsm = GetSubMenu( hm, MSYSTEM );
	
	// メニューの前処理
	MENUITEMINFO minfo;
	minfo.cbSize = sizeof(MENUITEMINFO);
	
	// ビデオキャプチャ
	minfo.fMask      = MIIM_TYPE;
	minfo.dwTypeData = nullptr;
	GetMenuItemInfo( hsm, ID_AVISAVE, MF_BYCOMMAND, &minfo );
	minfo.dwTypeData = (char*)(AVI6::IsAVI() ? GetTextConv( hinst, IDS_AVI1 ) : GetTextConv( hinst, IDS_AVI0 ));
	SetMenuItemInfo( hsm, ID_AVISAVE, MF_BYCOMMAND, &minfo );
	// モニタモード or ブレークポインタが設定されていたらビデオキャプチャ無効
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if( vm->IsMonitor() || vm->bp->GetNum() )
		EnableMenuItem( hsm, ID_AVISAVE, MF_BYCOMMAND | MF_GRAYED );
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	// リプレイ記録
	minfo.fMask      = MIIM_TYPE;
	minfo.dwTypeData = nullptr;
	GetMenuItemInfo( hsm, ID_REPLAYSAVE, MF_BYCOMMAND, &minfo );
	minfo.dwTypeData = (char*)(( REPLAY::GetStatus() == REP_RECORD ) ? GetTextConv( hinst, IDS_REP1 ) : GetTextConv( hinst, IDS_REP0 ));
	SetMenuItemInfo( hsm, ID_REPLAYSAVE, MF_BYCOMMAND, &minfo );
	// モニタモード or ブレークポインタが設定されている
	// またはリプレイ再生中だったらリプレイ記録無効
	if( 
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		vm->IsMonitor() || vm->bp->GetNum() ||
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		( REPLAY::GetStatus() == REP_REPLAY ) ){
		EnableMenuItem( hsm, ID_REPLAYSAVE,   MF_BYCOMMAND | MF_GRAYED );
		EnableMenuItem( hsm, ID_REPLAYRESUME, MF_BYCOMMAND | MF_GRAYED );
	}
	// 途中保存、やり直しはリプレイ記録中のみ
	if(
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		vm->IsMonitor() || vm->bp->GetNum() ||
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		( REPLAY::GetStatus() != REP_RECORD ) ){
		EnableMenuItem( hsm, ID_REPLAYDOKOSAVE, MF_BYCOMMAND | MF_GRAYED );
		EnableMenuItem( hsm, ID_REPLAYDOKOLOAD, MF_BYCOMMAND | MF_GRAYED );
	}
	
	// リプレイ再生
	minfo.fMask      = MIIM_TYPE;
	minfo.dwTypeData = nullptr;
	GetMenuItemInfo( hsm, ID_REPLAYLOAD, MF_BYCOMMAND, &minfo );
	minfo.dwTypeData = (char*)(( REPLAY::GetStatus() == REP_REPLAY ) ? GetTextConv( hinst, IDS_REP3 ): GetTextConv( hinst, IDS_REP2 ));
	SetMenuItemInfo( hsm, ID_REPLAYLOAD, MF_BYCOMMAND, &minfo );
	// モニタモード or ブレークポインタが設定されている
	// またはリプレイ記録中だったらリプレイ再生無効
	if(
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		vm->IsMonitor() || vm->bp->GetNum() ||
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		( REPLAY::GetStatus() == REP_RECORD ) ){
		EnableMenuItem( hsm, ID_REPLAYLOAD,   MF_BYCOMMAND | MF_GRAYED );
		EnableMenuItem( hsm, ID_REPLAYRESUME, MF_BYCOMMAND | MF_GRAYED );
	}
	
	// TAPE
	EnableMenuItem( hsm, ID_TAPEEJECT, MF_BYCOMMAND | vm->cmtl->GetFile().empty() ? MF_GRAYED : MF_ENABLED );
	
	// DISK
	switch( vm->disk->GetDrives() ){
	case 0:
		DeleteMenu( hsm, MDISK, MF_BYPOSITION );
		break;
	case 1:
		DeleteMenu( GetSubMenu( hsm, MDISK ), 1, MF_BYPOSITION );
		EnableMenuItem( hsm, ID_DISKEJECT1, MF_BYCOMMAND | vm->disk->GetFile(0).empty() ? MF_GRAYED : MF_ENABLED );
		break;
	default:
		EnableMenuItem( hsm, ID_DISKEJECT1, MF_BYCOMMAND | vm->disk->GetFile(0).empty() ? MF_GRAYED : MF_ENABLED );
		EnableMenuItem( hsm, ID_DISKEJECT2, MF_BYCOMMAND | vm->disk->GetFile(1).empty() ? MF_GRAYED : MF_ENABLED );
	}
	
	// 拡張ROM
	EnableMenuItem( hsm, ID_ROMEJECT, MF_BYCOMMAND | vm->mem->GetFile().empty() ? MF_GRAYED : MF_ENABLED );
	
	// コントローラ
	for( int i=0; i < 5; i++ ){
		if( i < OSD_GetJoyNum() ){
			EnableMenuItem( hsm, ID_JOY101 + i, MF_BYCOMMAND | MF_ENABLED );
			EnableMenuItem( hsm, ID_JOY201 + i, MF_BYCOMMAND | MF_ENABLED );
			
			minfo.fMask      = MIIM_TYPE;
			minfo.dwTypeData = (char*)OSD_GetJoyName( i ).c_str();
			SetMenuItemInfo( hsm, ID_JOY101 + i, MF_BYCOMMAND, &minfo );
			SetMenuItemInfo( hsm, ID_JOY201 + i, MF_BYCOMMAND, &minfo );
		}else{
			DeleteMenu( hsm, ID_JOY101 + i, MF_BYCOMMAND );
			DeleteMenu( hsm, ID_JOY201 + i, MF_BYCOMMAND );
		}
	}
	CheckMenuRadioItem( hsm, ID_JOY100, ID_JOY105, (joy->GetID(0) < 0) ? ID_JOY100 : ID_JOY101 + joy->GetID(0), MF_BYCOMMAND );
	CheckMenuRadioItem( hsm, ID_JOY200, ID_JOY205, (joy->GetID(1) < 0) ? ID_JOY200 : ID_JOY201 + joy->GetID(1), MF_BYCOMMAND );
	
	// MODE4カラー
	CheckMenuRadioItem( hsm, ID_M4MONO, ID_M4GRPK, ID_M4MONO + vm->vdg->GetMode4Color(), MF_BYCOMMAND );
	
	// フレームスキップ
	CheckMenuRadioItem( hsm, ID_FSKP0, ID_FSKP5, ID_FSKP0 + cfg->GetValue( CV_FrameSkip ), MF_BYCOMMAND );
	
	// ウィンドウ表示倍率
	if( !(cfg->GetValue( CV_WindowZoom ) % 100) ){
		CheckMenuRadioItem( hsm, ID_ZOOM100, ID_ZOOM300, ID_ZOOM100 + cfg->GetValue( CV_WindowZoom ) / 100 - 1, MF_BYCOMMAND );
	}
	
	// サンプリングレート
	CheckMenuRadioItem( hsm, ID_SPR44, ID_SPR11, ID_SPR11 - ((cfg->GetValue( CV_SampleRate )/11025)>>1), MF_BYCOMMAND );
	
	CheckMenuItem( hsm, ID_NOWAIT,    sche->GetWaitEnable()               ? MF_UNCHECKED : MF_CHECKED   );
	CheckMenuItem( hsm, ID_TURBO,     cfg->GetValue( CB_TurboTAPE )  ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_BOOST,     vm->cmtl->IsBoostUp()               ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_FULLSCRN,  cfg->GetValue( CB_FullScreen ) ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_STATUS,    cfg->GetValue( CB_DispStatus ) ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_DISP43,    cfg->GetValue( CB_DispNTSC )   ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_SCANLINE,  cfg->GetValue( CB_ScanLine )   ? MF_CHECKED   : MF_UNCHECKED );
	CheckMenuItem( hsm, ID_FILTERING, cfg->GetValue( CB_Filtering )  ? MF_CHECKED   : MF_UNCHECKED );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	CheckMenuItem( hsm, ID_MONITOR,   vm->IsMonitor()		? MF_CHECKED   : MF_UNCHECKED );
	#else
	DeleteMenu( hsm, MDEBUG, MF_BYPOSITION );
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	// フルスクリーン時はダイアログを表示するコマンドを封印
	
	// 環境設定
	EnableMenuItem( hsm, ID_CONFIG,  MF_BYCOMMAND | cfg->GetValue( CB_FullScreen ) ? MF_GRAYED : MF_ENABLED );
	
	// バージョン情報
	EnableMenuItem( hsm, ID_VERSION, MF_BYCOMMAND | cfg->GetValue( CB_FullScreen ) ? MF_GRAYED : MF_ENABLED );
	
	
	// ポップアップメニュー表示
	ClientToScreen( hwnd, &pt );
	int id = TrackPopupMenu( hsm, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr );
	DestroyMenu( hm );
	
	// 項目ごとの処理
	ExecMenu( id );
}









////////////////////////////////////////////////////////////////
// FolderDiaog_Win32()で使用するコールバックプロシージャ
////////////////////////////////////////////////////////////////
static int CALLBACK OsdBrowseCallbackProc( HWND hwnd, UINT msg, LPARAM lp1, LPARAM lp2 )
{
	char Path[PATH_MAX*2];	// 念のため2倍?
	
	// BIF_STATUSTEXT以外は何もしない
	if( OBI.ulFlags & BIF_STATUSTEXT ){
		switch( msg ){
		case BFFM_INITIALIZED:	// 初期化
			// 初期フォルダを設定
			SendMessage( hwnd, BFFM_SETSELECTION, (WPARAM)true, lp2 );
			break;
		case BFFM_SELCHANGED:	// ユーザーがフォルダを変更した
			// ITEMIDLIST構造体からパス名を取り出す
			SHGetPathFromIDList( (LPCITEMIDLIST)lp1, Path );
			// 変更されたフォルダのパスを表示する
			SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)Path );
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////
// フォルダの参照(Win32)
//
// 引数:	hwnd		親のウィンドウハンドル(Win32)
//			path		パス
// 返値:	bool		true:選択成功 false:エラーorキャンセル
////////////////////////////////////////////////////////////////
bool FolderDiaog_Win32( HWND hwnd, P6VPATH& path )
{
	LPMALLOC Memory;
	LPITEMIDLIST Ret, Root;
	char Buffer[PATH_MAX+1];
	char stitle[PATH_MAX+1];
	bool res = false;
	std::string tpath;
	
	tpath = P6VPATH2STR( path );
	OSD_UTF8toSJIS( tpath );
	
	// インスタンス取得
	HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr( hwnd, GWLP_HINSTANCE);
	
	// シェルのIMallocインタフェースにポインタを取得
	SHGetMalloc( &Memory );
	
	//「マイコンピュータ」フォルダの位置を取得する
	SHGetSpecialFolderLocation( hwnd, CSIDL_DRIVES, &Root );
	
	// リソースからタイトル(解説文)取得
	LoadString( hinst, IDS_FSTITLE, stitle, PATH_MAX );
	
	
	//BrowseInfo構造体の初期設定
	ZeroMemory( &OBI, sizeof(BROWSEINFO) );
	OBI.hwndOwner = hwnd;						// 親ウインドウのハンドル
	OBI.pidlRoot  = Root;						// ルートフォルダ(CSIDL_××)
	OBI.ulFlags   = BIF_STATUSTEXT | BIF_RETURNONLYFSDIRS;	// フォルダのタイプを示すフラグﾞ(BIF_××)
	OBI.lpszTitle = stitle;						// タイトル(解説文)
	OBI.pszDisplayName = Buffer;				// (戻り値)フォルダ名
	OBI.lpfn = OsdBrowseCallbackProc;			// コールバック関数のエントリポイント
	OBI.lParam = (LPARAM)tpath.c_str();			// コールバック関数に渡す引数
   	
	//「フォルダの参照」ダイアログを表示
	Ret = SHBrowseForFolder( &OBI );
	
	if( SHGetPathFromIDList( Ret, Buffer ) ){
		PathRemoveBackslash( Buffer );
		tpath = Buffer;
		OSD_SJIStoUTF8( tpath );
		path = STR2P6VPATH( tpath );
		res = true;
	}
	
	//メモリの解放
	Memory->Free( Ret );
	Memory->Free( Root );
	
	return res;
}


////////////////////////////////////////////////////////////////
// フォルダの参照
//
// 引数:	hwnd		親のウィンドウハンドル
//			path		パス
// 返値:	bool		true:選択成功 false:エラーorキャンセル
////////////////////////////////////////////////////////////////
bool OSD_FolderDiaog( HWINDOW hwnd, P6VPATH& path )
{
	return FolderDiaog_Win32( (HWND)OSD_GetWindowHandle( hwnd ), path );
}


////////////////////////////////////////////////////////////////
// ファイルの参照(Win32)
//
// 引数:	hwnd		親のウィンドウハンドル(Win32)
//			mode		モード FM_Load:ファイルを開く FM_Save:名前を付けて保存
//			title		ウィンドウキャプション文字列への参照(SJIS)
//			filter		ファイルフィルタ文字列へのポインタ(SJIS) ※stringにすると制御文字が失われるので必ずchar*にする※
//			fullpath	フルパス
//			path		ファイル検索パス
//			ext			拡張子文字列への参照
// 返値:	bool		true:選択成功 false:エラーorキャンセル
////////////////////////////////////////////////////////////////
bool FileDiaog_Win32( HWND hwnd, FileMode mode, const std::string& title, const char* filter, P6VPATH& fullpath, P6VPATH& path, const std::string& ext )
{
	PRINTD( OSD_LOG, "[OSD][FileDiaog_Win32] (%s)(%s)(%s)\n", P6VPATH2STR( fullpath ).c_str(), P6VPATH2STR( path ).c_str(), ext.c_str() );
	
	OPENFILENAME fname;
	char File[PATH_MAX+1];
	char Path[PATH_MAX+1];
	bool ret = false;
	MSG msg;
	std::string tpath;
	
	ZeroMemory( File, sizeof(File) );
	ZeroMemory( Path, sizeof(Path) );
	
	if( OSD_FileExist( fullpath ) ){
		tpath = P6VPATH2STR( fullpath );
		OSD_UTF8toSJIS( tpath );
		std::strncpy( File, tpath.c_str(), sizeof(File)-1 );
	}else{
		tpath = P6VPATH2STR( path );
		OSD_UTF8toSJIS( tpath );
		std::strncpy( Path, tpath.c_str(), sizeof(Path)-1 );
	}
	ZeroMemory( &fname, sizeof(OPENFILENAME) );
	
	fname.lStructSize     = sizeof(OPENFILENAME);
	fname.hwndOwner       = hwnd;						// 親のウィンドウハンドル
	fname.lpstrFilter     = filter;						// ファイルフィルタ
	fname.nFilterIndex    = 1;							// 1番目のファイルフィルタを使う
	fname.lpstrFile       = File;						// 選択されたフルパスの格納先
	fname.nMaxFile        = sizeof(File);				// そのサイズ
	fname.lpstrInitialDir = Path;						// 初期フォルダ
	fname.lpstrTitle      = title.c_str();				// タイトル
	fname.lpstrDefExt     = ext.c_str();				// 拡張子が省略されている場合に追加する拡張子
//	fname.Flags           = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_LONGNAMES;
	
	if( mode == FM_Save ){
		fname.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_LONGNAMES;
		ret = GetSaveFileName( &fname );
	}else{
		fname.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_LONGNAMES;
		ret = GetOpenFileName( &fname );
	}
	
	// キーボード，マウス関連のメッセージ削除(ダブルクリック対策)
	while( PeekMessage( &msg, hwnd, 0, 0, PM_REMOVE | PM_QS_INPUT ) );
	
	if( ret ){
		// フルパスを保存
		tpath = File;
		OSD_SJIStoUTF8( tpath );
		fullpath = STR2P6VPATH( tpath );
		
		// パスを保存
		PathRemoveFileSpec( File );
		PathAddBackslash( File );
		tpath = File;
		OSD_SJIStoUTF8( tpath );
		path = STR2P6VPATH( tpath );
		
		return true;
	}
	return false;
}


////////////////////////////////////////////////////////////////
// 各種ファイル選択(Win32)
//
// 引数:	hwnd		親のウィンドウハンドル(Win32)
//			type		ダイアログの種類(FileDlg参照)
//			fullpath	フルパス
//			path		ファイル検索パス
// 返値:	bool		true:選択成功 false:エラーorキャンセル
////////////////////////////////////////////////////////////////
bool FileSelect_Win32( HWND hwnd, FileDlg type, P6VPATH& fullpath, P6VPATH& path )
{
	PRINTD( OSD_LOG, "[OSD][FileSelect_Win32] (%s)(%s)\n", P6VPATH2STR( fullpath ).c_str(), P6VPATH2STR( path ).c_str() );
	
	FileMode mode = FM_Load;
	char title[256];
	char filter[256];
	char ext[16];
	int idt, idf, idx;
	
	HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr( hwnd, GWLP_HINSTANCE);
	
	switch( type ){
	case FD_TapeLoad:	// TAPE(LOAD)選択
		mode = FM_Load;
		idt  = IDS_FSTLT;
		idf  = IDS_FSTLF;
		idx  = IDS_FSTLX;
		break;
		
	case FD_TapeSave:	// TAPE(SAVE)選択
		mode = FM_Save;
		idt  = IDS_FSTST;
		idf  = IDS_FSTSF;
		idx  = IDS_FSTSX;
		break;
		
	case FD_Disk:		// DISK選択
		mode = FM_Load;
		idt  = IDS_FSDKT;
		idf  = IDS_FSDKF;
		idx  = IDS_FSDKX;
		break;
		
	case FD_ExtRom:		// 拡張ROM選択
		mode = FM_Load;
		idt  = IDS_FSRMT;
		idf  = IDS_FSRMF;
		idx  = IDS_FSRMX;
		break;
		
	case FD_Printer:	// プリンター出力ファイル選択
		mode = FM_Save;
		idt  = IDS_FSPRT;
		idf  = IDS_FSPRF;
		idx  = IDS_FSPRX;
		break;
		
	case FD_FontZ:		// 全角フォントファイル選択
		mode = FM_Load;
		idt  = IDS_FSZFT;
		idf  = IDS_FSZFF;
		idx  = IDS_FSZFX;
		break;
		
	case FD_FontH:		// 半角フォントファイル選択
		mode = FM_Load;
		idt  = IDS_FSHFT;
		idf  = IDS_FSHFF;
		idx  = IDS_FSHFX;
		break;
		
	case FD_DokoLoad:	// どこでもLOADファイル選択
		mode = FM_Load;
		idt  = IDS_FSDLT;
		idf  = IDS_FSDLF;
		idx  = IDS_FSDLX;
		break;
		
	case FD_DokoSave:	// どこでもSAVEファイル選択
		mode = FM_Save;
		idt  = IDS_FSDST;
		idf  = IDS_FSDSF;
		idx  = IDS_FSDSX;
		break;
		
	case FD_RepLoad:	// リプレイ再生ファイル選択
		mode = FM_Load;
		idt  = IDS_FSRLT;
		idf  = IDS_FSRLF;
		idx  = IDS_FSRLX;
		break;
		
	case FD_RepSave:	// リプレイ保存ファイル選択
		mode = FM_Save;
		idt  = IDS_FSRST;
		idf  = IDS_FSRSF;
		idx  = IDS_FSRSX;
		break;
		
	case FD_AVISave:	// ビデオキャプチャ出力ファイル選択
		mode = FM_Save;
		idt  = IDS_FSVCT;
		idf  = IDS_FSVCF;
		idx  = IDS_FSVCX;
		break;
		
	case FD_LoadAll:	// 汎用LOAD
	default:
		mode = FM_Load;
		idt  = IDS_FSCMT;
		idf  = IDS_FSCMF;
		idx  = IDS_FSCMX;
		break;
	}
	
	LoadString( hinst, idt, title,  sizeof(title)-1 );
	LoadString( hinst, idf, filter, sizeof(filter)-1 );
	LoadString( hinst, idx, ext,    sizeof(ext)-1 );
	
	return FileDiaog_Win32( hwnd, mode, title, filter, fullpath, path, ext );
}


////////////////////////////////////////////////////////////////
// 各種ファイル選択
//
// 引数:	hwnd		親のウィンドウハンドル
//			type		ダイアログの種類(FileDlg参照)
//			fullpath	フルパス
//			path		ファイル検索パス
// 返値:	bool		true:選択成功 false:エラーorキャンセル
////////////////////////////////////////////////////////////////
bool OSD_FileSelect( HWINDOW hwnd, FileDlg type, P6VPATH& fullpath, P6VPATH& path )
{
	return FileSelect_Win32( (HWND)OSD_GetWindowHandle( hwnd ), type, fullpath, path );
}


////////////////////////////////////////////////////////////////
// メッセージ表示(Win32)
//
// 引数:	hwnd		親のウィンドウハンドル(Win32)
//			mes			メッセージ文字列への参照(UTF-8)
//			cap			ウィンドウキャプション文字列への参照(UTF-8)
//			type		表示形式指示のフラグ
// 返値:	int			押されたボタンの種類
//							OSDR_OK:     OKボタン
//							OSDR_CANCEL: CANCELボタン
//							OSDR_YES:    YESボタン
//							OSDR_NO:     NOボタン
////////////////////////////////////////////////////////////////
int Message_Win32( HWND hwnd, const std::string& mes, const std::string& cap, int type )
{
	int Type = MB_OK;
	std::string tmes = mes;
	OSD_UTF8toSJIS( tmes );
	std::string tcap = cap;
	OSD_UTF8toSJIS( tcap );
	
	// メッセージボックスのタイプ
	switch( type&0x000f ){
	case OSDM_OK:			Type = MB_OK;			break;
	case OSDM_OKCANCEL:		Type = MB_OKCANCEL;		break;
	case OSDM_YESNO:		Type = MB_YESNO;		break;
	case OSDM_YESNOCANCEL:	Type = MB_YESNOCANCEL;	break;
	}
	
	// メッセージボックスのアイコンタイプ
	switch( type&0x00f0 ){
	case OSDM_ICONERROR:	Type |= MB_ICONERROR;		break;
	case OSDM_ICONQUESTION:	Type |= MB_ICONQUESTION;	break;
	case OSDM_ICONWARNING:	Type |= MB_ICONWARNING;		break;
	case OSDM_ICONINFO:		Type |= MB_ICONINFORMATION;	break;
	}
	
	int res = MessageBox( hwnd, tmes.c_str(), tcap.c_str(), Type | MB_TOPMOST );
	
	switch( res ){
	case IDOK:	return OSDR_OK;
	case IDYES:	return OSDR_YES;
	case IDNO:	return OSDR_NO;
	default:	return OSDR_CANCEL;
	}
}


////////////////////////////////////////////////////////////////
// メッセージ表示
//
// 引数:	hwnd		親のウィンドウハンドル
//			mes			メッセージ文字列への参照(UTF-8)
//			cap			ウィンドウキャプション文字列への参照(UTF-8)
//			type		表示形式指示のフラグ
// 返値:	int			押されたボタンの種類
//							OSDR_OK:     OKボタン
//							OSDR_CANCEL: CANCELボタン
//							OSDR_YES:    YESボタン
//							OSDR_NO:     NOボタン
////////////////////////////////////////////////////////////////
int OSD_Message( HWINDOW hwnd, const std::string& mes, const std::string& cap, int type )
{
	return Message_Win32( (HWND)OSD_GetWindowHandle( hwnd ), mes, cap, type );
}


///////////////////////////////////////////////////////////
// 環境設定ダイアログ表示
//
// 引数:	hwnd		ウィンドウハンドル
// 返値:	int			1:OK 0:CANCEL -1:ERROR
///////////////////////////////////////////////////////////
int OSD_ConfigDialog( HWINDOW hwnd )
{
	// INIファイルを開く
	ecfg.Init();
	
	HWND hhwnd      = (HWND)OSD_GetWindowHandle( hwnd );
	HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr( hhwnd, GWLP_HINSTANCE);
	
	// ページ毎の設定を行なう
	PROPSHEETPAGE psp[9];
	PROPSHEETHEADER psh;
	
	// 基本
	psp[0].dwSize      = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags     = PSP_DEFAULT;
	psp[0].hInstance   = hinst;
	psp[0].pszTemplate = MAKEINTRESOURCE(ID_CNFG1);
	psp[0].pszIcon     = nullptr;
	psp[0].pfnDlgProc  = (DLGPROC)OsdCnfgProc1;
	psp[0].pszTitle    = nullptr;
	psp[0].lParam      = 0;
	
	// 画面
	psp[1].dwSize      = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags     = PSP_DEFAULT;
	psp[1].hInstance   = hinst;
	psp[1].pszTemplate = MAKEINTRESOURCE(ID_CNFG2);
	psp[1].pszIcon     = nullptr;
	psp[1].pfnDlgProc  = (DLGPROC)OsdCnfgProc2;
	psp[1].pszTitle    = nullptr;
	psp[1].lParam      = 0;
	
	// サウンド
	psp[2].dwSize      = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags     = PSP_DEFAULT;
	psp[2].hInstance   = hinst;
	psp[2].pszTemplate = MAKEINTRESOURCE(ID_CNFG3);
	psp[2].pszIcon     = nullptr;
	psp[2].pfnDlgProc  = (DLGPROC)OsdCnfgProc3;
	psp[2].pszTitle    = nullptr;
	psp[2].lParam      = 0;
	
	// 入力関係
	psp[3].dwSize      = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags     = PSP_DEFAULT;
	psp[3].hInstance   = hinst;
	psp[3].pszTemplate = MAKEINTRESOURCE(ID_CNFGIN);
	psp[3].pszIcon     = nullptr;
	psp[3].pfnDlgProc  = (DLGPROC)OsdCnfgProc4;
	psp[3].pszTitle    = nullptr;
	psp[3].lParam      = 0;
	
	// ファイル
	psp[4].dwSize      = sizeof(PROPSHEETPAGE);
	psp[4].dwFlags     = PSP_DEFAULT;
	psp[4].hInstance   = hinst;
	psp[4].pszTemplate = MAKEINTRESOURCE(ID_CNFG4);
	psp[4].pszIcon     = nullptr;
	psp[4].pfnDlgProc  = (DLGPROC)OsdCnfgProc5;
	psp[4].pszTitle    = nullptr;
	psp[4].lParam      = 0;
	
	// フォルダ
	psp[5].dwSize      = sizeof(PROPSHEETPAGE);
	psp[5].dwFlags     = PSP_DEFAULT;
	psp[5].hInstance   = hinst;
	psp[5].pszTemplate = MAKEINTRESOURCE(ID_CNFG5);
	psp[5].pszIcon     = nullptr;
	psp[5].pfnDlgProc  = (DLGPROC)OsdCnfgProc6;
	psp[5].pszTitle    = nullptr;
	psp[5].lParam      = 0;
	
	// 色1
	psp[6].dwSize      = sizeof(PROPSHEETPAGE);
	psp[6].dwFlags     = PSP_DEFAULT;
	psp[6].hInstance   = hinst;
	psp[6].pszTemplate = MAKEINTRESOURCE(ID_CNFGCL1);
	psp[6].pszIcon     = nullptr;
	psp[6].pfnDlgProc  = (DLGPROC)OsdCnfgProcCol;
	psp[6].pszTitle    = nullptr;
	psp[6].lParam      = 0;
	
	// 色2
	psp[7].dwSize      = sizeof(PROPSHEETPAGE);
	psp[7].dwFlags     = PSP_DEFAULT;
	psp[7].hInstance   = hinst;
	psp[7].pszTemplate = MAKEINTRESOURCE(ID_CNFGCL2);
	psp[7].pszIcon     = nullptr;
	psp[7].pfnDlgProc  = (DLGPROC)OsdCnfgProcCol;
	psp[7].pszTitle    = nullptr;
	psp[7].lParam      = 0;
	
	// その他
	psp[8].dwSize      = sizeof(PROPSHEETPAGE);
	psp[8].dwFlags     = PSP_DEFAULT;
	psp[8].hInstance   = hinst;
	psp[8].pszTemplate = MAKEINTRESOURCE(ID_CNFGETC);
	psp[8].pszIcon     = nullptr;
	psp[8].pfnDlgProc  = (DLGPROC)OsdCnfgProcEtc;
	psp[8].pszTitle    = nullptr;
	psp[8].lParam      = 0;
	
	
	psh.dwSize     = sizeof(PROPSHEETHEADER);
	psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hhwnd;
	psh.hInstance  = hinst;
	psh.pszIcon    = nullptr;
	psh.pszCaption = GetTextConv( hinst, IDS_CFTITLE );
	psh.nPages     = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp       = (LPCPROPSHEETPAGE) &psp;
	
	// プロパティシート表示
	int ret = PropertySheet( &psh );
	
	// OKボタンが押されたならINIファイル書込み
	if( ret > 0) ecfg.Write();
	
	return ret;
}


///////////////////////////////////////////////////////////
// テキストボックスにパスを設定
///////////////////////////////////////////////////////////
void SetTextBoxPath( HWND hwnd, int id, const P6VPATH& path )
{
	P6VPATH tpath = path;
	OSD_DelDelimiter( tpath );
	std::string str = P6VPATH2STR( tpath );
	OSD_UTF8toSJIS( str );
	SetDlgItemText( hwnd, id, str.c_str() );
}

///////////////////////////////////////////////////////////
// テキストボックスからパスを取得
///////////////////////////////////////////////////////////
P6VPATH GetTextBoxPath( HWND hwnd, int id )
{
	char str[PATH_MAX+1];			// 文字列取得用
	
	GetDlgItemText( hwnd, id, str, sizeof(str) );
	std::string tstr = str;
	OSD_SJIStoUTF8( tstr );
	return STR2P6VPATH( tstr );
}

///////////////////////////////////////////////////////////
// ファイルを選択してテキストボックスに設定
///////////////////////////////////////////////////////////
void FileSelCom( HWND hwnd, int id, FileDlg type, const P6VPATH& path )
{
	char str[PATH_MAX+1];			// 文字列取得用
	std::string tstr;				// 文字列取得用
	P6VPATH folder, fpath;			// パス取得用
	
	GetDlgItemText( hwnd, id, str, sizeof(str) );
	tstr = str;
	OSD_SJIStoUTF8( tstr );
	folder = STR2P6VPATH( tstr );
	fpath  = path;
	if( FileSelect_Win32( hwnd, type, folder, fpath ) ){
		tstr = P6VPATH2STR( folder );
		OSD_UTF8toSJIS( tstr );
		SetDlgItemText( hwnd, id, tstr.c_str() );
	}
}

///////////////////////////////////////////////////////////
// フォルダを選択してテキストボックスに設定
///////////////////////////////////////////////////////////
void FolderSelCom( HWND hwnd, int id )
{
	char str[PATH_MAX+1];			// 文字列取得用
	std::string tstr;				// 文字列取得用
	P6VPATH folder;					// パス取得用
	
	GetDlgItemText( hwnd, id, str, sizeof(str) );
	tstr = str;
	OSD_SJIStoUTF8( tstr );
	folder = STR2P6VPATH( tstr );
	if( FolderDiaog_Win32( hwnd, folder ) ){
		tstr = P6VPATH2STR( folder );
		OSD_UTF8toSJIS( tstr );
		SetDlgItemText( hwnd, id, tstr.c_str() );
	}
}


enum { PP_BASE, PP_DISP, PP_SOUND, PP_FILE, PP_FOLDER, PP_COL, PP_ETC, PP_INPUT };

///////////////////////////////////////////////////////////
// 設定を読込む
///////////////////////////////////////////////////////////
static bool OsdReadINI( HWND hwnd, int page )
{
	int st;							// 状態取得用
	bool yn;						// 状態取得用
	
	switch( page ){
	case PP_BASE:	// 基本
		// 機種
		st = ecfg.GetValue( CV_Model );
		SendMessage( GetDlgItem( hwnd, ID_RB01 ), BM_SETCHECK, (st==60)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB02 ), BM_SETCHECK, (st==61)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB03 ), BM_SETCHECK, (st==62)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB04 ), BM_SETCHECK, (st==66)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB05 ), BM_SETCHECK, (st==64)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB06 ), BM_SETCHECK, (st==68)? BST_CHECKED:BST_UNCHECKED, 0 );
		
		// FDD
		st = ecfg.GetValue( CV_FDD );
		SendMessage( GetDlgItem( hwnd, ID_RB31 ), BM_SETCHECK, (st==0)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB32 ), BM_SETCHECK, (st==1)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB33 ), BM_SETCHECK, (st==2)? BST_CHECKED:BST_UNCHECKED, 0 );
		
		// 拡張RAM使用
		yn = ecfg.GetValue( CB_ExtRam );
		SendMessage( GetDlgItem( hwnd, ID_CB1 ), BM_SETCHECK, yn, 0 );
		
		// 戦士のカートリッジ使用
		st = ecfg.GetValue( CV_Soldier );
		SendMessage( GetDlgItem( hwnd, ID_CB13 ), BM_SETCHECK, st ? true : false, 0 );
		
		break;
		
	case PP_DISP:	// 画面
		// MODE4カラー
		st = ecfg.GetValue( CV_Mode4Color );
		SendMessage( GetDlgItem( hwnd, ID_RB11 ), BM_SETCHECK, (st==0)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB12 ), BM_SETCHECK, (st==1)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB13 ), BM_SETCHECK, (st==2)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB14 ), BM_SETCHECK, (st==3)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB15 ), BM_SETCHECK, (st==4)? BST_CHECKED:BST_UNCHECKED, 0 );
		
		// ビデオキャプチャ色深度
		st = ecfg.GetValue( CV_AviBpp );
		SendMessage( GetDlgItem( hwnd, ID_RB07 ), BM_SETCHECK, (st==16)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB08 ), BM_SETCHECK, (st==24)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB09 ), BM_SETCHECK, (st==32)? BST_CHECKED:BST_UNCHECKED, 0 );
		
		// スキャンライン
		yn = ecfg.GetValue( CB_ScanLine );
		SendMessage( GetDlgItem( hwnd, ID_CB2 ), BM_SETCHECK, yn, 0 );
		
		// スキャンライン輝度
		st = ecfg.GetValue( CV_ScanLineBr );
		SetDlgItemText( hwnd, ID_SCANLINEBR, std::to_string( st ).c_str() );
		
		// フィルタリング
		yn = ecfg.GetValue( CB_Filtering );
		SendMessage( GetDlgItem( hwnd, ID_CB15 ), BM_SETCHECK, yn, 0 );
		
		// 4:3表示
		yn = ecfg.GetValue( CB_DispNTSC );
		SendMessage( GetDlgItem( hwnd, ID_CB7 ), BM_SETCHECK, yn, 0 );
		
		// フルスクリーン
		yn = ecfg.GetValue( CB_FullScreen );
		SendMessage( GetDlgItem( hwnd, ID_CB10 ), BM_SETCHECK, yn, 0 );
		
		// ステータスバー表示状態
		yn = ecfg.GetValue( CB_DispStatus );
		SendMessage( GetDlgItem( hwnd, ID_CB11 ), BM_SETCHECK, yn, 0 );
		
		// フレームスキップ
		st = ecfg.GetValue( CV_FrameSkip );
		SendMessage( GetDlgItem( hwnd, ID_TBFPS ), TBM_SETPOS, true, st );
		SendMessage( GetDlgItem( hwnd, ID_TBFPS ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBFPS ), TBM_SETRANGE, true, MAKELPARAM(0,5) );
		
		break;
		
	case PP_SOUND:	// サウンド
		// サンプリングレート
		st = ecfg.GetValue( CV_SampleRate );
		SendMessage( GetDlgItem( hwnd, ID_RB21 ), BM_SETCHECK, (st==44100)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB22 ), BM_SETCHECK, (st==22050)? BST_CHECKED:BST_UNCHECKED, 0 );
		SendMessage( GetDlgItem( hwnd, ID_RB23 ), BM_SETCHECK, (st==11025)? BST_CHECKED:BST_UNCHECKED, 0 );
		
		// バッファサイズ
		st = ecfg.GetValue( CV_SoundBuffer );
		SendMessage( GetDlgItem( hwnd, ID_TBBUF ), TBM_SETPOS, true, st );
		SendMessage( GetDlgItem( hwnd, ID_TBBUF ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBBUF ), TBM_SETRANGE, true, MAKELPARAM(1,5) );
		
		// PSG LPFカットオフ周波数
		st = ecfg.GetValue( CV_PsgLPF );
		SetDlgItemText( hwnd, ID_EDIT1, std::to_string( st ).c_str() );
		
		// マスター音量
		st = ecfg.GetValue( CV_MasterVol );
		SendMessage( GetDlgItem( hwnd, ID_TBMST ), TBM_SETPOS, true, st/10 );
		SendMessage( GetDlgItem( hwnd, ID_TBMST ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBMST ), TBM_SETRANGE, true, MAKELPARAM(0,10) );
		
		// PSG音量
		st = ecfg.GetValue( CV_PsgVolume );
		SendMessage( GetDlgItem( hwnd, ID_TBPSG ), TBM_SETPOS, true, st/10 );
		SendMessage( GetDlgItem( hwnd, ID_TBPSG ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBPSG ), TBM_SETRANGE, true, MAKELPARAM(0,10) );
		
		// 音声合成音量
		st = ecfg.GetValue( CV_VoiceVolume );
		SendMessage( GetDlgItem( hwnd, ID_TBVCE ), TBM_SETPOS, true, st/10 );
		SendMessage( GetDlgItem( hwnd, ID_TBVCE ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBVCE ), TBM_SETRANGE, true, MAKELPARAM(0,10) );
		
		// TAPEモニタ音量
		st = ecfg.GetValue( CV_TapeVolume );
		SendMessage( GetDlgItem( hwnd, ID_TBTAPE ), TBM_SETPOS, true, st/10 );
		SendMessage( GetDlgItem( hwnd, ID_TBTAPE ), TBM_SETTHUMBLENGTH, 13, 0 );
		SendMessage( GetDlgItem( hwnd, ID_TBTAPE ), TBM_SETRANGE, true, MAKELPARAM(0,10) );
		
		break;
		
	case PP_INPUT:	// 入力関係
		// キーリピート間隔
		st = ecfg.GetValue( CV_KeyRepeat );
		SetDlgItemText( hwnd, ID_KEYREP, std::to_string( st ).c_str() );
		
		break;
		
	case PP_FOLDER:	// フォルダ
		// ROMパス
		SetTextBoxPath( hwnd, ID_PATH1, ecfg.GetValue( CF_RomPath ) );
		
		// TAPEパス
		SetTextBoxPath( hwnd, ID_PATH2, ecfg.GetValue( CF_TapePath ) );
		
		// DISKパス
		SetTextBoxPath( hwnd, ID_PATH3, ecfg.GetValue( CF_DiskPath ) );
		
		// 拡張ROMパス
		SetTextBoxPath( hwnd, ID_PATH4, ecfg.GetValue( CF_ExtRomPath ) );
		
		// IMGパス
		SetTextBoxPath( hwnd, ID_PATH5, ecfg.GetValue( CF_ImgPath ) );
		
		// WAVEパス
		SetTextBoxPath( hwnd, ID_PATH6, ecfg.GetValue( CF_WavePath ) );
		
		// どこでもSAVEパス
		SetTextBoxPath( hwnd, ID_PATH7, ecfg.GetValue( CF_DokoPath ) );
		
		break;
		
	case PP_FILE:	// ファイル
		// 拡張ROMファイル
		SetTextBoxPath( hwnd, ID_FEXROM, ecfg.GetValue( CF_ExtRom ) );
		
		// TAPE(LOAD)ファイル名
		SetTextBoxPath( hwnd, ID_FTPLD, ecfg.GetValue( CF_tape ) );
		
		// TAPE(SAVE)ファイル名
		SetTextBoxPath( hwnd, ID_FTPSV, ecfg.GetValue( CF_save ) );
		
		// DISK1ファイル名
		SetTextBoxPath( hwnd, ID_FDISK1, ecfg.GetValue( CF_disk1 ) );
		
		// DISK2ファイル名
		SetTextBoxPath( hwnd, ID_FDISK2, ecfg.GetValue( CF_disk2 ) );
		
		// プリンタファイル名
		SetTextBoxPath( hwnd, ID_FPRINT, ecfg.GetValue( CF_printer ) );
		
		break;
		
	case PP_COL:	// 色1
		break;
		
	case PP_ETC:	// その他
		// オーバークロック率
		st = min( max( MIN_OVERCLOCK, ecfg.GetValue( CV_OverClock ) ), MAX_OVERCLOCK );
		SetDlgItemText( hwnd, ID_OVERCLK, std::to_string( st ).c_str() );
		
		// CRCチェック
		yn = ecfg.GetValue( CB_CheckCRC );
		SendMessage( GetDlgItem( hwnd, ID_CB4 ), BM_SETCHECK, yn, 0 );
		
		// Turbo TAPE
		yn = ecfg.GetValue( CB_TurboTAPE );
		SendMessage( GetDlgItem( hwnd, ID_CB3 ), BM_SETCHECK, yn, 0 );
		
		// Boost Up
		yn = ecfg.GetValue( CB_BoostUp );
		SendMessage( GetDlgItem( hwnd, ID_CB5 ), BM_SETCHECK, yn, 0 );
		
		// BoostUp 最大倍率(N60モード)
		st = min( max( MIN_BOOST, ecfg.GetValue( CV_MaxBoost60 ) ), MAX_BOOST );
		SetDlgItemText( hwnd, ID_BOOST60, std::to_string( st ).c_str() );
		
		// BoostUp 最大倍率(N60m/N66モード)
		st = min( max( MIN_BOOST, ecfg.GetValue( CV_MaxBoost62 ) ), MAX_BOOST );
		SetDlgItemText( hwnd, ID_BOOST62, std::to_string( st ).c_str() );
		
		// TAPEストップビット数
		st = min( max( MIN_STOPBIT, ecfg.GetValue( CV_StopBit ) ), MAX_STOPBIT );
		SetDlgItemText( hwnd, ID_STOPBIT, std::to_string( st ).c_str() );
		
		// FDDウェイト
		yn = ecfg.GetValue( CB_FDDWait );
		SendMessage( GetDlgItem( hwnd, ID_CB14 ), BM_SETCHECK, yn, 0 );
		
		// 終了時 確認する
		yn = ecfg.GetValue( CB_CkQuit );
		SendMessage( GetDlgItem( hwnd, ID_CB8 ), BM_SETCHECK, yn, 0 );
		
		// 終了時 INIファイルを保存する
		yn = ecfg.GetValue( CB_SaveQuit );
		SendMessage( GetDlgItem( hwnd, ID_CB12 ), BM_SETCHECK, yn, 0 );
		
		break;
	}
	
	return true;
}


///////////////////////////////////////////////////////////
// 設定を保存する
///////////////////////////////////////////////////////////
static bool OsdWriteINI( HWND hwnd, int page )
{
	int st;							// 状態取得用
	char str[PATH_MAX+1];			// 文字列取得用
	
	switch( page ){
	case PP_BASE:	// 基本
		// 機種
		if     ( IsDlgButtonChecked( hwnd, ID_RB02 ) ) st = 61;	// PC-6001A
		else if( IsDlgButtonChecked( hwnd, ID_RB03 ) ) st = 62;	// PC-6001mk2
		else if( IsDlgButtonChecked( hwnd, ID_RB04 ) ) st = 66;	// PC-6601
		else if( IsDlgButtonChecked( hwnd, ID_RB05 ) ) st = 64;	// PC-6001mk2SR
		else if( IsDlgButtonChecked( hwnd, ID_RB06 ) ) st = 68;	// PC-6601SR
		else                                           st = 60;	// 上記以外ならPC-6001
		ecfg.SetValue( CV_Model, st );
		
		// FDD
		if     ( IsDlgButtonChecked( hwnd, ID_RB32 ) ) st = 1;		// 2台
		else if( IsDlgButtonChecked( hwnd, ID_RB33 ) ) st = 2;		// 1台
		else                                           st = 0;		// 上記以外ならなし
		ecfg.SetValue( CV_FDD, st );
		
		// 拡張RAM使用
		ecfg.SetValue( CB_ExtRam, IsDlgButtonChecked( hwnd, ID_CB1 ) == BST_CHECKED ? true : false );
		
		// 戦士のカートリッジ使用
		ecfg.SetValue( CV_Soldier, IsDlgButtonChecked( hwnd, ID_CB13 ) ? 2 : 0 );	// とりあえずmkⅡ決め打ちで
		
		break;
		
	case PP_DISP:	// 画面
		// MODE4カラー
		if     ( IsDlgButtonChecked( hwnd, ID_RB12 ) ) st = 1;	// 赤/青
		else if( IsDlgButtonChecked( hwnd, ID_RB13 ) ) st = 2;	// 青/赤
		else if( IsDlgButtonChecked( hwnd, ID_RB14 ) ) st = 3;	// ピンク/緑
		else if( IsDlgButtonChecked( hwnd, ID_RB15 ) ) st = 4;	// 緑/ピンク
		else                                           st = 0;	// 上記以外ならモノクロ
		ecfg.SetValue( CV_Mode4Color, st );
		
		// ビデオキャプチャ色深度
		if     ( IsDlgButtonChecked( hwnd, ID_RB07 ) ) st = 16;	// 16bit
		else if( IsDlgButtonChecked( hwnd, ID_RB08 ) ) st = 24;	// 24bit
		else                                           st = 32;	// 上記以外なら32bit
		ecfg.SetValue( CV_AviBpp, st );
		
		// スキャンライン
		ecfg.SetValue( CB_ScanLine, IsDlgButtonChecked( hwnd, ID_CB2 ) == BST_CHECKED ? true : false );
		
		// スキャンライン輝度
		GetDlgItemText( hwnd, ID_SCANLINEBR, str, sizeof(str) );
		st = min( max( MIN_SCANLINEBR, std::strtol( str, nullptr, 0 ) ), MAX_SCANLINEBR );
		ecfg.SetValue( CV_ScanLineBr, st );
		
		// フィルタリング
		ecfg.SetValue( CB_Filtering, IsDlgButtonChecked( hwnd, ID_CB15 ) == BST_CHECKED ? true : false );
		
		// 4:3表示
		ecfg.SetValue( CB_DispNTSC, IsDlgButtonChecked( hwnd, ID_CB7 ) == BST_CHECKED ? true : false );
		
		// フルスクリーン
		ecfg.SetValue( CB_FullScreen, IsDlgButtonChecked( hwnd, ID_CB10 ) == BST_CHECKED ? true : false );
		
		// ステータスバー表示状態
		ecfg.SetValue( CB_DispStatus, IsDlgButtonChecked( hwnd, ID_CB11 ) == BST_CHECKED ? true : false );
		
		// フレームスキップ
		st = SendMessage( GetDlgItem( hwnd, ID_TBFPS ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_FrameSkip, st );
		
		break;
		
	case PP_SOUND:	// サウンド
		// サンプリングレート
		if     ( IsDlgButtonChecked( hwnd, ID_RB22 ) ) st = 22050;	// 22050
		else if( IsDlgButtonChecked( hwnd, ID_RB23 ) ) st = 11025;	// 11025
		else                                           st = 44100;	// 上記以外なら44100
		ecfg.SetValue( CV_SampleRate, st );
		
		// バッファサイズ
		st = SendMessage( GetDlgItem( hwnd, ID_TBBUF ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_SoundBuffer, st );
		
		// PSG LPFカットオフ周波数
		GetDlgItemText( hwnd, ID_EDIT1, str, sizeof(str) );
		st = min( max( MIN_LPF, std::strtol( str, nullptr, 0 ) ), MAX_LPF );
		ecfg.SetValue( CV_PsgLPF, st );
		
		// マスター音量
		st = SendMessage( GetDlgItem( hwnd, ID_TBMST ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_MasterVol, st * 10 );
		
		// PSG音量
		st = SendMessage( GetDlgItem( hwnd, ID_TBPSG ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_PsgVolume, st * 10 );
		
		// 音声合成音量
		st = SendMessage( GetDlgItem( hwnd, ID_TBVCE ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_VoiceVolume, st * 10 );
		
		// TAPEモニタ音量
		st = SendMessage( GetDlgItem( hwnd, ID_TBTAPE ), TBM_GETPOS, 0, 0 );
		ecfg.SetValue( CV_TapeVolume, st * 10 );
		
		break;
		
	case PP_INPUT:	// 入力関係
		// キーリピート間隔
		GetDlgItemText( hwnd, ID_KEYREP, str, sizeof(str) );
		st = min( max( MIN_REPEAT, std::strtol( str, nullptr, 0 ) ), MAX_REPEAT );
		ecfg.SetValue( CV_KeyRepeat, st );
		
		break;
		
	case PP_FOLDER:	// フォルダ
		// ROMパス
		ecfg.SetValue( CF_RomPath, GetTextBoxPath( hwnd, ID_PATH1 ) );
		
		// TAPEパス
		ecfg.SetValue( CF_TapePath, GetTextBoxPath( hwnd, ID_PATH2 ) );
		
		// DISKパス
		ecfg.SetValue( CF_DiskPath, GetTextBoxPath( hwnd, ID_PATH3 ) );
		
		// 拡張ROMパス
		ecfg.SetValue( CF_ExtRomPath, GetTextBoxPath( hwnd, ID_PATH4 ) );
		
		// IMGパス
		ecfg.SetValue( CF_ImgPath, GetTextBoxPath( hwnd, ID_PATH5 ) );
		
		// WAVEパス
		ecfg.SetValue( CF_WavePath, GetTextBoxPath( hwnd, ID_PATH6 ) );
		
		// どこでもSAVEパス
		ecfg.SetValue( CF_DokoPath, GetTextBoxPath( hwnd, ID_PATH7 ) );
		
		break;
		
	case PP_FILE:	// ファイル
		// 拡張ROMファイル
		ecfg.SetValue( CF_ExtRom, GetTextBoxPath( hwnd, ID_FEXROM ) );
		
		// TAPE(LOAD)ファイル名
		ecfg.SetValue( CF_tape, GetTextBoxPath( hwnd, ID_FTPLD ) );
		
		// TAPE(SAVE)ファイル名
		ecfg.SetValue( CF_save, GetTextBoxPath( hwnd, ID_FTPSV ) );
		
		// DISK1ファイル名
		ecfg.SetValue( CF_disk1, GetTextBoxPath( hwnd, ID_FDISK1 ) );
		
		// DISK2ファイル名
		ecfg.SetValue( CF_disk2, GetTextBoxPath( hwnd, ID_FDISK2 ) );
		
		// プリンタファイル名
		ecfg.SetValue( CF_printer, GetTextBoxPath( hwnd, ID_FPRINT ) );
		
		break;
		
	case PP_COL:	// 色1
		break;
		
	case PP_ETC:	// その他
		// オーバークロック率
		GetDlgItemText( hwnd, ID_OVERCLK, str, sizeof(str) );
		st = min( max( MIN_OVERCLOCK, std::strtol( str, nullptr, 0 ) ), MAX_OVERCLOCK );
		ecfg.SetValue( CV_OverClock, st );
		
		// CRCチェック
		ecfg.SetValue( CB_CheckCRC, IsDlgButtonChecked( hwnd, ID_CB4 ) == BST_CHECKED ? true : false );
		
		// Turbo TAPE
		ecfg.SetValue( CB_TurboTAPE, IsDlgButtonChecked( hwnd, ID_CB3 ) == BST_CHECKED ? true : false );
		
		// Boost Up
		ecfg.SetValue( CB_BoostUp, IsDlgButtonChecked( hwnd, ID_CB5 ) == BST_CHECKED ? true : false );
		
		// BoostUp 最大倍率(N60モード)
		GetDlgItemText( hwnd, ID_BOOST60, str, sizeof(str) );
		st = min( max( MIN_BOOST, std::strtol( str, nullptr, 0 ) ), MAX_BOOST );
		ecfg.SetValue( CV_MaxBoost60, st );
		
		// BoostUp 最大倍率(N60m/N66モード)
		GetDlgItemText( hwnd, ID_BOOST62, str, sizeof(str) );
		st = min( max( MIN_BOOST, std::strtol( str, nullptr, 0 ) ), MAX_BOOST );
		ecfg.SetValue( CV_MaxBoost62, st );
		
		// TAPEストップビット数
		GetDlgItemText( hwnd, ID_STOPBIT, str, sizeof(str) );
		st = min( max( MIN_STOPBIT, std::strtol( str, nullptr, 0 ) ), MAX_STOPBIT );
		ecfg.SetValue( CV_StopBit, st );
		
		// FDDウェイト
		ecfg.SetValue( CB_FDDWait, IsDlgButtonChecked( hwnd, ID_CB14 ) == BST_CHECKED ? true : false );
		
		// 終了時 確認する
		ecfg.SetValue( CB_CkQuit, IsDlgButtonChecked( hwnd, ID_CB8 ) == BST_CHECKED ? true : false );
		
		// 終了時 INIファイルを保存する
		ecfg.SetValue( CB_SaveQuit, IsDlgButtonChecked( hwnd, ID_CB12 ) == BST_CHECKED ? true : false );
		
		break;
	}
	
	return true;
}


///////////////////////////////////////////////////////////
// 環境設定ダイアログプロシージャ
///////////////////////////////////////////////////////////
// 基本
static INT_PTR CALLBACK OsdCnfgProc1( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_BASE ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_COMMAND:
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:	// OKボタン処理
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_BASE ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// 画面
static INT_PTR CALLBACK OsdCnfgProc2( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_DISP ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_COMMAND:
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_DISP ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// サウンド
static INT_PTR CALLBACK OsdCnfgProc3( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_SOUND ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_SOUND ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// 入力関係
static INT_PTR CALLBACK OsdCnfgProc4( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_INPUT ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_INPUT ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// ファイル
static INT_PTR CALLBACK OsdCnfgProc5( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_FILE ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_COMMAND:
		switch( wp ){
		case ID_B11:	// ファイル参照ボタン(拡張ROMイメージ)
			FileSelCom( hwnd, ID_FEXROM, FD_ExtRom, ecfg.GetValue( CF_ExtRomPath ) );
			break;
			
		case ID_B12:	// ファイル参照ボタン(TAPEイメージ(LOAD))
			FileSelCom( hwnd, ID_FTPLD, FD_TapeLoad, ecfg.GetValue( CF_TapePath ) );
			break;
			
		case ID_B13:	// ファイル参照ボタン(TAPEイメージ(SAVE))
			FileSelCom( hwnd, ID_FTPSV, FD_TapeSave, ecfg.GetValue( CF_TapePath ) );
			break;
			
		case ID_B14:	// ファイル参照ボタン(DISK1イメージ)
			FileSelCom( hwnd, ID_FDISK1, FD_Disk, ecfg.GetValue( CF_DiskPath ) );
			break;
			
		case ID_B15:	// ファイル参照ボタン(DISK2イメージ)
			FileSelCom( hwnd, ID_FDISK2, FD_Disk, ecfg.GetValue( CF_DiskPath ) );
			break;
			
		case ID_B16:	// ファイル参照ボタン(プリンタ出力ファイル)
			FileSelCom( hwnd, ID_FPRINT, FD_Printer, OSD_GetConfigPath() );
			break;
			
		case ID_B11E:	// EJECTボタン(拡張ROMイメージ)
			SetDlgItemText( hwnd, ID_FEXROM, "" );
			break;
			
		case ID_B12E:	// EJECTボタン(TAPEイメージ(LOAD))
			SetDlgItemText( hwnd, ID_FTPLD, "" );
			break;
			
		case ID_B13E:	// EJECTボタン(TAPEイメージ(SAVE))
			SetDlgItemText( hwnd, ID_FTPSV, "" );
			break;
			
		case ID_B14E:	// EJECTボタン(DISK1イメージ)
			SetDlgItemText( hwnd, ID_FDISK1, "" );
			break;
			
		case ID_B15E:	// EJECTボタン(DISK2イメージ)
			SetDlgItemText( hwnd, ID_FDISK2, "" );
			break;
			
		case ID_B16E:	// EJECTボタン(プリンタ出力ファイル)
			SetDlgItemText( hwnd, ID_FPRINT, "" );
			break;
		}
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_FILE ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// フォルダ
static INT_PTR CALLBACK OsdCnfgProc6( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_FOLDER ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_COMMAND:
		switch( wp ){
		case ID_B01:	// フォルダ参照ボタン(ROM)
			FolderSelCom( hwnd, ID_PATH1 );
			break;
			
		case ID_B02:	// フォルダ参照ボタン(TAPE)
			FolderSelCom( hwnd, ID_PATH2 );
			break;
			
		case ID_B03:	// フォルダ参照ボタン(DISK)
			FolderSelCom( hwnd, ID_PATH3 );
			break;
			
		case ID_B04:	// フォルダ参照ボタン(拡張ROM)
			FolderSelCom( hwnd, ID_PATH4 );
			break;
			
		case ID_B05:	// フォルダ参照ボタン(IMG)
			FolderSelCom( hwnd, ID_PATH5 );
			break;
			
		case ID_B06:	// フォルダ参照ボタン(WAVE)
			FolderSelCom( hwnd, ID_PATH6 );
			break;
			
		case ID_B07:	// フォルダ参照ボタン(どこでもSAVE)
			FolderSelCom( hwnd, ID_PATH7 );
			break;
		}
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_FOLDER ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// 色1
static INT_PTR CALLBACK OsdCnfgProcCol( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	static COLORREF Color = 0;
	static COLORREF CustColors[16] = { RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ),
									   RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ),
									   RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ),
									   RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ), RGB( 255, 255, 255 ) };
	static CHOOSECOLOR cc;	// 色の選択用
	COLOR24 col;
	
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_COL ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		
		// 色の選択 コモンダイアログ 初期化
		cc.lStructSize  = sizeof(CHOOSECOLOR);
		cc.hwndOwner    = hwnd;
		cc.rgbResult    = Color;
		cc.lpCustColors = CustColors;
		cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
		
		break;
		
	case WM_DRAWITEM:	// オーナードロー
		if( wp >= ID_COL16 && wp <= ID_COL80 ){
			col = ecfg.GetColor( wp-ID_COL16+16 );
			// オーナードローボタン 描画
			OsdOwnerDrawBtn( ((LPDRAWITEMSTRUCT)lp)->hDC,
							 ((LPDRAWITEMSTRUCT)lp)->rcItem,
							 RGB( col.r, col.g, col.b ) );
			return true;
		}
		return false;
		
	case WM_COMMAND:
		if( wp >= ID_COL16 && wp <= ID_COL80 ){
			col = ecfg.GetColor( wp-ID_COL16+16 );
			Color = RGB( col.r, col.g, col.b );
			// 色の選択 コモンダイアログ表示
			cc.rgbResult = Color;
			if( ChooseColor( &cc ) ){
				Color = cc.rgbResult;
				col.r = GetRValue( Color );
				col.g = GetGValue( Color );
				col.b = GetBValue( Color );
				ecfg.SetColor( wp-ID_COL16+16, col );
				InvalidateRect( hwnd, nullptr, true );
			}
		}
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_COL ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}

// その他
static INT_PTR CALLBACK OsdCnfgProcEtc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		// 設定を読込む
		if( !OsdReadINI( hwnd, PP_ETC ) ) Message_Win32( hwnd, GetText( TERR_IniReadFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		break;
		
	case WM_NOTIFY:
		switch( ((NMHDR*)lp)->code ){
		case PSN_APPLY:
			// 設定を保存する
			if( !OsdWriteINI( hwnd, PP_ETC ) ) Message_Win32( hwnd, GetText( TERR_IniWriteFailed ), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			return true;
		}
	}
	return false;
}


// オーナードローボタン 描画
static void OsdOwnerDrawBtn( HDC hdc, RECT rc, COLORREF col )
{
	HPEN hOldPen   = (HPEN)SelectObject( hdc, (HPEN)GetStockObject( BLACK_PEN ) );
	HBRUSH hBrs    = CreateSolidBrush( col );
	HBRUSH hOldBrs = (HBRUSH)SelectObject( hdc, hBrs );
	
	Rectangle( hdc, rc.left, rc.top, rc.right, rc.bottom );
	
	SelectObject( hdc, hOldPen );
	SelectObject( hdc, hOldBrs );
	DeleteObject( hBrs );
}



///////////////////////////////////////////////////////////
// バージョン情報表示
//
// 引数:	hwnd		ウィンドウハンドル
//			mdl			機種
// 返値:	なし
///////////////////////////////////////////////////////////
void OSD_VersionDialog( HWINDOW hwnd, int mdl )
{
	model = mdl;
	HWND hhwnd = (HWND)OSD_GetWindowHandle( hwnd );
	DialogBox( (HINSTANCE)GetWindowLongPtr( hhwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(ID_VER), hhwnd, (DLGPROC)VerInfoProc );
}


///////////////////////////////////////////////////////////
// バージョン情報ダイアログプロシージャ
///////////////////////////////////////////////////////////
static INT_PTR CALLBACK VerInfoProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	static HCURSOR hCursor = nullptr;
	
	switch( msg ){
	case WM_INITDIALOG:
		{
		// バージョン文字列設定
		SetDlgItemText( hwnd, ID_VERSTR, APPNAME " Ver." VERSION );
		
		// アイコン設定
		const char* ilp;
		switch( model ){
		case 61: ilp = MAKEINTRESOURCE(P61ICON); break;
		case 62: ilp = MAKEINTRESOURCE(P62ICON); break;
		case 66: ilp = MAKEINTRESOURCE(P66ICON); break;
		case 64: ilp = MAKEINTRESOURCE(P64ICON); break;
		case 68: ilp = MAKEINTRESOURCE(P68ICON); break;
		default: ilp = MAKEINTRESOURCE(P60ICON);
		}
		HICON hNewIcon = LoadIcon( (HINSTANCE)GetWindowLongPtr( hwnd, GWLP_HINSTANCE), ilp );
		HICON hOldIcon = (HICON)SendMessage( GetDlgItem( hwnd, ID_VERICON ), STM_SETICON, (WPARAM)hNewIcon, 0 );
		if( hOldIcon ) DeleteObject( hOldIcon );
		
		// 指型カーソル取得
		HINSTANCE hInstHelp;
		char WinDir[PATH_MAX+1];
		
		GetWindowsDirectory( WinDir, PATH_MAX-13 );
		lstrcat( WinDir, "\\winhlp32.exe");
		hInstHelp = LoadLibrary( WinDir );
		if( hInstHelp ){
			hCursor = (HCURSOR)CopyImage( LoadImage( hInstHelp, MAKEINTRESOURCE(106), IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR ), IMAGE_CURSOR, 0, 0, LR_COPYDELETEORG );
			FreeLibrary( hInstHelp );
		}else
			hCursor = (HCURSOR)LoadImage( nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR );
		break;
		}
	case WM_SETCURSOR:	// カーソル変更
		if( (HWND)wp == GetDlgItem( hwnd, ID_URL ) ){
			SetCursor( hCursor );
			GetWindowLongPtr( hwnd, DWLP_MSGRESULT );
			return true;
		}
		return false;
		break;
		
	case WM_CTLCOLORSTATIC:	// URLの色を変える
		{
		HDC hDC = (HDC)wp;
		if( (HWND)lp == GetDlgItem( hwnd, ID_URL ) ){
			SetBkMode( hDC, TRANSPARENT );
			SetTextColor( hDC, RGB(0,0,255) );	// 色を変える
			return (INT_PTR)GetStockObject( NULL_BRUSH );
		}
		return ( COLOR_BTNFACE + 1 ) ? true : false;
		}
		break;
		
	case WM_COMMAND:
    	switch( LOWORD(wp) ){
		case IDOK:
		case IDCANCEL:
			if( hCursor ) DestroyCursor( hCursor );	// カーソル開放
			EndDialog( hwnd, true );
			break;
		case ID_URL:
			{
			char Url[256];
			GetWindowText( GetDlgItem( hwnd, ID_URL ), Url, 256 );
			ShellExecute( hwnd, "open", Url, nullptr, nullptr, SW_SHOWNORMAL );
			InvalidateRect( GetDlgItem( hwnd, ID_URL ), nullptr, false );
			return true;
			}
		}
	}
	return false;
}










//**************************************************************
// 実験中
//**************************************************************


////////////////////////////////////////////////////////////////
// ステータスバー作成
//
// 引数:	hwnd		ステータスバーのウィンドウハンドルへのポインタ(Win32)
//			hwndp		親ウィンドウハンドル(SDL)
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool OSD_CreateStatusBar( HWINDOW* hwnd, HWINDOW hwndp )
{
	static int iRight[] = { 100 , 200 , -1 };
	
	InitCommonControls();
	*hwnd = CreateStatusWindow( WS_CHILD | WS_VISIBLE | CCS_BOTTOM, "TITLE", (HWND)OSD_GetWindowHandle( hwndp ), 1 );
	SendMessage( (HWND)*hwnd , SB_SETPARTS , 3 , (LPARAM)iRight);
	
	
	return true;
}

