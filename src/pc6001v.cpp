#include <algorithm>
#include <clocale>
#include <memory>
#include <string>


#include "common.h"
#include "config.h"
#include "console.h"
#include "error.h"
#include "memory.h"
#include "osd.h"
#include "p6el.h"
#include "p6vm.h"
#include "pc6001v.h"
#include "typedef.h"
#include "vsurface.h"


#include "SDL_main.h"


///////////////////////////////////////////////////////////
// フォントファイルチェック(無ければ作成する)
//
// 引数:	cfg			INIオブジェクト
// 返値:	bool		true: 成功 false:失敗
///////////////////////////////////////////////////////////
bool CheckFont( const std::shared_ptr<CFG6>& cfg )
{
	P6VPATH FontFile;
	
	OSD_AddPath( FontFile, cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTH ) );
	
	if( !OSD_FileExist( FontFile ) )
		if( !OSD_CreateFont( FontFile, "", FSIZE ) ){
			Error::SetError( Error::FontCreateFailed );
			return false;
		}
	
	OSD_AddPath( FontFile, cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTZ ) );
	if( !OSD_FileExist( FontFile ) )
		if( !OSD_CreateFont( "", FontFile, FSIZE ) ){
			Error::SetError( Error::FontCreateFailed );
			return false;
		}
	
	return true;
}


///////////////////////////////////////////////////////////
// ROMファイル存在チェック&機種変更
//
// 引数:	cfg			INIオブジェクト
// 返値:	bool		true: 成功 false:失敗
///////////////////////////////////////////////////////////
bool SearchRom( const std::shared_ptr<CFG6>& cfg )
{
	P6VPATH RomSearch;
	
	int IniModel = cfg->GetValue( CV_Model );
	
	// 自動選定の時は飛ばす
	if( IniModel ){
		bool res = true;
		const auto& roms = GetRomSetList( IniModel );
		for( auto &rom : roms ){
			bool resf = false;
			for( auto &file : rom ){
				OSD_AddPath( RomSearch, cfg->GetValue( CF_RomPath ), STR2P6VPATH( file.FileName ) );
				if( OSD_FileExist( RomSearch ) ) resf = true;
			}
			if( !resf ) res = false;
			resf = false;
		}
		if( res ){
			Error::Reset();
			return true;
		}
	}
	
	// 選定
	std::vector<int> models = { 60, 61, 62, 66, 64, 68 };
	for( size_t i=0; i<models.size(); i++ ){
		// 見つからなかった機種はスキップ
		if( IniModel == models[i] ) continue;
		
		bool res = true;
		const auto& roms = GetRomSetList( models[i] );
		for( auto &rom : roms ){
			bool resf = false;
			for( auto &file : rom ){
				OSD_AddPath( RomSearch, cfg->GetValue( CF_RomPath ), STR2P6VPATH( file.FileName ) );
				if( OSD_FileExist( RomSearch ) ) resf = true;
			}
			if( !resf ) res = false;
			resf = false;
		}
		if( res ){
			cfg->SetValue( CV_Model, models[i] );
			// 自動選定の時はメッセージを出さない
			if( IniModel ) Error::SetError( Error::RomChange );
			return true;
		}
	}
	
	Error::SetError( Error::NoRom );
	return false;
}


