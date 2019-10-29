#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <filesystem>
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
	std::filesystem::path IniPath;		// INIファイルパス
	std::filesystem::path DokoFile;		// どこでもSAVEファイル名
	std::string Caption;				// ウィンドウキャプション
	
	// INIファイルに保存するメンバ
	std::filesystem::path RomPath;		// ROMパス
	std::filesystem::path ExtRomPath;	// 拡張ROMパス
	std::filesystem::path ExtRomFile;	// 拡張ROMファイル名
	std::filesystem::path WavePath;		// WAVEパス
	std::filesystem::path TapePath;		// TAPEパス
	std::filesystem::path TapeFile;		// TAPEファイル名
	std::filesystem::path SaveFile;		// TAPE(SAVE)ファイル名
	std::filesystem::path DiskPath;		// DISKパス
	std::filesystem::path DiskFile1;	// DISK1ファイル名
	std::filesystem::path DiskFile2;	// DISK2ファイル名
	std::filesystem::path ImgPath;		// スクリーンショット格納パス
	std::filesystem::path PrinterFile;	// プリンタファイル名
	std::filesystem::path DokoSavePath;	// どこでもSAVEパス
	std::filesystem::path FontPath;		// フォントパス
	
	void InitIni( bool );									// INIオブジェクト初期値設定
	
	const std::string& GetPCKeyName( PCKEYsym );			// 仮想キーコードから名称取得
	const std::string& GetP6KeyName( P6KEYsym );			// P6キーコードから名称取得
	PCKEYsym GetPCKeyCode( const std::string& );			// キー名称から仮想キーコードを取得
	P6KEYsym GetP6KeyCode( const std::string& );			// キー名称からP6キーコードを取得

public:
	CFG6();													// コンストラクタ
	virtual ~CFG6();										// デストラクタ
	
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
	const std::filesystem::path& GetExtRomFile();			// 拡張ROMファイル名取得
	void SetExtRomFile( const std::filesystem::path& );		//                  設定
	
	const std::filesystem::path& GetTapeFile();				// TAPEファイル名取得
	void SetTapeFile( const std::filesystem::path& );		//               設定
	
	const std::filesystem::path& GetSaveFile();				// TAPE(SAVE)ファイル名取得
	void SetSaveFile( const std::filesystem::path& );		//                     設定
	
	const std::filesystem::path& GetDiskFile( int );		// DISKファイル名取得
	void SetDiskFile( int, const std::filesystem::path& );	//               設定
	
	const std::filesystem::path& GetPrinterFile();			// プリンタファイル名取得
	void SetPrinterFile( const std::filesystem::path& );	//                   設定
	
	// [PATH] ------------------------------------------------------
	const std::filesystem::path& GetRomPath();				// ROMパス取得
	void SetRomPath( const std::filesystem::path& );		//        設定
	
	const std::filesystem::path& GetTapePath();				// TAPEパス取得
	void SetTapePath( const std::filesystem::path& );		//         設定
	
	const std::filesystem::path& GetDiskPath();				// DISKパス取得
	void SetDiskPath( const std::filesystem::path& );		//         設定
	
	const std::filesystem::path& GetExtRomPath();			// 拡張ROMパス取得
	void SetExtRomPath( const std::filesystem::path& );		//            設定
	
	const std::filesystem::path& GetImgPath();				// スクリーンショット格納パス取得
	void SetImgPath( const std::filesystem::path& );		//                           設定
	
	const std::filesystem::path& GetWavePath();				// WAVEパス取得
	void SetWavePath( const std::filesystem::path& );		//         設定
	
	const std::filesystem::path& GetFontPath();				// フォントパス取得
	void SetFontPath( const std::filesystem::path& );		//             設定
	
	const std::filesystem::path& GetDokoSavePath();			// どこでもSAVEパス取得
	void SetDokoSavePath( const std::filesystem::path& );	//                 設定
	
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
	
	const std::filesystem::path GetDokoFile();				// どこでもSAVEファイル名取得
	void SetDokoFile( const std::filesystem::path& );		//                       設定
	
	// ------------------------------------------
	bool DokoSave( cIni* );		// どこでもSAVE
	bool DokoLoad( cIni* );		// どこでもLOAD
	// ------------------------------------------
};

#endif	// CONFIG_H_INCLUDED
