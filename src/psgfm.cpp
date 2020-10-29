#include "pc6001v.h"

#include "common.h"
#include "log.h"
#include "psgfm.h"
#include "schedule.h"

#include "p6el.h"
#include "p6vm.h"

// イベントID
#define	EID_PSG		(1)
#define	EID_TIMERA	(2)
#define	EID_TIMERB	(3)

// ポートアクセスウェイトクロック数
#define PAWAIT		(1)


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
PSGb::PSGb( VM6* vm, const ID& id ) : Device( vm, id ), JoyNo( 0 ), Clock( 0 )
{
}

PSG60::PSG60( VM6* vm, const ID& id ) : PSGb( vm, id )
{
	// Dvice Description (Out)
	descs.outdef.emplace( outA0H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA0H ) );
	descs.outdef.emplace( outA1H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA1H ) );
	descs.outdef.emplace( outA3H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA3H ) );
	
	// Dvice Description (In)
	descs.indef.emplace ( inA2H,  STATIC_CAST( Device::InFuncPtr,  &PSG60::InA2H  ) );
}

OPN64::OPN64( VM6* vm, const ID& id ) : PSGb( vm, id )
{
	// Dvice Description (Out)
	descs.outdef.emplace( outA0H, STATIC_CAST( Device::OutFuncPtr, &OPN64::OutA0H ) );
	descs.outdef.emplace( outA1H, STATIC_CAST( Device::OutFuncPtr, &OPN64::OutA1H ) );
	descs.outdef.emplace( outA3H, STATIC_CAST( Device::OutFuncPtr, &OPN64::OutA3H ) );
	
	// Dvice Description (In)
	descs.indef.emplace ( inA2H,  STATIC_CAST( Device::InFuncPtr,  &OPN64::InA2H  ) );
	descs.indef.emplace ( inA3H,  STATIC_CAST( Device::InFuncPtr,  &OPN64::InA3H  ) );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
PSGb::~PSGb( void )
{
}

PSG60::~PSG60( void )
{
}

OPN64::~OPN64( void )
{
}


////////////////////////////////////////////////////////////////
// イベントコールバック関数
//
// 引数:	id		イベントID
//			clock	クロック
// 返値:	なし
////////////////////////////////////////////////////////////////
void PSG60::EventCallback( int id, int clock )
{
	switch( id ){
	case EID_PSG:
		break;
	}
}

void OPN64::EventCallback( int id, int clock )
{
	switch( id ){
	case EID_PSG:
		break;
	
	case EID_TIMERA:
	case EID_TIMERB:
		cYM2203::TimerIntr();
		break;
	}
}


////////////////////////////////////////////////////////////////
// 更新サンプル数取得
//   若干の誤差を含むが実用上(多分)問題なし
////////////////////////////////////////////////////////////////
int PSGb::GetUpdateSamples( void )
{
	int samples = (int)( (double)SndDev::SampleRate * vm->EventGetProgress( this->Device::GetID(), EID_PSG ) + 0.5 );
	vm->EventReset( this->Device::GetID(), EID_PSG );
	return samples;
}


////////////////////////////////////////////////////////////////
// レジスタ変更前のストリーム更新
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void PSG60::PreWriteReg( void )
{
	SoundUpdate( 0 );
}


void OPN64::PreWriteReg( void )
{
	SoundUpdate( 0 );
}


////////////////////////////////////////////////////////////////
// TimerA設定
////////////////////////////////////////////////////////////////
void OPN64::SetTimerA( int cnt )
{
	double ct = 72. * (1024. - (double)cnt) / (double)Clock * 1000000.;
	
	if( cnt ) vm->EventAdd( Device::GetID(), EID_TIMERA, ct, EV_LOOP|EV_US );
	else	  vm->EventDel( Device::GetID(), EID_TIMERA );
}


////////////////////////////////////////////////////////////////
// TimerB設定
////////////////////////////////////////////////////////////////
void OPN64::SetTimerB( int cnt )
{
	double ct = 1152. * (256. - (double)cnt) / (double)Clock * 1000000.;
	
	if( cnt ) vm->EventAdd( Device::GetID(), EID_TIMERB, ct, EV_LOOP|EV_US );
	else	  vm->EventDel( Device::GetID(), EID_TIMERB );
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool PSG60::Init( int clock, int srate )
{
	PRINTD( PSG_LOG, "[PSG][Init]\n" );
	
	Clock = clock;
	
	// PSG/OPN クロック設定
	InitMod( Clock, srate );
	
	// PSG/OPN ボリュームテーブル設定
	SetVolumeTable( DEFAULT_PSGVOL );
	
	// リセット
	cAY8910::Reset();
	
	// 少なくとも1秒に1回くらいは更新するだろうという前提
	if( !vm->EventAdd( Device::GetID(), EID_PSG, 1000, EV_LOOP|EV_MS ) ) return false;
	
	return SndDev::Init( srate );
}


bool OPN64::Init( int clock, int srate )
{
	PRINTD( PSG_LOG, "[OPN][Init]\n" );
	
	Clock = clock;
	
	// PSG/OPN クロック設定
	InitMod( Clock, srate );
	
	// PSG/OPN ボリュームテーブル設定
	SetVolumeTable( DEFAULT_PSGVOL );
	
	// リセット
	cYM2203::Reset();
	
	// 少なくとも1秒に1回くらいは更新するだろうという前提
	if( !vm->EventAdd( Device::GetID(), EID_PSG, 1000, EV_LOOP|EV_MS ) ) return false;
	
	return SndDev::Init( srate );
}



////////////////////////////////////////////////////////////////
// サンプリングレート設定
//
// 引数:	rate	サンプリングレート
//			size	バッファサイズ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool PSG60::SetSampleRate( int rate, int size )
{
	cAY8910::SetClock( Clock, rate );
	
	return SndDev::SetSampleRate( rate, size );
}


bool OPN64::SetSampleRate( int rate, int size )
{
	cYM2203::SetClock( Clock, rate );
	
	return SndDev::SetSampleRate( rate, size );
}


////////////////////////////////////////////////////////////////
// ストリーム更新
//
// 引数:	samples	更新するサンプル数(0:処理クロック分)
// 返値:	int		残りサンプル数
////////////////////////////////////////////////////////////////
int PSG60::SoundUpdate( int samples )
{
	int length = min( max( 0, samples ? samples : GetUpdateSamples() ), SndDev::cRing::FreeSize() );
	
	PRINTD( PSG_LOG, "[PSG][SoundUpdate] Samples: %d -> %d\n", samples, length );
	
	// バッファに書込み
	for( int i=0; i<length; i++ ){
		SndDev::cRing::Put( ( this->Update1Sample() * SndDev::Volume ) / 100 );
	}
	
	return SndDev::cRing::ReadySize();
}

int OPN64::SoundUpdate( int samples )
{
	int length = min( max( 0, samples ? samples : GetUpdateSamples() ), SndDev::cRing::FreeSize() );
	
	PRINTD( PSG_LOG, "[OPN][SoundUpdate] Samples: %d -> %d\n", samples, length );
	
	// バッファに書込み
	for( int i=0; i<length; i++ ){
		SndDev::cRing::Put( ( this->Update1Sample() * SndDev::Volume ) / 100 );
	}
	
	return SndDev::cRing::ReadySize();
}


////////////////////////////////////////////////////////////////
// ポートアクセス関数
////////////////////////////////////////////////////////////////
BYTE PSG60::PortAread( void ){ return vm->KeyGetJoy( JoyNo ); }
void PSG60::PortBwrite( BYTE data ){ JoyNo = (~data>>6)&1; }

BYTE OPN64::PortAread( void ){ return vm->KeyGetJoy( JoyNo ); }
void OPN64::PortBwrite( BYTE data ){ JoyNo = (~data>>6)&1; }


////////////////////////////////////////////////////////////////
// I/Oアクセス関数
////////////////////////////////////////////////////////////////
// PSGレジスタアドレスラッチ
void PSG60::OutA0H( int, BYTE data )
{
	this->WriteReg( 0, data );
}

// PSGライトデータ
void PSG60::OutA1H( int, BYTE data )
{
	this->WriteReg( 1, data );
}

// PSGインアクティブ
void PSG60::OutA3H( int, BYTE data ){}

// PSGリードデータ
BYTE PSG60::InA2H( int )
{
	return this->ReadReg();
}


// OPNレジスタアドレスラッチ
void OPN64::OutA0H( int, BYTE data )
{
	this->WriteReg( 0, data );
}

// OPNライトデータ
void OPN64::OutA1H( int, BYTE data )
{
	this->WriteReg( 1, data );
}

// OPNインアクティブ
void OPN64::OutA3H( int, BYTE data ){}

// OPNリードデータ
BYTE OPN64::InA2H( int )
{
	return this->ReadReg();
}

// OPNステータスリード
BYTE OPN64::InA3H( int )
{
	return this->ReadStatus();
}



////////////////////////////////////////////////////////////////
// どこでもSAVE
//
// 引数:	Ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool PSG60::DokoSave( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->PutValue( "PSG", "RegisterLatch",	"",	RegisterLatch,	"0x%02X" );
	Ini->PutValue( "PSG", "LastEnable",		"",	LastEnable,		"0x%02X" );

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->PutValue( "PSG", stren, "", reg[i], "0x%02X" );
	}
	
	Ini->PutValue( "PSG", "envelop", "", (envelop - enveloptable)/64 );
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->PutValue( "PSG", stren, "", olevel[i] );
		sprintf( stren, "scount_%02d", i );
		Ini->PutValue( "PSG", stren, "", scount[i], "0x%08X" );
		sprintf( stren, "speriod_%02d", i );
		Ini->PutValue( "PSG", stren, "", speriod[i], "0x%08X" );
	}
	Ini->PutValue( "PSG", "ecount",		"",	ecount,			"0x%08X" );
	Ini->PutValue( "PSG", "eperiod",		"",	eperiod,		"0x%08X" );
	Ini->PutValue( "PSG", "ncount",		"",	ncount,			"0x%08X" );
	Ini->PutValue( "PSG", "nperiod",		"",	nperiod,		"0x%08X" );
	
	Ini->PutValue( "PSG", "tperiodbase",	"",	tperiodbase,	"0x%08X" );
	Ini->PutValue( "PSG", "eperiodbase",	"",	eperiodbase,	"0x%08X" );
	Ini->PutValue( "PSG", "nperiodbase",	"",	nperiodbase,	"0x%08X" );
	
	Ini->PutValue( "PSG", "mask",			"", mask );
