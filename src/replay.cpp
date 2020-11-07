#include "log.h"
#include "replay.h"
#include "common.h"
#include "error.h"


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
REPLAY::REPLAY( void ) : RepST(REP_IDLE), RepFrm(0), EndFrm(0)
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
REPLAY::~REPLAY( void )
{
	switch( RepST ){
	case REP_RECORD:	StopRecord(); break;
	case REP_REPLAY:	StopReplay(); break;
	}
}


////////////////////////////////////////////////////////////////
// 初期化
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::Init( void )
{
	PRINTD( GRP_LOG, "[REPLAY][Init]\n" );
	
	cIni::Init();
	
	RepST   = REP_IDLE;
	RepFrm  = 0;
	EndFrm  = 0;
	
	return true;
}


////////////////////////////////////////////////////////////////
// ステータス取得
//
// 引数:	なし
// 返値:	int		ステータス
////////////////////////////////////////////////////////////////
int REPLAY::GetStatus( void ) const
{
	return RepST;
}


////////////////////////////////////////////////////////////////
// リプレイ記録開始
//
// 引数:	filepath	出力ファイルパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::StartRecord( const P6VPATH& filepath )
{
	// とりあえずエラー設定
	Error::SetError( Error::ReplayPlayError );
	try{
		if( RepST != REP_IDLE ) throw Error::ReplayRecError;
		
		if( !cIni::Read( filepath ) ) throw Error::ReplayRecError;
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		cIni::Init();
		return false;
	}
	
	RepFrm = 0;
	RepST  = REP_RECORD;
	
	// 無事だったのでエラーなし
	Error::Reset();
	
	return true;
}


////////////////////////////////////////////////////////////////
// リプレイ記録再開
//
// 引数:	filepath	出力ファイルパス
// 引数:	frame       途中再開するフレーム
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::ResumeRecord( const P6VPATH& filepath, int frame )
{
	if( !StartRecord( filepath ) ) return false;
	
	// 指定されたフレーム以降のリプレイを削除し、そこから再開
	cIni::DeleteAfter( "REPLAY", Stringf( "%08lX", frame ) );
	
	RepFrm = frame;
	return true;
}


////////////////////////////////////////////////////////////////
// リプレイ記録停止
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void REPLAY::StopRecord( void )
{
	if( RepST != REP_RECORD ) return;
	
	cIni::SetEntry( "REPLAY", "EndFrm", "", "0x%08lX", RepFrm );
	cIni::Write();
	cIni::Init();
	
	RepST = REP_IDLE;
}


////////////////////////////////////////////////////////////////
// リプレイ1フレーム書出し
//
// 引数:	mt		キーマトリクス
// 			chg		キーマトリクス変化 true:した false:しない
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::ReplayWriteFrame( const std::vector<BYTE>& mt, bool chg )
{
	std::string strva;
	
	if( RepST != REP_RECORD ) return false;
	
	// マトリクスを書出し
	for( auto &i : mt )
		strva += Stringf( "%02X", i );
	cIni::SetEntry( "REPLAY", Stringf( "%08lX ", RepFrm ), "", strva.c_str() );
	
	RepFrm++;
	
	return true;
}


////////////////////////////////////////////////////////////////
// リプレイ再生開始
//
// 引数:	filepath	入力ファイルパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::StartReplay( const P6VPATH& filepath )
{
	// とりあえずエラー設定
	Error::SetError( Error::ReplayPlayError );
	try{
		if( RepST != REP_IDLE ) throw Error::ReplayPlayError;
		
		if( !cIni::Read( filepath ) ) throw Error::ReplayPlayError;
		if( !cIni::GetVal( "REPLAY", "EndFrm", EndFrm ) ) throw Error::NoReplayData;
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		cIni::Init();
		return false;
	}
	
	RepFrm = 1;
	RepST  = REP_REPLAY;
	
	// 無事だったのでエラーなし
	Error::Reset();
	
	return true;
}


////////////////////////////////////////////////////////////////
// リプレイ再生停止
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void REPLAY::StopReplay( void )
{
	if( RepST != REP_REPLAY ) return;
	
	cIni::Init();
	
	RepST = REP_IDLE;
}


////////////////////////////////////////////////////////////////
// リプレイ1フレーム読込み
//
// 引数:	mt		キーマトリクス
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool REPLAY::ReplayReadFrame( std::vector<BYTE>& mt )
{
	std::string strva;
	
	if( RepST != REP_REPLAY ) return false;
	
	if( cIni::GetEntry( "REPLAY", Stringf( "%08lX", RepFrm ), strva ) ){
		strva.resize( mt.size() * 2, 'F' );
		int i = 0;
		for( auto &m : mt )
			m = std::stoul( strva.substr( i++ * 2, 2 ), nullptr, 16 );
	}
	
	if( ++RepFrm >= EndFrm )
		// データ終端に達したらリプレイ終了
		StopReplay();
	
	return true;
}