///////////////////////////////////////////////////////////
// メイン
///////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
	std::shared_ptr<CFG6> Cfg;				// 環境設定オブジェクト
	EL6::ReturnCode Restart = EL6::Quit;	// 再起動フラグ
	
	
	std::setlocale( LC_CTYPE, "" );
	
	// 二重起動禁止
	if( OSD_IsWorking() ) return false;
	
	// 環境設定オブジェクト確保
	try{
		Cfg = std::make_shared<CFG6>();
	}
	catch( std::bad_alloc& ){
		return false;
	}
	
	// OSD関連初期化
	if( !OSD_Init() ){
		Error::SetError( Error::InitFailed );
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		OSD_Quit();	// 終了処理
		return false;
	}
	
	// 設定ファイルフォルダの存在チェック&作成
	if( !OSD_FileExist( OSD_GetConfigPath() ) ) OSD_CreateFolder( OSD_GetConfigPath() );
	
	// INIファイル読込み
	if( !Cfg->Init() ){
		switch( Error::GetError() ){
		case Error::IniDefault:
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
			Error::Reset();
			break;
			
		default:
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			OSD_Quit();			// 終了処理
			return false;
		}
	}
	
	// 各種フォルダの存在チェック&作成
	if( !OSD_FileExist( Cfg->GetValue( CF_RomPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_RomPath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_TapePath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_TapePath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_DiskPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_DiskPath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_ExtRomPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_ExtRomPath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_ImgPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_ImgPath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_WavePath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_WavePath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_FontPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_FontPath ) );
	if( !OSD_FileExist( Cfg->GetValue( CF_DokoPath ) ) )	OSD_CreateFolder( Cfg->GetValue( CF_DokoPath ) );
	
	
	// フォントファイルチェック&作成
	if( !CheckFont( Cfg ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
		Error::Reset();
	}
	
	// コンソール用フォント読込み
	P6VPATH FontZ, FontH;
	OSD_AddPath( FontZ, Cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTZ ) );
	OSD_AddPath( FontH, Cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTH ) );
	
	if( !JFont::OpenFont( FontZ, FontH ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		Error::Reset();
	}
	
	// P6VMオブジェクトを作成して実行
	do{
		std::unique_ptr<EL6> P6Core;			// P6VMオブジェクト
		
		// 再起動ならばINIファイル再読込み
		if( Restart == EL6::Restart ){
			if( !Cfg->Init() ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
				Error::Reset();
			}
			Restart = EL6::Quit;
		}
		
		// ROMファイル存在チェック&機種変更
		if( SearchRom( Cfg ) ){
			if( Error::GetError() != Error::NoError ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
				Error::Reset();
			}
		}else{
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			Error::Reset();
			break;	// do 抜ける
		}
		
		// P6VMオブジェクト確保
		try{
			P6Core = std::make_unique<EL6>();
		}
		catch( std::bad_alloc& ){
			break;
		}
		
		// P6VMオブジェクト初期化
		if( !P6Core || !P6Core->Init( Cfg ) ){
			// 失敗した場合
			if( Error::GetError() == Error::RomCrcNG ){
				// CRCが合わない場合
				int ret = OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDM_YESNO | OSDM_ICONWARNING );
				Error::Reset();
				if( ret != OSDR_YES ) break;
				
				Cfg->SetValue( CB_CheckCRC, false );
				Cfg->Write();
				Restart = EL6::Restart;
			}else{
				// 初期化失敗
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Reset();
				break;			// do 抜ける
			}
		}
		
		switch( Restart ){
		case EL6::Dokoload:	// どこでもLOAD?
			if( !P6Core->DokoDemoLoad( Cfg->GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Reset();
			}
			Cfg->SetDokoFile( "" );
			break;
			
		case EL6::Replay:	// リプレイ再生?
			if( !P6Core->DokoDemoLoad( Cfg->GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Reset();
			}
			P6Core->REPLAY::StartReplay( Cfg->GetDokoFile() );
			Cfg->SetDokoFile( "" );
			break;
			
		default:
			break;
		}
		
		
		// 再起動でなければP6VM実行
		if( Restart != EL6::Restart ){
			P6Core->Start();
			Restart = P6Core->EventLoop();
			P6Core->Stop();
		}
		
	}while( Restart != EL6::Quit );
	
	// INIファイル書込み
	if( Cfg->GetValue( CB_SaveQuit ) ) Cfg->Write();
	
	// 終了処理
	OSD_Quit();
	
	return true;
}
