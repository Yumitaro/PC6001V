#ifndef OSD_H_INCLUDED
#define OSD_H_INCLUDED

// OS依存の汎用ルーチン(主にUI用)

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "event.h"
#include "typedef.h"
#include "keydef.h"
#include "vsurface.h"


// ファイル選択ダイアログ用
enum FileMode{ FM_Load, FM_Save };
enum FileDlg{ FD_TapeLoad, FD_TapeSave, FD_Disk, FD_ExtRom, FD_Printer, FD_FontZ, FD_FontH,
			  FD_DokoLoad, FD_DokoSave, FD_RepLoad, FD_RepSave, FD_AVISave, FD_LoadAll, EndofFileDlg };




/*
////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////
// パスの末尾にデリミタを追加
void AddDelimiter( std::filesystem::path& );
// パスの末尾のデリミタを削除
void DelDelimiter( std::filesystem::path& );
// 相対パス化
void RelativePath( std::filesystem::path& );
// 絶対パス化
void AbsolutePath( std::filesystem::path& );
// パス結合
void AddPath( std::filesystem::path&, const std::filesystem::path&, const std::filesystem::path& );
// パスからフォルダ名を取得
const std::string GetFolderNamePart( const std::filesystem::path& );
// パスからファイル名を取得
const std::string GetFileNamePart( const std::filesystem::path& );
// パスから拡張子名を取得
const std::string GetFileNameExt( const std::filesystem::path& );
// 拡張子名を変更
bool ChangeFileNameExt( std::filesystem::path&, const std::string& );


////////////////////////////////////////////////////////////////
// ファイル操作関数
////////////////////////////////////////////////////////////////
// ファイルを開く
//FILE* Fopen( const std::filesystem::path&, const std::string& );
// ファイルストリームを開く
bool FSopen( std::fstream&, const std::filesystem::path&, const std::ios_base::openmode );
// フォルダを作成
bool CreateFolder( const std::filesystem::path& );
// ファイルの存在チェック
bool FileExist( const std::filesystem::path& );
// ファイルサイズ取得
DWORD GetFileSize( const std::filesystem::path& );
// ファイルの読取り専用チェック
bool FileReadOnly( const std::filesystem::path& );
*/


////////////////////////////////////////////////////////////////
// プロセス管理関数
////////////////////////////////////////////////////////////////
// 初期化
bool OSD_Init();
// 初期化Sub(ライブラリ依存処理等)
bool OSD_Init_Sub();
// 終了処理
void OSD_Quit();
// 終了処理Sub(ライブラリ依存処理等)
void OSD_Quit_Sub();
// 多重起動チェック
bool OSD_IsWorking();


////////////////////////////////////////////////////////////////
// パス名処理関数
////////////////////////////////////////////////////////////////
// モジュールパス取得
const std::filesystem::path& OSD_GetModulePath();


////////////////////////////////////////////////////////////////
// ファイル操作関数
////////////////////////////////////////////////////////////////
// ファイルを開く
FILE* OSD_Fopen( const std::filesystem::path&, const std::string& );
// フォルダの参照
bool OSD_FolderDiaog( HWINDOW, std::filesystem::path& );
// 各種ファイル選択
bool OSD_FileSelect( HWINDOW, FileDlg, std::filesystem::path&, std::filesystem::path& );


////////////////////////////////////////////////////////////////
// メッセージ表示関数
////////////////////////////////////////////////////////////////
// メッセージ表示
int OSD_Message( HWINDOW, const std::string&, const std::string&, int );


////////////////////////////////////////////////////////////////
// キー入力処理関数
////////////////////////////////////////////////////////////////
// キーリピート設定
void OSD_SetKeyRepeat( int );


////////////////////////////////////////////////////////////////
// ジョイスティック処理関数
////////////////////////////////////////////////////////////////
// 利用可能なジョイスティック数取得
int OSD_GetJoyNum();
// ジョイスティック名取得
const std::string OSD_GetJoyName( int );
// ジョイスティックオープンされてる？
bool OSD_OpenedJoy( HJOYINFO );
// ジョイスティックオープン
HJOYINFO OSD_OpenJoy( int );
// ジョイスティッククローズ
void OSD_CloseJoy( HJOYINFO );
// ジョイスティックの軸の数取得
int OSD_GetJoyNumAxes( HJOYINFO );
// ジョイスティックのボタンの数取得
int OSD_GetJoyNumButtons( HJOYINFO );
// ジョイスティックの軸の状態取得
int OSD_GetJoyAxis( HJOYINFO, int );
// ジョイスティックのボタンの状態取得
bool OSD_GetJoyButton( HJOYINFO, int );


