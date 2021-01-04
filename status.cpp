#include "common.h"
#include "cpus.h"
#include "disk.h"
#include "keyboard.h"
#include "log.h"
#include "osd.h"
#include "p6el.h"
#include "replay.h"
#include "status.h"


//------------------------------------------------------
//  ステータスバークラス
//------------------------------------------------------
////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
cWndStat::cWndStat( void ) : DrvNum( 0 ), Indicator( ST_IDLE )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
cWndStat::~cWndStat( void )
{
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool cWndStat::Init( int w, int drv )
{
	PRINTD( WIN_LOG, "[WndStat][Init]\n" );
	
	if( drv >= 0 ) DrvNum = drv;
	ZCons::SetColor( FC_WHITE, FC_GRAY );
	
	return ZCons::InitRes( w < 0 ? VSurface::Width() : w, JFont::FontHeight() * (DrvNum + 1) + 4, "", FC_WHITE, FC_GRAY );
}


////////////////////////////////////////////////////////////////
// ウィンドウ更新
////////////////////////////////////////////////////////////////
void cWndStat::Update( const std::shared_ptr<VM6>& vm )
{
	PRINTD( WIN_LOG, "[WndStat][Update]\n" );
	
	const BYTE Kana[]  = { 0x96, 0xe5, 0 };	// かな
	const BYTE KKana[] = { 0xb6, 0xc5, 0 };	// カナ
	
	ZCons::Cls();
	ZCons::SetColor( FC_WHITE );
	
	// TAPE
	ZCons::Locate( 0, 0 );
	ZCons::Printf( "[TAPE]" );
	if( vm->CmtlIsMount() ){
		ZCons::SetColor( vm->CmtlIsAutoStart() ? FC_YELLOW : FC_WHITE );
		ZCons::Printf( " %-16s", vm->CmtlGetName().empty() ? OSD_GetFileNamePart( vm->CmtlGetFile() ).c_str() : vm->CmtlGetName().c_str() );
		ZCons::SetColor( FC_WHITE );
		ZCons::Locate( ZCons::GetXline()-19, 0 );
		if( vm->CpusIsCmtIntrReady() == LOADOPEN ){
			ZCons::SetColor( FC_WHITE, FC_MAGENTA );
		}
		ZCons::Printf( "[%05d/%05d]", vm->CmtlGetCount(), vm->CmtlGetBetaSize() );
		ZCons::SetColor( FC_WHITE, FC_GRAY );
	}
	
	// DISK
	if( DrvNum > 0 ){
		if( vm->DskInAccess( 0 ) ){ ZCons::SetColor( FC_WHITE, FC_RED  ); }
		else					  { ZCons::SetColor( FC_WHITE, FC_GRAY ); }
		ZCons::Locate( 0, 1 );
		ZCons::Printf( "[DRV1]" );
		if( vm->DskIsMount( 0 ) ){
			ZCons::SetColor( vm->DskIsSystem( 0 ) ? FC_YELLOW : FC_WHITE, vm->DskIsProtect( 0 ) ? FC_DRED : FC_GRAY );
			ZCons::Printf( " %-16s", vm->DskGetName( 0 ).empty() ? OSD_GetFileNamePart( vm->DskGetFile( 0 ) ).c_str() : vm->DskGetName( 0 ).c_str() );
		}
	}
	if( DrvNum > 1 ){
		if( vm->DskInAccess( 1 ) ){ ZCons::SetColor( FC_WHITE, FC_RED  ); }
		else					  { ZCons::SetColor( FC_WHITE, FC_GRAY ); }
		ZCons::Locate( 0, 2 );
		ZCons::Printf( "[DRV2]" );
		if( vm->DskIsMount( 1 ) ){
			ZCons::SetColor( vm->DskIsSystem( 1 ) ? FC_YELLOW : FC_WHITE, vm->DskIsProtect( 1 ) ? FC_DRED : FC_GRAY );
			ZCons::Printf( " %-16s", vm->DskGetName( 1 ).empty() ? OSD_GetFileNamePart( vm->DskGetFile( 1 ) ).c_str() : vm->DskGetName( 1 ).c_str() );
		}
		// アクセスランプ
	}
	ZCons::SetColor( FC_WHITE, FC_GRAY );
	
	// かなキー
	ZCons::Locate( -5, 0 );
	switch( vm->KeyGetKeyIndicator() & (KI_KANA|KI_KKANA) ){
	case KI_KANA:	// かな
		ZCons::PutCharH( Kana[0] );
		ZCons::PutCharH( Kana[1] );
		break;
	case KI_KKANA:	// カナ
		ZCons::PutCharH( KKana[0] );
		ZCons::PutCharH( KKana[1] );
	}
	
	// CAPSキー
	if( vm->KeyGetKeyIndicator() & KI_CAPS ){ ZCons::PrintfR( "ABC" ); }	// ABC
	else                                    { ZCons::PrintfR( "abc" ); }	// abc
	
	// インジケータ
	ZCons::Locate( -2, 0 );
	switch( Indicator ){
	case ST_REPLAYREC:					// リプレイ記録中
		ZCons::SetColor( FC_RED );
		ZCons::PrintfR( "●" );
		break;
		
	case ST_REPLAYPLAY:					// リプレイ再生中
		ZCons::SetColor( FC_GREEN );
		ZCons::PrintfR( "■" );
		break;
		
	case ST_CAPTUREREC:					// ビデオキャプチャ中
		ZCons::SetColor( FC_RED );
		ZCons::PrintfR( "◎" );
		break;
		
	case ST_REPLAYPLAY|ST_CAPTUREREC:	// リプレイ再生中＆ビデオキャプチャ中
		ZCons::SetColor( FC_RED );
		ZCons::PrintfR( "◆" );
		break;
	}
}


////////////////////////////////////////////////////////////////
// インジケータセット
//
// 引数:	ind			インジケータID
// 返値:	なし
////////////////////////////////////////////////////////////////
void cWndStat::SetIndicator( DWORD ind )
{
	Indicator = ind;
}
