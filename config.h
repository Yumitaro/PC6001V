#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <string>
#include <vector>

#include "ini.h"
#include "keydef.h"
#include "vsurface.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class CFG6 : public cIni {
protected:
	// INIファイルに保存しないメンバ
	P6VPATH IniPath;		// INIファイルパス
	P6VPATH DokoFile;		// どこでもSAVEファイル名
	std::string Caption;	// ウィンドウキャプション
	
	// INIファイルに保存するメンバ
	P6VPATH RomPath;		// ROMパス
	P6VPATH ExtRomPath;		// 拡張ROMパス
	P6VPATH ExtRomFile;		// 拡張ROMファイル名
	P6VPATH WavePath;		// WAVEパス
	P6VPATH TapePath;		// TAPEパス
	P6VPATH TapeFile;		// TAPEファイル名
	P6VPATH SaveFile;		// TAPE(SAVE)ファイル名
	P6VPATH DiskPath;		// DISKパス
	P6VPATH DiskFile1;		// DISK1ファイル名
	P6VPATH DiskFile2;		// DISK2ファイル名
	P6VPATH ImgPath;		// スクリーンショット格納パス
	P6VPATH PrinterFile;	// プリンタファイル名
	P6VPATH DokoSavePath;	// どこでもSAVEパス
	P6VPATH FontPath;		// フォントパス
	
	void InitIni( bool );									// INIオブジェクト初期値設定
	
	const std::string& GetPCKeyName( PCKEYsym );			// 仮想キーコードから名称取得
	const std::string& GetP6KeyName( P6KEYsym );			// P6キーコードから名称取得
	PCKEYsym GetPCKeyCode( const std::string& );			// キー名称から仮想キーコードを取得
	P6KEYsym GetP6KeyCode( const std::string& );			// キー名称からP6キーコードを取得

