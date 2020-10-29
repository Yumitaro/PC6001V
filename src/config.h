#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <string>
#include <vector>

#include "ini.h"
#include "keydef.h"
#include "vsurface.h"


// 設定項目
typedef enum {
	CF_Model = 0,
	CF_FDD,
	CF_ExtRam,
	CF_OverClock,
	CF_CheckCRC,
	CF_FDDWait,
	CF_TurboTAPE,
	CF_BoostUp,
	CF_MaxBoost60,
	CF_MaxBoost62,
	CF_StopBit,
	CF_Mode4Color,
	CF_ScanLine,
	CF_ScanLineBr,
	CF_Filtering,
	CF_DispNTSC,
	CF_FullScreen,
	CF_WindowZoom,
	CF_DispStatus,
	CF_FrameSkip,
	CF_SampleRate,
	CF_SoundBuffer,
	CF_MasterVolume,
	CF_PsgVolume,
	CF_PsgLPF,
	CF_VoiceVolume,
	CF_TapeVolume,
	CF_TapeLPF,
	CF_AviBpp,
	CF_ExtRom,
	CF_tape,
	CF_save,
	CF_disk1,
	CF_disk2,
	CF_printer,
	CF_RomPath,
	CF_TapePath,
	CF_DiskPath,
	CF_ExtRomPath,
	CF_ImgPath,
	CF_WavePath,
	CF_FontPath,
	CF_DokoPath,
	CF_CkQuit,
	CF_SaveQuit,
	CF_UseSoldier,
	CF_KeyRepeat
} TConfig;


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class CFG6 : public cIni {
protected:
	// INIファイルに保存しないメンバ
	P6VPATH IniPath;		// INIファイルパス
	P6VPATH DokoFile;		// どこでもSAVEファイル名
	std::string Caption;	// ウィンドウキャプション
	
	void InitIni( bool );									// INIオブジェクト初期値設定
	
	const std::string& GetPCKeyName( PCKEYsym );			// 仮想キーコードから名称取得
	const std::string& GetP6KeyName( P6KEYsym );			// P6キーコードから名称取得
	PCKEYsym GetPCKeyCode( const std::string& );			// キー名称から仮想キーコードを取得
	P6KEYsym GetP6KeyCode( const std::string& );			// キー名称からP6キーコードを取得
	
	void SetDefault( TConfig, bool );						// 初期値設定

public:
	CFG6();
	~CFG6();
	
	bool Init();											// 初期化(INIファイル読込み)
	bool Write();											// INIファイル書込み
	
	// メンバアクセス関数
	int GetValue( TConfig );								// 値 取得
	void SetValue( TConfig, int );							//    設定
	
	bool GetYesNo( TConfig );								// bool取得
	void SetYesNo( TConfig, bool );							//     設定
	
	P6VPATH GetPath( TConfig );								// path取得
	void SetPath( TConfig, const P6VPATH& );				//     設定
	
	
	// [KEY] -------------------------------------------------------
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
