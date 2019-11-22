#ifndef P6EL_H_INCLUDED
#define P6EL_H_INCLUDED

#include <memory>
#include <string>

#include "typedef.h"
#include "movie.h"
#include "replay.h"
#include "thread.h"
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
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	friend class cWndMon;
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
public:
	enum ReturnCode {
		Quit = 0,
		Restart,
		Dokoload,
		Replay,
		Error,
		
		EndofReturnCode
	};
	
protected:
	// オブジェクトポインタ
	CFG6* cfg;						// 環境設定オブジェクト
	
	std::unique_ptr<VM6>  vm;		// VM
	std::unique_ptr<SCH6> sche;		// スケジューラ
	std::unique_ptr<DSP6> graph;	// 画面描画
	std::unique_ptr<SND6> snd;		// サウンド
	std::unique_ptr<JOY6> joy;		// ジョイスティック
	
	std::unique_ptr<cWndStat> staw;	// ステータスバー
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	std::unique_ptr<cWndReg> regw;	// レジスタウィンドウ
	std::unique_ptr<cWndMem> memw;	// メモリウィンドウ
	std::unique_ptr<cWndMon> monw;	// モニタウィンドウ
	bool MonDisp;					// モニタウィンドウ表示状態 true:表示 false:非表示
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	TIMERID UpDateFPSID;			// FPS表示タイマID
	int FSkipCount;					// フレームスキップカウンタ
	
	static int Speed;				// 停止時の速度退避用
	
	void DeleteAllObject();								// 全オブジェクト削除
	
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
	
	bool CheckFuncKey( int, bool, bool );				// 各種機能キーチェック
	
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
	std::filesystem::path TapePathUI;	// TAPEパス
	std::filesystem::path DiskPathUI;	// DISKパス
	std::filesystem::path ExRomPathUI;	// 拡張ROMパス
	std::filesystem::path DokoPathUI;	// どこでもSAVEパス
	
	void UI_TapeInsert( const std::filesystem::path& = "" );		// TAPE 挿入
	void UI_DiskInsert( int, const std::filesystem::path& = "" );	// DISK 挿入
	void UI_RomInsert( const std::filesystem::path& = "" );			// 拡張ROM 挿入
	void UI_RomEject();												// 拡張ROM 排出
	void UI_DokoSave( const std::filesystem::path& = "" );			// どこでもSAVE
	void UI_DokoLoad( const std::filesystem::path& = "" );			// どこでもLOAD
	void UI_ReplaySave( const std::filesystem::path& = "" );		// リプレイ保存
	void UI_ReplayResumeSave( const std::filesystem::path& = "" );	// リプレイ保存再開
	void UI_ReplayDokoLoad();										// リプレイ中どこでもLOAD
	void UI_ReplayDokoSave();										// リプレイ中どこでもSAVE
	void UI_ReplayLoad( const std::filesystem::path& = "" );		// リプレイ再生
	void UI_AVISave();												// ビデオキャプチャ
	void UI_AutoType( const std::filesystem::path& = "" );			// 打込み代行
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
	
	bool TapeMount( const std::filesystem::path& );			// TAPE マウント
	void TapeUnmount();										// TAPE アンマウント
	bool DiskMount( int, const std::filesystem::path& );	// DISK マウント
	void DiskUnmount( int );								// DISK アンマウント
	
	bool ReplayRecStart( const std::filesystem::path& );	// リプレイ保存開始
	bool ReplayRecResume( const std::filesystem::path& );	// リプレイ保存再開
	bool ReplayRecDokoLoad();								// リプレイ中どこでもLOAD
	bool ReplayRecDokoSave();								// リプレイ中どこでもSAVE
	void ReplayRecStop();									// リプレイ保存停止
	void ReplayPlayStart( const std::filesystem::path& );	// リプレイ再生開始
	void ReplayPlayStop();									// リプレイ再生停止
	
	bool IsAutoKey();										// 自動キー入力実行中?
	bool SetAutoKeyFile( const std::filesystem::path& );	// 自動キー入力文字列設定(ファイルから)
	bool SetAutoKey( const std::string& );					// 自動キー入力文字列設定
	void SetAutoStart();									// オートスタート文字列設定
	
	void SetPalette();										// パレット設定
	
public:
	EL6();										// コンストラクタ
	~EL6();										// デストラクタ
	
	bool Init( const CFG6* );					// 初期化
	
	bool Start();								// 動作開始
	void Stop();								// 動作停止
	
	ReturnCode EventLoop();						// イベントループ
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	bool IsMonitor() const;						// モニタモード?
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	HWINDOW GetWindowHandle();					// ウィンドウハンドル取得
	
	// ------------------------------------------
	bool DokoDemoSave( const std::filesystem::path& );	// どこでもSAVE
	bool DokoDemoLoad( const std::filesystem::path& );	// どこでもLOAD
	int GetDokoModel( const std::filesystem::path& );	// どこでもLOADファイルから機種名読込
	// ------------------------------------------
};


#endif		// P6EL_H_INCLUDED
