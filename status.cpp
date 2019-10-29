#include "p6el.h"

#include "log.h"
#include "status.h"
#include "common.h"
#include "cpus.h"
#include "disk.h"
#include "keyboard.h"
#include "replay.h"


//------------------------------------------------------
//  ステータスバークラス
//------------------------------------------------------
////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
cWndStat::cWndStat( VM6* pvm ) : vm(pvm), DrvNum(0), ReplayStatus(0) {}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
cWndStat::~cWndStat( void ){}


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
void cWndStat::Update( void )
{
	PRINTD( WIN_LOG, "[WndStat][Update]\n" );
	
	const BYTE Kana[]  = { 0x96, 0xe5, 0 };	// かな
	const BYTE KKana[] = { 0xb6, 0xc5, 0 };	// カナ
	
	ZCons::Cls();
	ZCons::SetColor( FC_WHITE );
	
	// TAPE
	ZCons::Locate( 0, 0 );
	ZCons::SPrint( "[TAPE]" );
	if( vm->CMTL::IsMount() ){
		ZCons::SetColor( vm->CMTL::IsAutoStart() ? FC_YELLOW : FC_WHITE );
		ZCons::SPrint( Stringf( " %-16s", vm->CMTL::GetName().empty() ? GetFileNamePart( vm->CMTL::GetFile() ).c_str() : vm->CMTL::GetName().c_str() ) );
		ZCons::SetColor( FC_WHITE );
		ZCons::Locate( ZCons::GetXline()-19, 0 );
		if( vm->IsCmtIntrReady() == LOADOPEN ) ZCons::SetColor( FC_WHITE, FC_MAGENTA );
		ZCons::SPrint( Stringf( "[%05d/%05d]", vm->CMTL::GetCount(), vm->CMTL::GetBetaSize() ) );
		ZCons::SetColor( FC_WHITE, FC_GRAY );
	}
	
	// DISK
	if( DrvNum > 0 ){
		if( vm->DskInAccess( 0 ) ) ZCons::SetColor( FC_WHITE, FC_RED );
		else					   ZCons::SetColor( FC_WHITE, FC_GRAY );
		ZCons::Locate( 0, 1 );
		ZCons::SPrint( "[DRV1]" );
		if( vm->DskIsMount( 0 ) ){
			ZCons::SetColor( vm->DskIsSystem( 0 ) ? FC_YELLOW : FC_WHITE, vm->DskIsProtect( 0 ) ? FC_DRED : FC_GRAY );
			ZCons::SPrint( Stringf( " %-16s", vm->DskGetName( 0 ).empty() ? GetFileNamePart( vm->DskGetFile( 0 ) ).c_str() : vm->DskGetName( 0 ).c_str() ) );
		}
	}
	if( DrvNum > 1 ){
		if( vm->DskInAccess( 1 ) ) ZCons::SetColor( FC_WHITE, FC_RED );
		else					   ZCons::SetColor( FC_WHITE, FC_GRAY );
		ZCons::Locate( 0, 2 );
		ZCons::SPrint( "[DRV2]" );
		if( vm->DskIsMount( 1 ) ){
			ZCons::SetColor( vm->DskIsSystem( 1 ) ? FC_YELLOW : FC_WHITE, vm->DskIsProtect( 1 ) ? FC_DRED : FC_GRAY );
			ZCons::SPrint( Stringf( " %-16s", vm->DskGetName( 1 ).empty() ? GetFileNamePart( vm->DskGetFile( 1 ) ).c_str() : vm->DskGetName( 1 ).c_str() ) );
		}
		// アクセスランプ
	}
	ZCons::SetColor( FC_WHITE, FC_GRAY );
	
	// かなキー
	ZCons::Locate( -5, 0 );
	switch( vm->KEY6::GetKeyIndicator() & 3 ){
	case KI_KANA:	// かな
		ZCons::PutCharH( Kana[0] );
		ZCons::PutCharH( Kana[1] );
		break;
	case KI_KKANA:	// カナ
		ZCons::PutCharH( KKana[0] );
		ZCons::PutCharH( KKana[1] );
	}
	
	// CAPSキー
	if( vm->KEY6::GetKeyIndicator() & 4 ) ZCons::SPrintcr( "ABC" );	// ABC
	else                                  ZCons::SPrintcr( "abc" );	// abc
	
	// リプレイステータス
	ZCons::Locate( -2, 0 );
	switch( ReplayStatus ){
	case REP_RECORD:	// 記録中
		ZCons::SetColor( FC_RED );
		ZCons::SPrint( "●" );
		break;
	case REP_REPLAY:	// 再生中
		ZCons::SetColor( FC_GREEN );
		ZCons::SPrint( "■" );
		break;
	}
}


////////////////////////////////////////////////////////////////
// リプレイステータスセット
//
// 引数:	stat		リプレイステータス
// 返値:	なし
////////////////////////////////////////////////////////////////
void cWndStat::SetReplayStatus( int stat )
{
	ReplayStatus = stat;
}