*/
#else
	for( int i=0; i<16; i++ )
		Ini->PutValue( "PSG", Stringf( "Regs_%02d", i ), "", Regs[i], "0x%02X" );
	
	Ini->PutValue( "PSG", "PeriodA",		"", PeriodA );
	Ini->PutValue( "PSG", "PeriodB",		"", PeriodB );
	Ini->PutValue( "PSG", "PeriodC",		"", PeriodC );
	Ini->PutValue( "PSG", "PeriodN",		"", PeriodN );
	Ini->PutValue( "PSG", "PeriodE",		"", PeriodE );
	Ini->PutValue( "PSG", "CountA",		"", CountA );
	Ini->PutValue( "PSG", "CountB",		"", CountB );
	Ini->PutValue( "PSG", "CountC",		"", CountC );
	Ini->PutValue( "PSG", "CountN",		"", CountN );
	Ini->PutValue( "PSG", "CountE",		"", CountE );
	Ini->PutValue( "PSG", "VolA",			"", VolA );
	Ini->PutValue( "PSG", "VolB",			"", VolB );
	Ini->PutValue( "PSG", "VolC",			"", VolC );
	Ini->PutValue( "PSG", "VolE",			"", VolE );
	Ini->PutValue( "PSG", "EnvelopeA",	"",	EnvelopeA,	"0x%02X" );
	Ini->PutValue( "PSG", "EnvelopeB",	"",	EnvelopeB,	"0x%02X" );
	Ini->PutValue( "PSG", "EnvelopeC",	"",	EnvelopeC,	"0x%02X" );
	Ini->PutValue( "PSG", "OutputA",		"",	OutputA,	"0x%02X" );
	Ini->PutValue( "PSG", "OutputB",		"",	OutputB,	"0x%02X" );
	Ini->PutValue( "PSG", "OutputC",		"",	OutputC,	"0x%02X" );
	Ini->PutValue( "PSG", "OutputN",		"",	OutputN,	"0x%02X" );
	Ini->PutValue( "PSG", "CountEnv",		"", CountEnv );
	Ini->PutValue( "PSG", "Hold",			"",	Hold,		"0x%02X" );
	Ini->PutValue( "PSG", "Alternate",	"",	Alternate,	"0x%02X" );
	Ini->PutValue( "PSG", "Attack",		"",	Attack,		"0x%02X" );
	Ini->PutValue( "PSG", "Holding",		"",	Holding,	"0x%02X" );
	Ini->PutValue( "PSG", "RNG",			"", RNG );
