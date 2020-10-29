#ifndef P6EL_H_INCLUDED
#define P6EL_H_INCLUDED

#include <memory>
#include <string>

#include "typedef.h"
#include "movie.h"
#include "replay.h"
#include "thread.h"
#include "vdg.h"
#include "vsurface.h"


class VM6;
class CFG6;
class DSP6;
class SND6;
class SCH6;
class JOY6;
class cWndStat;

#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
class cWndReg;
class cWndMem;
class cWndMon;
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
// エミュレータレイヤークラス
class EL6 : public cThread, public AVI6, public REPLAY {
	
	friend class DSP6;
	
public:
	enum ReturnCode{
		Quit = 0,
		Restart,
		Dokoload,
		Replay,
		Error
	};
	
protected:
	// オブジェクトポインタ
	std::shared_ptr<CFG6> cfg;		// 環境設定オブジェクト
	std::shared_ptr<VM6>  vm;		// VM
	
	std::unique_ptr<SCH6> sche;		// スケジューラ
	std::unique_ptr<SND6> snd;		// サウンド
	std::unique_ptr<JOY6> joy;		// ジョイスティック
	std::unique_ptr<DSP6> graph;	// 画面描画
	
	std::unique_ptr<cWndStat> staw;	// ステータスバー
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	std::unique_ptr<cWndReg> regw;	// レジスタウィンドウ
	std::unique_ptr<cWndMem> memw;	// メモリウィンドウ
	std::unique_ptr<cWndMon> monw;	// モニタウィンドウ
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	TIMERID UpDateFPSID;			// FPS表示タイマID
	int FSkipCount;					// フレームスキップカウンタ
	bool MMotion;					// マウス動いたフラグ
	
	static int Speed;				// 停止時の速度退避用
	
	bool ScreenUpdate();								// 画面更新
	int SoundUpdate( int, cRing* = nullptr );			// サウンド更新
	static void StreamUpdate( void*, BYTE*, int );		// ストリーム更新 コールバック関数
	static DWORD UpDateFPS( DWORD, void* );				// FPS表示タイマ コールバック関数
	bool StartFPSTimer();								// FPS表示タイマ開始
	void StopFPSTimer();								// FPS表示タイマ停止
	
	void OnThread( void* ) override;					// スレッド関数
	
	int Emu();											// 1命令実行
	int EmuVSYNC();										// 1画面分実行
	void Wait();										// Wait
	
	bool CheckFuncKey( int, bool );						// 各種機能キーチェック
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	void CheckMonKey( int, int, bool );					// モニタモードキーチェック
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	// 自動キー入力情報構造体
	struct AKEY{
		std::string Buffer;		// キーバッファ
		int Wait;				// 待ち回数カウンタ
		bool Relay;				// リレースイッチOFF待ちフラグ
		bool RelayOn;			// リレースイッチON待ちフラグ
		
		AKEY() : Buffer(""), Wait(0), Relay(false), RelayOn(false) {}
	};
	AKEY ak;					// 自動キー入力情報
	
	char GetAutoKey();									// 自動キー入力1文字取得
	
	
	// UI関連
	P6VPATH TapePathUI;		// TAPEパス
	P6VPATH DiskPathUI;		// DISKパス
	P6VPATH ExRomPathUI;	// 拡張ROMパス
	P6VPATH DokoPathUI;		// どこでもSAVEパス
	
	void UI_TapeInsert( const P6VPATH& = "" );						// TAPE 挿入
	void UI_DiskInsert( int, const P6VPATH& = "" );					// DISK 挿入
	void UI_RomInsert( const P6VPATH& = "" );						// 拡張ROM 挿入
	void UI_RomEject();												// 拡張ROM 排出
	void UI_DokoSave( const P6VPATH& = "" );						// どこでもSAVE
	void UI_DokoLoad( const P6VPATH& = "" );						// どこでもLOAD
	void UI_ReplaySave( const P6VPATH& = "" );						// リプレイ保存
	void UI_ReplayResumeSave( const P6VPATH& = "" );				// リプレイ保存再開
	void UI_ReplayDokoLoad();										// リプレイ中どこでもLOAD
	void UI_ReplayDokoSave();										// リプレイ中どこでもSAVE
	void UI_ReplayLoad( const P6VPATH& = "" );						// リプレイ再生
	void UI_AVISave();												// ビデオキャプチャ
	void UI_AutoType( const P6VPATH& = "" );						// 打込み代行
	void UI_Reset();												// リセット
	void UI_Restart();												// 再起動
	void UI_Quit();													// 終了
	void UI_NoWait();												// Wait変更
	void UI_TurboTape();											// Turbo TAPE変更
	void UI_BoostUp();												// Boost Up変更
	void UI_FullScreen();											// フルスクリーン変更
	void UI_WindowZoom( int );										// ウィンドウ表示倍率変更
	void UI_StatusBar();											// ステータスバー表示状態変更
	void UI_Disp43();												// 4:3表示変更
	void UI_ScanLine();												// スキャンラインモード変更
	void UI_Filtering();											// フィルタリング変更
	void UI_Mode4Color( int );										// MODE4カラー変更
	void UI_FrameSkip( int );										// フレームスキップ変更
	void UI_SampleRate( int );										// サンプリングレート変更
	void UI_Config();												// 環境設定
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	void Exec( int );							// 指定ステート数実行
	void ToggleMonitor();						// モニタモード切替え
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	
	void ShowPopupMenu( int, int );							// ポップアップメニュー表示
	void ExecMenu( int );									// メニュー選択項目実行
	
	bool TapeMount( const P6VPATH& );						// TAPE マウント
	void TapeUnmount();										// TAPE アンマウント
	bool DiskMount( int, const P6VPATH& );					// DISK マウント
	void DiskUnmount( int );								// DISK アンマウント
	
	bool ReplayRecStart( const P6VPATH& );					// リプレイ保存開始
	bool ReplayRecResume( const P6VPATH& );					// リプレイ保存再開
	bool ReplayRecDokoLoad();								// リプレイ中どこでもLOAD
	bool ReplayRecDokoSave();								// リプレイ中どこでもSAVE
	void ReplayRecStop();									// リプレイ保存停止
	void ReplayPlayStart( const P6VPATH& );					// リプレイ再生開始
	void ReplayPlayStop();									// リプレイ再生停止
	
	bool IsAutoKey();										// 自動キー入力実行中?
	bool SetAutoKeyFile( const P6VPATH& );					// 自動キー入力文字列設定(ファイルから)
	bool SetAutoKey( const std::string& );					// 自動キー入力文字列設定
	void SetAutoStart();									// オートスタート文字列設定
	
	void SetPalette();										// パレット設定
	
	std::shared_ptr<VSurface> GetBackBuffer();				// バックバッファ取得
	const VDGInfo& GetVideoInfo() const;					// 画面情報取得
	
public:
	EL6();
	~EL6();
	
	bool Init( const std::shared_ptr<CFG6>& );	// 初期化
	
	bool Start();								// 動作開始
	void Stop();								// 動作停止
	
	ReturnCode EventLoop();						// イベントループ
	
	HWINDOW GetWindowHandle();					// ウィンドウハンドル取得
	
	// ---------------------------------------------------------
	bool DokoDemoSave( const P6VPATH& );		// どこでもSAVE
	bool DokoDemoLoad( const P6VPATH& );		// どこでもLOAD
	int GetDokoModel( const P6VPATH& );			// どこでもLOADファイルから機種名読込
	// ---------------------------------------------------------
};


#endif		// P6EL_H_INCLUDED
