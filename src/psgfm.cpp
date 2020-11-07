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
	
	Ini->SetVal( "PSG", "RegisterLatch",	"", "0x%02X", RegisterLatch );
	Ini->SetVal( "PSG", "LastEnable",		"", "0x%02X", LastEnable    );

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->SetVal( "PSG", stren, "", reg[i], "0x%02X" );
	}
	
	Ini->SetVal( "PSG", "envelop", "", (envelop - enveloptable)/64 );
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->SetVal( "PSG", stren, "", olevel[i] );
		sprintf( stren, "scount_%02d", i );
		Ini->SetVal( "PSG", stren, "", scount[i], "0x%08X" );
		sprintf( stren, "speriod_%02d", i );
		Ini->SetVal( "PSG", stren, "", speriod[i], "0x%08X" );
	}
	Ini->SetVal( "PSG", "ecount",		"",	ecount,			"0x%08X" );
	Ini->SetVal( "PSG", "eperiod",		"",	eperiod,		"0x%08X" );
	Ini->SetVal( "PSG", "ncount",		"",	ncount,			"0x%08X" );
	Ini->SetVal( "PSG", "nperiod",		"",	nperiod,		"0x%08X" );
	
	Ini->SetVal( "PSG", "tperiodbase",	"",	tperiodbase,	"0x%08X" );
	Ini->SetVal( "PSG", "eperiodbase",	"",	eperiodbase,	"0x%08X" );
	Ini->SetVal( "PSG", "nperiodbase",	"",	nperiodbase,	"0x%08X" );
	
	Ini->SetVal( "PSG", "mask",			"", mask );
*/
#else
	for( int i=0; i<16; i++ )
		Ini->SetVal( "PSG", Stringf( "Regs_%02d", i ), "", "0x%02X", Regs[i] );
	
	Ini->SetVal( "PSG", "PeriodA",		"", PeriodA );
	Ini->SetVal( "PSG", "PeriodB",		"", PeriodB );
	Ini->SetVal( "PSG", "PeriodC",		"", PeriodC );
	Ini->SetVal( "PSG", "PeriodN",		"", PeriodN );
	Ini->SetVal( "PSG", "PeriodE",		"", PeriodE );
	Ini->SetVal( "PSG", "CountA",		"", CountA );
	Ini->SetVal( "PSG", "CountB",		"", CountB );
	Ini->SetVal( "PSG", "CountC",		"", CountC );
	Ini->SetVal( "PSG", "CountN",		"", CountN );
	Ini->SetVal( "PSG", "CountE",		"", CountE );
	Ini->SetVal( "PSG", "VolA",			"", VolA );
	Ini->SetVal( "PSG", "VolB",			"", VolB );
	Ini->SetVal( "PSG", "VolC",			"", VolC );
	Ini->SetVal( "PSG", "VolE",			"", VolE );
	Ini->SetVal( "PSG", "EnvelopeA",	"", "0x%02X", EnvelopeA );
	Ini->SetVal( "PSG", "EnvelopeB",	"", "0x%02X", EnvelopeB );
	Ini->SetVal( "PSG", "EnvelopeC",	"", "0x%02X", EnvelopeC );
	Ini->SetVal( "PSG", "OutputA",		"", "0x%02X", OutputA   );
	Ini->SetVal( "PSG", "OutputB",		"", "0x%02X", OutputB   );
	Ini->SetVal( "PSG", "OutputC",		"", "0x%02X", OutputC   );
	Ini->SetVal( "PSG", "OutputN",		"", "0x%02X", OutputN   );
	Ini->SetVal( "PSG", "CountEnv",		"", CountEnv );
	Ini->SetVal( "PSG", "Hold",			"", "0x%02X", Hold      );
	Ini->SetVal( "PSG", "Alternate",	"", "0x%02X", Alternate );
	Ini->SetVal( "PSG", "Attack",		"", "0x%02X", Attack    );
	Ini->SetVal( "PSG", "Holding",		"", "0x%02X", Holding   );
	Ini->SetVal( "PSG", "RNG",			"", RNG );