#endif
	
	return true;
}


bool OPN64::DokoSave( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->PutValue( "OPN", "RegisterLatch",	"",	RegisterLatch,	"0x%02X" );
	Ini->PutValue( "OPN", "LastEnable",		"",	LastEnable,		"0x%02X" );
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOAD
//
// 引数:	Ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool PSG60::DokoLoad( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->GetValue( "PSG", "RegisterLatch",	RegisterLatch );
	Ini->GetValue( "PSG", "LastEnable",		LastEnable    );

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->GetValue( "PSG", stren, reg[i] );
	}
	
	st = (envelop - enveloptable)/64;
	Ini->GetValue( "PSG", "envelop",	st );
	envelop = enveloptable[st];
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->GetValue( "PSG", stren, olevel[i] );
		sprintf( stren, "scount_%02d", i );
		Ini->GetValue( "PSG", stren, scount[i] );
		sprintf( stren, "speriod_%02d", i );
		Ini->GetValue( "PSG", stren, speriod[i] );
	}
	
	Ini->GetValue( "PSG", "ecount",		ecount      );
	Ini->GetValue( "PSG", "eperiod",		eperiod     );
	Ini->GetValue( "PSG", "ncount",		ncount      );
	Ini->GetValue( "PSG", "nperiod",		nperiod     );
	Ini->GetValue( "PSG", "tperiodbase",	tperiodbase );
	Ini->GetValue( "PSG", "eperiodbase",	eperiodbase );
	Ini->GetValue( "PSG", "nperiodbase",	nperiodbase );
	
	Ini->GetValue( "PSG", "mask",		mask        );
