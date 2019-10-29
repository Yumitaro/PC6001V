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
PSGb::PSGb( VM6* vm, const ID& id ) : Device(vm,id), JoyNo(0), Clock(0) {}
PSG60::PSG60( VM6* vm, const ID& id ) : PSGb(vm,id)
{
	// Dvice Description (Out)
	descs.outdef.emplace( outA0H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA0H ) );
	descs.outdef.emplace( outA1H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA1H ) );
	descs.outdef.emplace( outA3H, STATIC_CAST( Device::OutFuncPtr, &PSG60::OutA3H ) );
	
	// Dvice Description (In)
	descs.indef.emplace ( inA2H,  STATIC_CAST( Device::InFuncPtr,  &PSG60::InA2H  ) );
}

OPN64::OPN64( VM6* vm, const ID& id ) : PSGb(vm,id)
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
PSGb::~PSGb( void ){}
PSG60::~PSG60( void ){}
OPN64::~OPN64( void ){}


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
	int samples = (int)( (double)SndDev::SampleRate * vm->EVSC::GetProgress( this->Device::GetID(), EID_PSG ) + 0.5 );
	vm->EVSC::Reset( this->Device::GetID(), EID_PSG );
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
	
	if( cnt ) vm->EVSC::Add( Device::GetID(), EID_TIMERA, ct, EV_LOOP|EV_US );
	else	  vm->EVSC::Del( Device::GetID(), EID_TIMERA );
}


////////////////////////////////////////////////////////////////
// TimerB設定
////////////////////////////////////////////////////////////////
void OPN64::SetTimerB( int cnt )
{
	double ct = 1152. * (256. - (double)cnt) / (double)Clock * 1000000.;
	
	if( cnt ) vm->EVSC::Add( Device::GetID(), EID_TIMERB, ct, EV_LOOP|EV_US );
	else	  vm->EVSC::Del( Device::GetID(), EID_TIMERB );
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
	if( !vm->EVSC::Add( Device::GetID(), EID_PSG, 1000, EV_LOOP|EV_MS ) ) return false;
	
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
	if( !vm->EVSC::Add( Device::GetID(), EID_PSG, 1000, EV_LOOP|EV_MS ) ) return false;
	
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
BYTE PSG60::PortAread( void ){ return vm->KEY6::GetJoy( JoyNo ); }
void PSG60::PortBwrite( BYTE data ){ JoyNo = (~data>>6)&1; }

BYTE OPN64::PortAread( void ){ return vm->KEY6::GetJoy( JoyNo ); }
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
	
	Ini->PutEntry( "PSG", "", "RegisterLatch",	"0x%02X",	RegisterLatch );
	Ini->PutEntry( "PSG", "", "LastEnable",		"0x%02X",	LastEnable );

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->PutEntry( "PSG", "", stren, "0x%02X", reg[i] );
	}
	
	Ini->PutEntry( "PSG", "", "envelop",		"%d",		(envelop - enveloptable)/64 );
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->PutEntry( "PSG", "", stren, "%d", olevel[i] );
		sprintf( stren, "scount_%02d", i );
		Ini->PutEntry( "PSG", "", stren, "0x%08X", scount[i] );
		sprintf( stren, "speriod_%02d", i );
		Ini->PutEntry( "PSG", "", stren, "0x%08X", speriod[i] );
	}
	Ini->PutEntry( "PSG", "", "ecount",			"0x%08X",	ecount );
	Ini->PutEntry( "PSG", "", "eperiod",		"0x%08X",	eperiod );
	Ini->PutEntry( "PSG", "", "ncount",			"0x%08X",	ncount );
	Ini->PutEntry( "PSG", "", "nperiod",		"0x%08X",	nperiod );
	
	Ini->PutEntry( "PSG", "", "tperiodbase",	"0x%08X",	tperiodbase );
	Ini->PutEntry( "PSG", "", "eperiodbase",	"0x%08X",	eperiodbase );
	Ini->PutEntry( "PSG", "", "nperiodbase",	"0x%08X",	nperiodbase );
	
	Ini->PutEntry( "PSG", "", "mask",			"%d",		mask );