#endif
	
	return true;
}


bool OPN64::DokoSave( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->SetVal( "OPN", "RegisterLatch",	"", "0x%02X", RegisterLatch );
	Ini->SetVal( "OPN", "LastEnable",		"", "0x%02X", LastEnable    );
	
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
	
	Ini->GetVal( "PSG", "RegisterLatch",	RegisterLatch );
	Ini->GetVal( "PSG", "LastEnable",		LastEnable    );

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->GetVal( "PSG", stren, reg[i] );
	}
	
	st = (envelop - enveloptable)/64;
	Ini->GetVal( "PSG", "envelop",	st );
	envelop = enveloptable[st];
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->GetVal( "PSG", stren, olevel[i] );
		sprintf( stren, "scount_%02d", i );
		Ini->GetVal( "PSG", stren, scount[i] );
		sprintf( stren, "speriod_%02d", i );
		Ini->GetVal( "PSG", stren, speriod[i] );
	}
	
	Ini->GetVal( "PSG", "ecount",		ecount      );
	Ini->GetVal( "PSG", "eperiod",		eperiod     );
	Ini->GetVal( "PSG", "ncount",		ncount      );
	Ini->GetVal( "PSG", "nperiod",		nperiod     );
	Ini->GetVal( "PSG", "tperiodbase",	tperiodbase );
	Ini->GetVal( "PSG", "eperiodbase",	eperiodbase );
	Ini->GetVal( "PSG", "nperiodbase",	nperiodbase );
	
	Ini->GetVal( "PSG", "mask",		mask        );
*/
#else
	for( int i=0; i<16; i++ ){
		Ini->GetVal( "PSG", Stringf( "Regs_%02d", i ), Regs[i] );
	}
	Ini->GetVal( "PSG", "PeriodA",		PeriodA   );
	Ini->GetVal( "PSG", "PeriodB",		PeriodB   );
	Ini->GetVal( "PSG", "PeriodC",		PeriodC   );
	Ini->GetVal( "PSG", "PeriodN",		PeriodN   );
	Ini->GetVal( "PSG", "PeriodE",		PeriodE   );
	Ini->GetVal( "PSG", "CountA",		CountA    );
	Ini->GetVal( "PSG", "CountB",		CountB    );
	Ini->GetVal( "PSG", "CountC",		CountC    );
	Ini->GetVal( "PSG", "CountN",		CountN    );
	Ini->GetVal( "PSG", "CountE",		CountE    );
	Ini->GetVal( "PSG", "VolA",			VolA      );
	Ini->GetVal( "PSG", "VolB",			VolB      );
	Ini->GetVal( "PSG", "VolC",			VolC      );
	Ini->GetVal( "PSG", "VolE",			VolE      );
	Ini->GetVal( "PSG", "EnvelopeA",	EnvelopeA );
	Ini->GetVal( "PSG", "EnvelopeB",	EnvelopeB );
	Ini->GetVal( "PSG", "EnvelopeC",	EnvelopeC );
	Ini->GetVal( "PSG", "OutputA",		OutputA   );
	Ini->GetVal( "PSG", "OutputB",		OutputB   );
	Ini->GetVal( "PSG", "OutputC",		OutputC   );
	Ini->GetVal( "PSG", "OutputN",		OutputN   );
	Ini->GetVal( "PSG", "CountEnv",		CountEnv  );
	Ini->GetVal( "PSG", "Hold",			Hold      );
	Ini->GetVal( "PSG", "Alternate",	Alternate );
	Ini->GetVal( "PSG", "Attack",		Attack    );
	Ini->GetVal( "PSG", "Holding",		Holding   );
	Ini->GetVal( "PSG", "RNG",			RNG       );
#endif
	
	return true;
}


bool OPN64::DokoLoad( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->GetVal( "OPN", "RegisterLatch",	RegisterLatch );
	Ini->GetVal( "OPN", "LastEnable",		LastEnable    );
	
	return true;
}

