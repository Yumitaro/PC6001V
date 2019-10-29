#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

#include "pc6001v.h"
#include "typedef.h"

#include "common.h"
#include "config.h"
#include "console.h"
#include "error.h"
#include "memory.h"
#include "p6el.h"
#include "p6vm.h"
#include "osd.h"
#include "vsurface.h"


#include "SDL_main.h"


///////////////////////////////////////////////////////////
// フォントファイルチェック(無ければ作成する)
//
// 引数:	cfg			INIオブジェクトポインタ
// 返値:	bool		true: 成功 false:失敗
///////////////////////////////////////////////////////////
bool CheckFont( CFG6* cfg )
{
	std::filesystem::path FontFile;
	
	AddPath( FontFile, cfg->GetFontPath(), std::filesystem::u8path( FILE_FONTH ) );
	
	if( !FileExist( FontFile ) )
		if( !OSD_CreateFont( FontFile, "", FSIZE ) ){
			Error::SetError( Error::FontCreateFailed );
			return false;
		}
	
	AddPath( FontFile, cfg->GetFontPath(), std::filesystem::u8path( FILE_FONTZ ) );
	if( !FileExist( FontFile ) )
		if( !OSD_CreateFont( "", FontFile, FSIZE ) ){
			Error::SetError( Error::FontCreateFailed );
			return false;
		}
	
	return true;
}


///////////////////////////////////////////////////////////
// ROMファイル存在チェック&機種変更
//
// 引数:	cfg			INIオブジェクトポインタ
// 返値:	bool		true: 成功 false:失敗
///////////////////////////////////////////////////////////
bool SearchRom( CFG6* cfg )
{
	std::filesystem::path RomSearch;
	
	int IniModel = cfg->GetModel();
	
	// 自動選定の時は飛ばす
	if( IniModel ){
		bool res = true;
		std::vector<std::vector<ROMINFO>> roms = GetRomSetList( IniModel );
		for( auto &rom : roms ){
			bool resf = false;
			for( auto &file : rom ){
				AddPath( RomSearch, cfg->GetRomPath(), std::filesystem::u8path( file.FileName ) );
				if( FileExist( RomSearch ) ) resf = true;
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
		std::vector<std::vector<ROMINFO>> roms = GetRomSetList( models[i] );
		for( auto &rom : roms ){
			bool resf = false;
			for( auto &file : rom ){
				AddPath( RomSearch, cfg->GetRomPath(), std::filesystem::u8path( file.FileName ) );
				if( FileExist( RomSearch ) ) resf = true;
			}
			if( !resf ) res = false;
			resf = false;
		}
		if( res ){
			cfg->SetModel( models[i] );
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
	EL6::ReturnCode Restart = EL6::Quit;	// 再起動フラグ
	CFG6 Cfg;								// 環境設定オブジェクト
	
	
	setlocale( LC_CTYPE, "" );
	
	// 二重起動禁止
	if( OSD_IsWorking() ) return false;
	
	// OSD関連初期化
	if( !OSD_Init() ){
		Error::SetError( Error::InitFailed );
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		OSD_Quit();	// 終了処理
		return false;
	}
	
	// INIファイル読込み
	if( !Cfg.Init() ){
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
	
	// フォルダの存在チェック&作成
	if( !FileExist( Cfg.GetRomPath() ) )		CreateFolder( Cfg.GetRomPath() );
	if( !FileExist( Cfg.GetTapePath() ) )		CreateFolder( Cfg.GetTapePath() );
	if( !FileExist( Cfg.GetDiskPath() ) )		CreateFolder( Cfg.GetDiskPath() );
	if( !FileExist( Cfg.GetExtRomPath() ) )		CreateFolder( Cfg.GetExtRomPath() );
	if( !FileExist( Cfg.GetImgPath() ) )		CreateFolder( Cfg.GetImgPath() );
	if( !FileExist( Cfg.GetWavePath() ) )		CreateFolder( Cfg.GetWavePath() );
	if( !FileExist( Cfg.GetFontPath() ) )		CreateFolder( Cfg.GetFontPath() );
	if( !FileExist( Cfg.GetDokoSavePath() ) )	CreateFolder( Cfg.GetDokoSavePath() );
	
	
	// フォントファイルチェック&作成
	if( !CheckFont( &Cfg ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
		Error::Reset();
	}
	
	// コンソール用フォント読込み
	std::filesystem::path FontZ, FontH;
	AddPath( FontZ, Cfg.GetFontPath(), std::filesystem::u8path( FILE_FONTZ ) );
	AddPath( FontH, Cfg.GetFontPath(), std::filesystem::u8path( FILE_FONTH ) );
	if( !JFont::OpenFont( FontZ, FontH ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		Error::Reset();
	}
	
	
	
	// P6オブジェクトを作成して実行
	do{
		// 再起動ならばINIファイル再読込み
		if( Restart == EL6::Restart ){
			if( !Cfg.Init() ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
				Error::Reset();
			}
			Restart = EL6::Quit;
		}
		
		// ROMファイル存在チェック&機種変更
		if( SearchRom( &Cfg ) ){
			if( Error::GetError() != Error::NoError ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
				Error::Reset();
			}
		}else{
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			Error::Reset();
			break;	// do 抜ける
		}
		
		// 機種別P6オブジェクト確保
		std::unique_ptr<EL6> P6Core( new EL6() );
		if( !P6Core ){
			break;	// do 抜ける
		}
		
		// VM初期化
		if( !P6Core->Init( &Cfg ) ){
			// 失敗した場合
			if( Error::GetError() == Error::RomCrcNG ){
				// CRCが合わない場合
				int ret = OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDM_YESNO | OSDM_ICONWARNING );
				Error::Reset();
				if( ret != OSDR_YES ) break;
				
				Cfg.SetCheckCRC( false );
				Cfg.Write();
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
			if( !P6Core->DokoDemoLoad( Cfg.GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Reset();
			}
			Cfg.SetDokoFile( "" );
			break;
			
		case EL6::Replay:	// リプレイ再生?
			if( !P6Core->DokoDemoLoad( Cfg.GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Reset();
			}
			P6Core->REPLAY::StartReplay( Cfg.GetDokoFile() );
			Cfg.SetDokoFile( "" );
			break;
			
		default:
			break;
		}
		
		
		// 再起動でなければVM実行
		if( Restart != EL6::Restart ){
			P6Core->Start();
			Restart = P6Core->EventLoop();
			P6Core->Stop();
		}
		
	}while( Restart != EL6::Quit );
	
	// INIファイル書込み
	if( Cfg.GetSaveQuit() ) Cfg.Write();
	
	// 終了処理
	OSD_Quit();
	
	return true;
}