////////////////////////////////////////////////////////////////
// サウンド関連関数
////////////////////////////////////////////////////////////////
// オーディオデバイスオープン
bool OSD_OpenAudio( void*, CBF_SND, int, int );
// オーディオデバイスクローズ
void OSD_CloseAudio();
// 再生開始
void OSD_StartAudio();
// 再生停止
void OSD_StopAudio();
// 再生状態取得
bool OSD_AudioPlaying();
// Waveファイル読込み
bool OSD_LoadWAV( const std::filesystem::path&, BYTE**, DWORD*, int* );
// Waveファイル開放
void OSD_FreeWAV( BYTE* );
// オーディオをロックする
void OSD_LockAudio();
// オーディオをアンロックする
void OSD_UnlockAudio();


////////////////////////////////////////////////////////////////
// タイマ関連関数
////////////////////////////////////////////////////////////////
// 指定時間待機
void OSD_Delay( DWORD );
// プロセス開始からの経過時間取得
DWORD OSD_GetTicks();
// タイマ追加
TIMERID OSD_AddTimer( DWORD, CBF_TMR, void * );
// タイマ削除
bool OSD_DelTimer( TIMERID );


////////////////////////////////////////////////////////////////
// ウィンドウ関連関数
////////////////////////////////////////////////////////////////
// ウィンドウ作成
bool OSD_CreateWindow( HWINDOW*, const int, const int, const int, const int, const bool, const bool, const int );
// ウィンドウ破棄
void OSD_DestroyWindow( HWINDOW );
// ウィンドウの幅を取得
int OSD_GetWindowWidth( HWINDOW );
// ウィンドウの高さを取得
int OSD_GetWindowHeight( HWINDOW );
// フルスクリーン?
bool OSD_IsFullScreen( HWINDOW );
// フィルタリング有効?
bool OSD_IsFiltering( HWINDOW );
// ウィンドウクリア
void OSD_ClearWindow( HWINDOW );
// ウィンドウ反映
void OSD_RenderWindow( HWINDOW );
// ウィンドウに転送(等倍)
void OSD_BlitToWindow( HWINDOW, VSurface*, const int, const int );
// ウィンドウに転送(拡大等)
void OSD_BlitToWindowEx( HWINDOW, VSurface*,  const VRect*, const bool );
// ウィンドウのイメージデータ取得
bool OSD_GetWindowImage( HWINDOW, void**, VRect*, int );
// アイコン設定
void OSD_SetIcon( HWINDOW, int );
// キャプション設定
void OSD_SetWindowCaption( HWINDOW, const std::string& );
// マウスカーソル表示/非表示
void OSD_ShowCursor( bool );
// OS依存のウィンドウハンドルを取得
void* OSD_GetWindowHandle( HWINDOW );
// 環境設定ダイアログ表示
int OSD_ConfigDialog( HWINDOW hwnd );
// バージョン情報表示
void OSD_VersionDialog( HWINDOW, int );


////////////////////////////////////////////////////////////////
// イベント処理関連関数
////////////////////////////////////////////////////////////////
// イベントキュークリア
void OSD_FlushEvents();
// イベント取得(イベントが発生するまで待つ)
bool OSD_GetEvent( Event* );
// イベントをキューにプッシュする
bool OSD_PushEvent( EventType, ... );


////////////////////////////////////////////////////////////////
// その他の雑関数
////////////////////////////////////////////////////////////////
// フォントファイル作成
bool OSD_CreateFont( const std::filesystem::path&, const std::filesystem::path&, int );
// ShiftJIS -> UTF-8
bool OSD_SJIStoUTF8( std::string& );
// UTF-8 -> ShiftJIS
bool OSD_UTF8toSJIS( std::string& );


// メッセージボックスのタイプ
#define	OSDM_OK				0x000
#define	OSDM_OKCANCEL		0x001
#define	OSDM_YESNO			0x002
#define	OSDM_YESNOCANCEL	0x003

// メッセージボックスのアイコンタイプ
#define	OSDM_ICONERROR		0x010
#define	OSDM_ICONQUESTION	0x020
#define	OSDM_ICONWARNING	0x030
#define	OSDM_ICONINFO		0x040

// メッセージボックスの戻り値
#define	OSDR_OK				0x00
#define	OSDR_CANCEL			0x01
#define	OSDR_YES			0x02
#define	OSDR_NO				0x03


#endif	// OSD_H_INCLUDED