*/
#else
	for( int i=0; i<16; i++ )
		Ini->PutEntry( "PSG", "", Stringf( "Regs_%02d", i ), "0x%02X", Regs[i] );
	
	Ini->PutEntry( "PSG", "", "PeriodA",	"%d",		PeriodA );
	Ini->PutEntry( "PSG", "", "PeriodB",	"%d",		PeriodB );
	Ini->PutEntry( "PSG", "", "PeriodC",	"%d",		PeriodC );
	Ini->PutEntry( "PSG", "", "PeriodN",	"%d",		PeriodN );
	Ini->PutEntry( "PSG", "", "PeriodE",	"%d",		PeriodE );
	Ini->PutEntry( "PSG", "", "CountA",		"%d",		CountA );
	Ini->PutEntry( "PSG", "", "CountB",		"%d",		CountB );
	Ini->PutEntry( "PSG", "", "CountC",		"%d",		CountC );
	Ini->PutEntry( "PSG", "", "CountN",		"%d",		CountN );
	Ini->PutEntry( "PSG", "", "CountE",		"%d",		CountE );
	Ini->PutEntry( "PSG", "", "VolA",		"%d",		VolA );
	Ini->PutEntry( "PSG", "", "VolB",		"%d",		VolB );
	Ini->PutEntry( "PSG", "", "VolC",		"%d",		VolC );
	Ini->PutEntry( "PSG", "", "VolE",		"%d",		VolE );
	Ini->PutEntry( "PSG", "", "EnvelopeA",	"0x%02X",	EnvelopeA );
	Ini->PutEntry( "PSG", "", "EnvelopeB",	"0x%02X",	EnvelopeB );
	Ini->PutEntry( "PSG", "", "EnvelopeC",	"0x%02X",	EnvelopeC );
	Ini->PutEntry( "PSG", "", "OutputA",	"0x%02X",	OutputA );
	Ini->PutEntry( "PSG", "", "OutputB",	"0x%02X",	OutputB );
	Ini->PutEntry( "PSG", "", "OutputC",	"0x%02X",	OutputC );
	Ini->PutEntry( "PSG", "", "OutputN",	"0x%02X",	OutputN );
	Ini->PutEntry( "PSG", "", "CountEnv",	"%d",		CountEnv );
	Ini->PutEntry( "PSG", "", "Hold",		"0x%02X",	Hold );
	Ini->PutEntry( "PSG", "", "Alternate",	"0x%02X",	Alternate );
	Ini->PutEntry( "PSG", "", "Attack",		"0x%02X",	Attack );
	Ini->PutEntry( "PSG", "", "Holding",	"0x%02X",	Holding );
	Ini->PutEntry( "PSG", "", "RNG",		"%d",		RNG );
#endif
	
	return true;
}