*/
#else
	for( int i=0; i<16; i++ ){
		Ini->GetValue( "PSG", Stringf( "Regs_%02d", i ), Regs[i] );
	}
	Ini->GetValue( "PSG", "PeriodA",		PeriodA   );
	Ini->GetValue( "PSG", "PeriodB",		PeriodB   );
	Ini->GetValue( "PSG", "PeriodC",		PeriodC   );
	Ini->GetValue( "PSG", "PeriodN",		PeriodN   );
	Ini->GetValue( "PSG", "PeriodE",		PeriodE   );
	Ini->GetValue( "PSG", "CountA",		CountA    );
	Ini->GetValue( "PSG", "CountB",		CountB    );
	Ini->GetValue( "PSG", "CountC",		CountC    );
	Ini->GetValue( "PSG", "CountN",		CountN    );
	Ini->GetValue( "PSG", "CountE",		CountE    );
	Ini->GetValue( "PSG", "VolA",		VolA      );
	Ini->GetValue( "PSG", "VolB",		VolB      );
	Ini->GetValue( "PSG", "VolC",		VolC      );
	Ini->GetValue( "PSG", "VolE",		VolE      );
	Ini->GetValue( "PSG", "EnvelopeA",	EnvelopeA );
	Ini->GetValue( "PSG", "EnvelopeB",	EnvelopeB );
	Ini->GetValue( "PSG", "EnvelopeC",	EnvelopeC );
	Ini->GetValue( "PSG", "OutputA",		OutputA   );
	Ini->GetValue( "PSG", "OutputB",		OutputB   );
	Ini->GetValue( "PSG", "OutputC",		OutputC   );
	Ini->GetValue( "PSG", "OutputN",		OutputN   );
	Ini->GetValue( "PSG", "CountEnv",	CountEnv  );
	Ini->GetValue( "PSG", "Hold",		Hold      );
	Ini->GetValue( "PSG", "Alternate",	Alternate );
	Ini->GetValue( "PSG", "Attack",		Attack    );
	Ini->GetValue( "PSG", "Holding",		Holding   );
	Ini->GetValue( "PSG", "RNG",			RNG       );
#endif
	
	return true;
}


bool OPN64::DokoLoad( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->GetValue( "OPN", "RegisterLatch",	RegisterLatch );
	Ini->GetValue( "OPN", "LastEnable",		LastEnable    );
	
	return true;
}