public:
	CFG6();
	~CFG6();
	
	bool Init();											// 初期化(INIファイル読込み)
	bool Write();											// INIファイル書込み
	
	// メンバアクセス関数
	// [CONFIG] ----------------------------------------------------
	int GetModel();											// 機種取得
	void SetModel( int );									//     設定
	
	int GetFddNum();										// FDD接続台数取得
	void SetFddNum( int );									//            設定
	
	bool GetUseExtRam();									// 拡張RAMを使う取得
	void SetUseExtRam( bool );								//              設定
	
	bool GetTurboTAPE();									// Turbo TAPE 有効フラグ取得
	void SetTurboTAPE( bool );								//                      設定
	
	bool GetBoostUp();										// BoostUp 有効フラグ取得
	void SetBoostUp( bool );								//                   設定
	
	int GetMaxBoost1();										// BoostUp 最大倍率(N60モード)取得
	void SetMaxBoost1( int );								//                            設定
	
	int GetMaxBoost2();										// BoostUp 最大倍率(N60m/N66モード)取得
	void SetMaxBoost2( int );								//                                 設定
	
	int GetStopBit();										// TAPEストップビット数取得
	void SetStopBit( int );									//                     設定
	
	int GetOverClock();										// オーバークロック率取得
	void SetOverClock( int );								//                   設定
	
	bool GetCheckCRC();										// CRCチェック取得
	void SetCheckCRC( bool );								//            設定
	
	bool GetFddWaitEnable();								// FDDウェイト有効フラグ取得
	void SetFddWaitEnable( bool );							//                      設定
	
	// [DISPLAY] ---------------------------------------------------
	int GetMode4Color();									// モード4カラーモード取得
	void SetMode4Color( int );								//                    設定
	
	bool GetScanLine();										// スキャンライン取得
	void SetScanLine( bool );								//               設定
	
	int GetScanLineBr();									// スキャンライン輝度取得
	void SetScanLineBr( int );								//                   設定
	
	bool GetFiltering();									// フィルタリング取得
	void SetFiltering( bool );								//               設定
	
	bool GetDispNTSC();										// 4:3表示取得
	void SetDispNTSC( bool );								//        設定
	
	bool GetFullScreen();									// フルスクリーン取得
	void SetFullScreen( bool );								//               設定
	
	int GetWindowZoom();									// ウィンドウ表示倍率取得
	void SetWindowZoom( int );								//                   設定
	
	bool GetDispStat();										// ステータスバー表示状態取得
	void SetDispStat( bool );								//                       設定
	
	int GetFrameSkip();										// フレームスキップ取得
	void SetFrameSkip( int );								//                 設定
	
	// [SOUND] -----------------------------------------------------
	int GetSampleRate();									// サンプリングレート取得
	void SetSampleRate( int );								//                   設定
	
	int GetSoundBuffer();									// サウンドバッファ長倍率取得
	void SetSoundBuffer( int );								//                       設定
	
	int GetMasterVol();										// マスター音量取得
	void SetMasterVol( int );								//             設定
	
	int GetPsgVol();										// PSG音量取得
	void SetPsgVol( int );									//        設定
	
	int GetPsgLPF();										// PSG LPFカットオフ周波数取得
	void SetPsgLPF( int );									//                        設定
	
	int GetVoiceVol();										// 音声合成音量取得
	void SetVoiceVol( int );								//             設定
	
	int GetCmtVol();										// TAPEモニタ音量取得
	void SetCmtVol( int );									//               設定
	
	int GetCmtLPF();										// TAPE LPFカットオフ周波数取得
	void SetCmtLPF( int );									//                         設定
	
	// [MOVIE] -----------------------------------------------------
	int GetAviBpp();										// 色深度取得
	void SetAviBpp( int );									//       設定
	
	// [FILES] -----------------------------------------------------
	const P6VPATH& GetExtRomFile();							// 拡張ROMファイル名取得
	void SetExtRomFile( const P6VPATH& );					//                  設定
	
	const P6VPATH& GetTapeFile();							// TAPEファイル名取得
	void SetTapeFile( const P6VPATH& );						//               設定
	
	const P6VPATH& GetSaveFile();							// TAPE(SAVE)ファイル名取得
	void SetSaveFile( const P6VPATH& );						//                     設定
	
	const P6VPATH& GetDiskFile( int );						// DISKファイル名取得
	void SetDiskFile( int, const P6VPATH& );				//               設定
	
	const P6VPATH& GetPrinterFile();						// プリンタファイル名取得
	void SetPrinterFile( const P6VPATH& );					//                   設定
	
	// [PATH] ------------------------------------------------------
	const P6VPATH& GetRomPath();							// ROMパス取得
	void SetRomPath( const P6VPATH& );						//        設定
	
	const P6VPATH& GetTapePath();							// TAPEパス取得
	void SetTapePath( const P6VPATH& );						//         設定
	
	const P6VPATH& GetDiskPath();							// DISKパス取得
	void SetDiskPath( const P6VPATH& );						//         設定
	
	const P6VPATH& GetExtRomPath();							// 拡張ROMパス取得
	void SetExtRomPath( const P6VPATH& );					//            設定
	
	const P6VPATH& GetImgPath();							// スクリーンショット格納パス取得
	void SetImgPath( const P6VPATH& );						//                           設定
	
	const P6VPATH& GetWavePath();							// WAVEパス取得
	void SetWavePath( const P6VPATH& );						//         設定
	
	const P6VPATH& GetFontPath();							// フォントパス取得
	void SetFontPath( const P6VPATH& );						//             設定
	
	const P6VPATH& GetDokoSavePath();						// どこでもSAVEパス取得
	void SetDokoSavePath( const P6VPATH& );					//                 設定
	
	// [CHECK] -----------------------------------------------------
	bool GetCkQuit();										// 終了時確認取得
	void SetCkQuit( bool );									//           設定
	
	bool GetSaveQuit();										// 終了時INI保存取得
	void SetSaveQuit( bool );								//              設定
	
	// [OPTION] ----------------------------------------------------
	int GetUseSoldier();									// 戦士のカートリッジ使うフラグ取得
	void SetUseSoldier( int );								//                             設定
	
	// [KEY] -------------------------------------------------------
	int GetKeyRepeat();										// キーリピート取得
	void SetKeyRepeat( int );								//             設定
	
	P6KEYsym GetVKey( PCKEYsym );							// キー定義取得
	void SetVKey( PCKEYsym, P6KEYsym );						//         設定
	
	int GetVKeyDef( std::vector<VKeyConv>& );				// キー定義配列取得
	
	// [COLOR] -----------------------------------------------------
	COLOR24 GetColor( int );								// カラーデータ取得
	void SetColor( int, const COLOR24& );					//             設定
	
	
	
	// その他 ------------------------------------------------------
	const std::string& GetCaption();						// ウィンドウキャプション取得
	const P6VPATH GetDokoFile();							// どこでもSAVEファイル名取得
	void SetDokoFile( const P6VPATH& );						//                       設定
	
	// ---------------------------------------------------------
	bool DokoSave( cIni* );		// どこでもSAVE
	bool DokoLoad( cIni* );		// どこでもLOAD
	// ---------------------------------------------------------
};

#endif	// CONFIG_H_INCLUDED
