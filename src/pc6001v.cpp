#include <algorithm>
#include <clocale>
#include <memory>
#include <string>


#include "pc6001v.h"

#include "common.h"
#include "config.h"
#include "console.h"
#include "error.h"
#include "memory.h"
#include "osd.h"
#include "p6el.h"
#include "p6vm.h"
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
	int IniModel = cfg->GetValue( CV_Model );
	
	// 自動選定の時は飛ばす
	if( IniModel ){
		std::string files;
		bool resf = false;
		bool res  = true;
		
		const auto& roms = GetRomSetList( IniModel );
		for( auto& rom : roms ){
			resf = false;
			for( auto& file : rom ){
				std::vector<P6VPATH> ffiles;
				ffiles.clear();
				OSD_FindFile( cfg->GetValue( CF_RomPath ), STR2P6VPATH( file.FileName ), ffiles );
				for( auto& ff : ffiles ){
					if( OSD_GetFileSize( ff ) == file.Size ){
						resf = true;
						break;
					}
				}
			}
			if( !resf ){
				res = false;
				files += rom[0].FileName + "\n";
			}
		}
		if( res ){
			Error::Clear();
			return true;
		}else{
			Error::SetError( Error::NoRomChange, files );
			if( OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDM_YESNO | OSDM_ICONERROR ) != OSDR_YES ){
				Error::SetError( Error::NoRom );
				return false;
			}
		}
	}
	
	// 選定
	std::vector<int> models = { 60, 61, 62, 66, 64, 68 };
	for( size_t i=0; i<models.size(); i++ ){
		// 見つからなかった機種はスキップ
		if( IniModel == models[i] ) continue;
		
		bool resf = false;
		bool res = true;
		const auto& roms = GetRomSetList( models[i] );
		for( auto& rom : roms ){
			resf = false;
			for( auto& file : rom ){
				std::vector<P6VPATH> ffiles;
				OSD_FindFile( cfg->GetValue( CF_RomPath ), STR2P6VPATH( file.FileName ), ffiles );
				for( auto& ff : ffiles ){
					if( file.Size == OSD_GetFileSize( ff ) ){
						resf = true;
						break;
					}
				}
			}
			if( !resf ){ res = false; }
		}
		if( res ){
			cfg->SetValue( CV_Model, models[i] );
			// 自動選定の時はメッセージを出さない
			if( IniModel ){ Error::SetError( Error::RomChange ); }
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
			Error::Clear();
			break;
			
		default:
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			OSD_Quit();			// 終了処理
			return false;
		}
	}
	
	// 各種フォルダの存在チェック&作成
	std::vector<TCPath> paths = { CF_RomPath, CF_TapePath, CF_DiskPath, CF_ExtRomPath, CF_ImgPath, CF_WavePath, CF_FontPath, CF_DokoPath };
	for( auto& cf : paths ){
		if( !OSD_FileExist( Cfg->GetValue( cf ) ) ) OSD_CreateFolder( Cfg->GetValue( cf ) );
	}
	
	
	// フォントファイルチェック&作成
	if( !CheckFont( Cfg ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
		Error::Clear();
	}
	
	// コンソール用フォント読込み
	P6VPATH FontZ, FontH;
	OSD_AddPath( FontZ, Cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTZ ) );
	OSD_AddPath( FontH, Cfg->GetValue( CF_FontPath ), STR2P6VPATH( FILE_FONTH ) );
	
	if( !JFont::OpenFont( FontZ, FontH ) ){
		OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
		Error::Clear();
	}
	
	// P6VMオブジェクトを作成して実行
	do{
		std::unique_ptr<EL6> P6Core;			// P6VMオブジェクト
		
		// 再起動ならばINIファイル再読込み
		if( Restart == EL6::Restart ){
			if( !Cfg->Init() ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONWARNING );
				Error::Clear();
			}
			Restart = EL6::Quit;
		}
		
		// ROMファイル存在チェック&機種変更
		if( SearchRom( Cfg ) ){
			if( Error::GetError() != Error::NoError ){
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_WARNING ), OSDR_OK | OSDM_ICONWARNING );
				Error::Clear();
			}
		}else{
			OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			Error::Clear();
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
				int ret = OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_WARNING ), OSDM_YESNO | OSDM_ICONWARNING );
				Error::Clear();
				if( ret != OSDR_YES ) break;
				
				Cfg->SetValue( CB_CheckCRC, false );
				Cfg->Write();
				Restart = EL6::Restart;
			}else{
				// 初期化失敗
				OSD_Message( nullptr, Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Clear();
				break;			// do 抜ける
			}
		}
		
		switch( Restart ){
		case EL6::Dokoload:	// どこでもLOAD
			if( !P6Core->DokoDemoLoad( Cfg->GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Clear();
			}
			Cfg->SetDokoFile( "" );
			break;
			
		case EL6::ReplayPlay:	// リプレイ再生
		case EL6::ReplayResume:	// リプレイ保存再開
		case EL6::ReplayMovie:	// リプレイを動画に変換
			if( !P6Core->DokoDemoLoad( Cfg->GetDokoFile() ) ){
				// 失敗した場合
				OSD_Message( P6Core->GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
				Error::Clear();
				Cfg->SetDokoFile( "" );
			}
			break;
			
		default:
			break;
		}
		
		
		// 再起動でなければP6VM実行
		if( Restart != EL6::Restart ){
			Restart = P6Core->EventLoop( Restart );
		}
		
	}while( Restart != EL6::Quit );
	
	// INIファイル書込み
	if( Cfg->GetValue( CB_SaveQuit ) ){
		Cfg->Write();
	}
	
	// 終了処理
	OSD_Quit();
	
	return true;
}