bool OPN64::DokoSave( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->PutEntry( "OPN", "", "RegisterLatch",	"0x%02X",	RegisterLatch );
	Ini->PutEntry( "OPN", "", "LastEnable",		"0x%02X",	LastEnable );
	
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
	int st;
	
	if( !Ini ) return false;
	
	Ini->GetInt( "PSG", "RegisterLatch",	&st,	RegisterLatch );	RegisterLatch = st;
	Ini->GetInt( "PSG", "LastEnable",		&st,	LastEnable );		LastEnable = st;

#ifdef USEFMGEN
/*
	for( int i=0; i<16; i++ ){
		char stren[16];
		sprintf( stren, "reg_%02d", i );
		Ini->GetInt( "PSG", stren, &st, reg[i] );	reg[i] = st;
	}
	
	Ini->GetInt( "PSG", "envelop",		&st,	(envelop - enveloptable)/64 );	envelop = enveloptable[st];
	
	for( int i=0; i<3; i++ ){
		char stren[16];
		sprintf( stren, "olevel_%02d", i );
		Ini->GetInt( "PSG", stren, &st, olevel[i] );	olevel[i] = st;
		sprintf( stren, "scount_%02d", i );
		Ini->GetInt( "PSG", stren, &st, scount[i] );	scount[i] = st;
		sprintf( stren, "speriod_%02d", i );
		Ini->GetInt( "PSG", stren, &st, speriod[i] );	speriod[i] = st;
	}
	
	Ini->GetInt( "PSG", "ecount",		&st,	ecount );		ecount  = st;
	Ini->GetInt( "PSG", "eperiod",		&st,	eperiod );		eperiod = st;
	Ini->GetInt( "PSG", "ncount",		&st,	ncount );		ncount  = st;
	Ini->GetInt( "PSG", "nperiod",		&st,	nperiod );		nperiod = st;
	Ini->GetInt( "PSG", "tperiodbase",	&st,	tperiodbase );	tperiodbase = st;
	Ini->GetInt( "PSG", "eperiodbase",	&st,	eperiodbase );	eperiodbase = st;
	Ini->GetInt( "PSG", "nperiodbase",	&st,	nperiodbase );	nperiodbase = st;
	
	Ini->GetInt( "PSG", "mask",			&mask,	mask );
*/
#else
	for( int i=0; i<16; i++ ){
		Ini->GetInt( "PSG", Stringf( "Regs_%02d", i ), &st, Regs[i] );	Regs[i] = st;
	}
	Ini->GetInt( "PSG", "PeriodA",		&PeriodA,		PeriodA );
	Ini->GetInt( "PSG", "PeriodB",		&PeriodB,		PeriodB );
	Ini->GetInt( "PSG", "PeriodC",		&PeriodC,		PeriodC );
	Ini->GetInt( "PSG", "PeriodN",		&PeriodN,		PeriodN );
	Ini->GetInt( "PSG", "PeriodE",		&PeriodE,		PeriodE );
	Ini->GetInt( "PSG", "CountA",		&CountA,		CountA );
	Ini->GetInt( "PSG", "CountB",		&CountB,		CountB );
	Ini->GetInt( "PSG", "CountC",		&CountC,		CountC );
	Ini->GetInt( "PSG", "CountN",		&CountN,		CountN );
	Ini->GetInt( "PSG", "CountE",		&CountE,		CountE );
	Ini->GetInt( "PSG", "VolA",			&VolA,			VolA );
	Ini->GetInt( "PSG", "VolB",			&VolB,			VolB );
	Ini->GetInt( "PSG", "VolC",			&VolC,			VolC );
	Ini->GetInt( "PSG", "VolE",			&VolE,			VolE );
	Ini->GetInt( "PSG", "EnvelopeA",	&st,			EnvelopeA );	EnvelopeA = st;
	Ini->GetInt( "PSG", "EnvelopeB",	&st,			EnvelopeB );	EnvelopeB = st;
	Ini->GetInt( "PSG", "EnvelopeC",	&st,			EnvelopeC );	EnvelopeC = st;
	Ini->GetInt( "PSG", "OutputA",		&st,			OutputA );		OutputA = st;
	Ini->GetInt( "PSG", "OutputB",		&st,			OutputB );		OutputB = st;
	Ini->GetInt( "PSG", "OutputC",		&st,			OutputC );		OutputC = st;
	Ini->GetInt( "PSG", "OutputN",		&st,			OutputN );		OutputN = st;
	Ini->GetInt( "PSG", "CountEnv",		&st,			CountEnv );		CountEnv = st;
	Ini->GetInt( "PSG", "Hold",			&st,			Hold );			Hold = st;
	Ini->GetInt( "PSG", "Alternate",	&st,			Alternate );	Alternate = st;
	Ini->GetInt( "PSG", "Attack",		&st,			Attack );		Attack = st;
	Ini->GetInt( "PSG", "Holding",		&st,			Holding );		Holding = st;
	Ini->GetInt( "PSG", "RNG",			&RNG,			RNG );
#endif
	
	return true;
}


bool OPN64::DokoLoad( cIni* Ini )
{
	int st;
	
	if( !Ini ) return false;
	
	Ini->GetInt( "OPN", "RegisterLatch",	&st,	RegisterLatch );	RegisterLatch = st;
	Ini->GetInt( "OPN", "LastEnable",		&st,	LastEnable );		LastEnable = st;
	
	return true;
}

