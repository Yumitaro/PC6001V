#include <new>

#include "pc6001v.h"

#include "breakpoint.h"
#include "config.h"
#include "cpum.h"
#include "cpus.h"
#include "debug.h"
#include "disk.h"
#include "error.h"
#include "graph.h"
#include "intr.h"
#include "io.h"
#include "keyboard.h"
#include "log.h"
#include "memory.h"
#include "pio.h"
#include "psgfm.h"
#include "schedule.h"
#include "sound.h"
#include "tape.h"
#include "voice.h"
#include "vdg.h"

#include "p6el.h"
#include "p6vm.h"




////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
VM6::VM6( EL6* emuobj ) :
	CPU6(this,DEV_ID("CPU1")),	// CPU
	PIO6(this,DEV_ID("8255")),	// 8255
	CMTL(this,DEV_ID("TAPE")),	// CMT(LOAD)
	CMTS(this,DEV_ID("SAVE")),	// CMT(SAVE)
	
	cclock(0), pclock(0), el(emuobj), iom(nullptr), ios(nullptr), intr(nullptr),
	mem(nullptr), vdg(nullptr), psg(nullptr), voice(nullptr), disk(nullptr)
{
}

VM60::VM60( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB60(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY60(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK60;
	pclock = PSG_CLOCK60;
	
	DevTable.Intr    = &VM60::c_intr;		// 割込み
	DevTable.Vdg     = &VM60::c_vdg;		// VDG
	DevTable.Psg     = &VM60::c_psg;		// PSG
	DevTable.M8255   = &VM60::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM60::c_8255s;		// I/O(SUB CPU側)
	DevTable.Disk    = &VM60::c_disk;		// DISK
	DevTable.CmtL    = &VM60::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM60::c_soldier;	// 戦士のカートリッジ
}

VM61::VM61( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB60(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY60(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK60;
	pclock = PSG_CLOCK60;
	
	DevTable.Intr    = &VM60::c_intr;		// 割込み
	DevTable.Vdg     = &VM60::c_vdg;		// VDG
	DevTable.Psg     = &VM60::c_psg;		// PSG
	DevTable.M8255   = &VM60::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM60::c_8255s;		// I/O(SUB CPU側)
	DevTable.Disk    = &VM60::c_disk;		// DISK
	DevTable.CmtL    = &VM60::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM60::c_soldier;	// 戦士のカートリッジ
}

VM62::VM62( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB62(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY62(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK60;
	pclock = PSG_CLOCK60;
	
	DevTable.Intr    = &VM62::c_intr;		// 割込み
	DevTable.Memory  = &VM62::c_memory;		// メモリ
	DevTable.Vdg     = &VM62::c_vdg;		// VDG
	DevTable.Psg     = &VM62::c_psg;		// PSG
	DevTable.M8255   = &VM62::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM62::c_8255s;		// I/O(SUB CPU側)
	DevTable.Voice   = &VM62::c_voice;		// 音声合成
	DevTable.Disk    = &VM62::c_disk;		// DISK
	DevTable.CmtL    = &VM62::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM6::c_soldier;		// 戦士のカートリッジ
}

VM66::VM66( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB62(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY62(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK66;
	pclock = PSG_CLOCK66;
	
	DevTable.Intr    = &VM62::c_intr;		// 割込み
	DevTable.Memory  = &VM62::c_memory;		// メモリ
	DevTable.Vdg     = &VM62::c_vdg;		// VDG
	DevTable.Psg     = &VM62::c_psg;		// PSG
	DevTable.M8255   = &VM62::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM62::c_8255s;		// I/O(SUB CPU側)
	DevTable.Voice   = &VM62::c_voice;		// 音声合成
	DevTable.Disk    = &VM66::c_disk;		// DISK
	DevTable.CmtL    = &VM62::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM6::c_soldier;		// 戦士のカートリッジ
}

VM64::VM64( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB62(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY62(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK64;
	pclock = PSG_CLOCK64;
	
	DevTable.Intr    = &VM64::c_intr;		// 割込み
	DevTable.Memory  = &VM64::c_memory;		// メモリ
	DevTable.Vdg     = &VM64::c_vdg;		// VDG
	DevTable.Psg     = &VM64::c_psg;		// PSG
	DevTable.M8255   = &VM64::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM64::c_8255s;		// I/O(SUB CPU側)
	DevTable.Voice   = &VM64::c_voice;		// 音声合成
	DevTable.Disk    = &VM64::c_disk;		// DISK
	DevTable.CmtL    = &VM64::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM6::c_soldier;		// 戦士のカートリッジ
}

VM68::VM68( EL6* emuobj ) : VM6(emuobj),
	SUB6 (this,DEV_ID("8049")),	// SUB CPU
	SUB68(this,DEV_ID("8049")),	// SUB CPU
	KEY6 (this,DEV_ID("KEYB")),	// キー
	KEY62(this,DEV_ID("KEYB"))	// キー
{
	cclock = CPUM_CLOCK64;
	pclock = PSG_CLOCK64;
	
	DevTable.Intr    = &VM64::c_intr;		// 割込み
	DevTable.Memory  = &VM64::c_memory;		// メモリ
	DevTable.Vdg     = &VM64::c_vdg;		// VDG
	DevTable.Psg     = &VM64::c_psg;		// PSG
	DevTable.M8255   = &VM64::c_8255m;		// I/O(Z80側)
	DevTable.S8255   = &VM64::c_8255s;		// I/O(SUB CPU側)
	DevTable.Voice   = &VM64::c_voice;		// 音声合成
	DevTable.Disk    = &VM68::c_disk;		// DISK
	DevTable.CmtL    = &VM64::c_cmtl;		// CMT(LOAD)
	DevTable.Soldier = &VM6::c_soldier;		// 戦士のカートリッジ
}



////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
VM6::~VM6( void )
{
	DeleteAllObject();
}

VM60::~VM60( void )
{
}

VM61::~VM61( void )
{
}

VM62::~VM62( void )
{
}

VM66::~VM66( void )
{
}

VM64::~VM64( void )
{
}

VM68::~VM68( void )
{
}




// =============================================================
// P6デバイス用関数群
// =============================================================


// EL ----------------------------------------------------------

////////////////////////////////////////////////////////////////
// モニタモード?
////////////////////////////////////////////////////////////////
#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
bool VM6::ElIsMonitor( void ) const
{
	return el->IsMonitor();
}
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


// IO6 ---------------------------------------------------------

////////////////////////////////////////////////////////////////
// IN関数
////////////////////////////////////////////////////////////////
BYTE VM6::IomIn( int port, int* wcnt )
{
	BYTE data = iom->In( port, wcnt );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// ブレークポイントチェック
	if( BPoint::CheckBP( BPoint::BP_IN, port&0xff ) ){
		PRINTD( IO_LOG, " -> Break!\n" );
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	return data;
}

BYTE VM6::IosIn( int port, int* wcnt )
{
	return ios->In( port, wcnt );
}


////////////////////////////////////////////////////////////////
// OUT関数
////////////////////////////////////////////////////////////////
void VM6::IomOut( int port, BYTE data, int* wcnt )
{
	iom->Out( port, data, wcnt );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// ブレークポイントチェック
	if( BPoint::CheckBP( BPoint::BP_OUT, port&0xff ) ){
		PRINTD( IO_LOG, " -> Break!\n" );
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
}

void VM6::IosOut( int port, BYTE data, int* wcnt )
{
	ios->Out( port, data, wcnt );
}


// IRQ6 --------------------------------------------------------

////////////////////////////////////////////////////////////////
// 割込みチェック
////////////////////////////////////////////////////////////////
int VM6::IntIntrCheck( void )
{
	return intr->IntrCheck();
}


////////////////////////////////////////////////////////////////
// 割込み要求
////////////////////////////////////////////////////////////////
void VM6::IntReqIntr( DWORD vec )
{
	intr->ReqIntr( vec );
}


////////////////////////////////////////////////////////////////
// 割込み撤回
////////////////////////////////////////////////////////////////
void VM6::IntCancelIntr( DWORD vec )
{
	intr->CancelIntr( vec );
}


////////////////////////////////////////////////////////////////
// タイマ割込みスイッチ取得
////////////////////////////////////////////////////////////////
bool VM6::IntGetTimerIntr( void )
{
	return intr->GetTimerIntr();
}


// SUB6 --------------------------------------------------------

////////////////////////////////////////////////////////////////
// CMT割込み発生可?
////////////////////////////////////////////////////////////////
bool VM6::IsCmtIntrReady( void )
{
	// BoostUp有効の場合はワークエリアもチェック
	return SUB6::IsCmtIntrReady() &&
			!( CMTL::IsBoostUp() && ( mem->Read( vdg->IsSRmode() ? 0xe6b8 : 0xfa19 ) & 2 ) );
}


// MEM6 --------------------------------------------------------

////////////////////////////////////////////////////////////////
// フェッチ(M1)
////////////////////////////////////////////////////////////////
BYTE VM6::MemFetch( WORD addr, int* m1wait )
{
	BYTE data = mem->Fetch( addr, m1wait );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// ブレークポイントチェック
	if( BPoint::CheckBP( BPoint::BP_READ, addr ) ){
		PRINTD( MEM_LOG, " -> Break!\n" );
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE VM6::MemRead( WORD addr, int* wcnt )
{
	BYTE data = mem->Read( addr, wcnt );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// ブレークポイントチェック
	if( BPoint::CheckBP( BPoint::BP_READ, addr ) ){
		PRINTD( MEM_LOG, " -> Break!\n" );
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void VM6::MemWrite( WORD addr, BYTE data, int* wcnt )
{
	mem->Write( addr, data, wcnt );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// ブレークポイントチェック
	if( BPoint::CheckBP( BPoint::BP_WRITE, addr ) ){
		PRINTD( MEM_LOG, " -> Break!\n" );
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
}


////////////////////////////////////////////////////////////////
// CG ROM BANK を切り替える
////////////////////////////////////////////////////////////////
void VM6::MemSetCGBank( bool data )
{
	mem->SetCGBank( data );
}


////////////////////////////////////////////////////////////////
// 直接読込み
////////////////////////////////////////////////////////////////
BYTE VM6::MemReadMainRom( WORD addr ) const { return mem->ReadMainRom( addr ); }
BYTE VM6::MemReadIntRam ( WORD addr ) const { return mem->ReadIntRam( addr ); }
BYTE VM6::MemReadExtRom ( WORD addr ) const { return mem->ReadExtRom( addr ); }
BYTE VM6::MemReadExtRam ( WORD addr ) const { return mem->ReadExtRam( addr ); }
BYTE VM6::MemReadCGrom1 ( WORD addr ) const { return mem->ReadCGrom1( addr ); }
BYTE VM6::MemReadCGrom2 ( WORD addr ) const { return mem->ReadCGrom2( addr ); }
BYTE VM6::MemReadCGrom3 ( WORD addr ) const { return mem->ReadCGrom3( addr ); }
#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
const std::string& VM6::MemGetReadMemBlk( int blk )  const { return mem->GetReadMemBlk( blk );  }
const std::string& VM6::MemGetWriteMemBlk( int blk ) const { return mem->GetWriteMemBlk( blk ); }
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


// VDG ---------------------------------------------------------

////////////////////////////////////////////////////////////////
// CRT表示状態取得
////////////////////////////////////////////////////////////////
bool VM6::VdgGetCrtDisp( void ) const
{
	return vdg->GetCrtDisp();
}


////////////////////////////////////////////////////////////////
// CRT表示状態設定
////////////////////////////////////////////////////////////////
void VM6::VdgSetCrtDisp( bool st )
{
	vdg->SetCrtDisp( st );
}


////////////////////////////////////////////////////////////////
// ウィンドウサイズ取得
////////////////////////////////////////////////////////////////
bool VM6::VdgGetWinSize( void ) const
{
	return vdg->GetWinSize();
}


////////////////////////////////////////////////////////////////
// バスリクエスト区間停止フラグ取得
////////////////////////////////////////////////////////////////
bool VM6::VdgIsBusReqStop( void ) const
{
	return vdg->IsBusReqStop();
}


////////////////////////////////////////////////////////////////
// バスリクエスト区間実行フラグ取得
////////////////////////////////////////////////////////////////
bool VM6::VdgIsBusReqExec( void ) const
{
	return vdg->IsBusReqExec();
}


////////////////////////////////////////////////////////////////
// VRAMアドレス取得
////////////////////////////////////////////////////////////////
WORD VM6::VdgGetVramAddr( void ) const
{
	return vdg->GetVramAddr();
}


////////////////////////////////////////////////////////////////
// ATTRアドレス取得
////////////////////////////////////////////////////////////////
WORD VM6::VdgGerAttrAddr( void ) const
{
	return vdg->GerAttrAddr();
}


////////////////////////////////////////////////////////////////
// SRモード取得
////////////////////////////////////////////////////////////////
bool VM6::VdgIsSRmode( void ) const
{
	return vdg->IsSRmode();
}


////////////////////////////////////////////////////////////////
// SRビットマップモード取得
////////////////////////////////////////////////////////////////
bool VM6::VdgIsSRBitmap( WORD addr ) const
{
	return vdg->IsSRBitmap( addr );
}


////////////////////////////////////////////////////////////////
// SRのG-VRAMアドレス取得 (ビットマップモード)
////////////////////////////////////////////////////////////////
WORD VM6::VdgSRGVramAddr( WORD addr ) const
{
	return vdg->SRGVramAddr( addr );
}


// DSK ---------------------------------------------------------

////////////////////////////////////////////////////////////////
// マウント済み?
////////////////////////////////////////////////////////////////
bool VM6::DskIsMount( int drvno ) const
{
	return disk->IsMount( drvno );
}


////////////////////////////////////////////////////////////////
// システムディスク?
////////////////////////////////////////////////////////////////
bool VM6::DskIsSystem( int drvno ) const
{
	return disk->IsSystem( drvno );
}


////////////////////////////////////////////////////////////////
// プロテクト?
////////////////////////////////////////////////////////////////
bool VM6::DskIsProtect( int drvno ) const
{
	return disk->IsProtect( drvno );
}


////////////////////////////////////////////////////////////////
// アクセス中?
////////////////////////////////////////////////////////////////
bool VM6::DskInAccess( int drvno ) const
{
	return disk->InAccess( drvno );
}


////////////////////////////////////////////////////////////////
// ファイルパス取得
////////////////////////////////////////////////////////////////
const std::filesystem::path& VM6::DskGetFile( int drvno ) const
{
	return disk->GetFile( drvno );
}


////////////////////////////////////////////////////////////////
// DISK名取得
////////////////////////////////////////////////////////////////
const std::string& VM6::DskGetName( int drvno ) const
{
	return disk->GetName( drvno );
}


// =============================================================




////////////////////////////////////////////////////////////////
// 機種別オブジェクト確保
////////////////////////////////////////////////////////////////
void VM60::AllocObjSpecific( void )
{
	intr   = new IRQ60( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM60( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG60( this, DEV_ID("VDG1") );		// VDG
	psg    = new PSG60( this, DEV_ID("PSG1") );		// PSG
	voice  = new VCE60( this, DEV_ID("VCE1") );		// 音声合成
	disk   = new DSK60( this, DEV_ID("DSK1") );		// DISK
}

void VM61::AllocObjSpecific( void )
{
	intr   = new IRQ60( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM61( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG60( this, DEV_ID("VDG1") );		// VDG
	psg    = new PSG60( this, DEV_ID("PSG1") );		// PSG
	voice  = new VCE60( this, DEV_ID("VCE1") );		// 音声合成
	disk   = new DSK60( this, DEV_ID("DSK1") );		// DISK
}

void VM62::AllocObjSpecific( void )
{
	intr   = new IRQ62( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM62( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG62( this, DEV_ID("VDG2") );		// VDG
	psg    = new PSG60( this, DEV_ID("PSG1") );		// PSG
	voice  = new VCE62( this, DEV_ID("VCE2") );		// 音声合成
	disk   = new DSK60( this, DEV_ID("DSK1") );		// DISK
}

void VM66::AllocObjSpecific( void )
{
	intr   = new IRQ62( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM66( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG62( this, DEV_ID("VDG2") );		// VDG
	psg    = new PSG60( this, DEV_ID("PSG1") );		// PSG
	voice  = new VCE62( this, DEV_ID("VCE2") );		// 音声合成
	disk   = new DSK66( this, DEV_ID("DSK3") );		// DISK
}

void VM64::AllocObjSpecific( void )
{
	intr   = new IRQ64( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM64( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG64( this, DEV_ID("VDG3") );		// VDG
	psg    = new OPN64( this, DEV_ID("OPN1") );		// OPN
	voice  = new VCE64( this, DEV_ID("VCE3") );		// 音声合成
	disk   = new DSK64( this, DEV_ID("DSK2") );		// DISK
}

void VM68::AllocObjSpecific( void )
{
	intr   = new IRQ64( this, DEV_ID("INTR") );		// 割込み
	mem    = new MEM68( this, DEV_ID("MEM1") );		// メモリ
	vdg    = new VDG64( this, DEV_ID("VDG3") );		// VDG
	psg    = new OPN64( this, DEV_ID("OPN1") );		// OPN
	voice  = new VCE64( this, DEV_ID("VCE3") );		// 音声合成
	disk   = new DSK68( this, DEV_ID("DSK4") );		// DISK
}


////////////////////////////////////////////////////////////////
// 全オブジェクト確保
////////////////////////////////////////////////////////////////
bool VM6::AllocObject( CFG6* cnfg )
{
	PRINTD( VM_LOG, "[VM][AllocObject]\n" );
	
	try{
		// 各種オブジェクト確保
		iom = new IO6;		// I/O(Z80)
		ios = new IO6;		// I/O(SUB CPU)
		
		// 機種別オブジェクト確保
		AllocObjSpecific();
		
		
		// 全メモリ確保とROMファイル読込み
		BYTE flg = (cnfg->GetCheckCRC()   ? MCRCCHK   : 0)
				 | (cnfg->GetUseExtRam()  ? MUSEEXRAM : 0)
				 | (cnfg->GetUseSoldier() );
		
		if( !mem->AllocAllMemory( cnfg->GetRomPath(), flg ) ) throw Error::GetError();
		
	}
	catch( std::bad_alloc ){	// new に失敗した場合
		// 全オブジェクト削除
		DeleteAllObject();
		Error::SetError( Error::MemAllocFailed );
		return false;
	}
	catch( Error::Errno i ){	// 例外発生
		// 全オブジェクト削除
		DeleteAllObject();
		return false;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// 全オブジェクト削除
////////////////////////////////////////////////////////////////
void VM6::DeleteAllObject( void )
{
	if( voice ){ delete voice;	voice = nullptr; }
	if( disk ) { delete disk;	disk  = nullptr; }
	if( intr ) { delete intr;	intr  = nullptr; }
	if( vdg )  { delete vdg;	vdg   = nullptr; }
	if( mem )  { delete mem;	mem   = nullptr; }
	if( psg )  { delete psg;	psg   = nullptr; }
	if( ios )  { delete ios;	ios   = nullptr; }
	if( iom )  { delete iom;	iom   = nullptr; }
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool VM6::Init( CFG6* cnfg  )
{
	// 全オブジェクト確保
	if( !AllocObject( cnfg ) ) return false;
	
	// イベントスケジューラ
	EVSC::SetMasterClock( cclock * cnfg->GetOverClock() / 100 );
//	std::vector<Device*> devs = { intr, cpum, cpus, vdg, psg, voice, pio, key, cmtl, cmts, disk };
	std::vector<Device*> devs = { intr, (CPU6*)this, (SUB6*)this, vdg, psg, voice, (PIO6*)this, (KEY6*)this, (CMTL*)this, (CMTS*)this, disk };
	EVSC::Entry( devs );
	
	// I/O(Z80)　-----
	if( !iom->Init( 256 ) ) return false;
	
	// I/O(SUB CPU)　-----
	if( !ios->Init( 10 ) ) return false;
	
	// 割込み -----
	intr->Reset();
	
	// CPU -----
	CPU6::Reset();
	
	// SUB CPU -----
	SUB6::Reset();
	
	// メモリ -----
	if( !mem->Init() ) return false;
	mem->Reset();
	if( !cnfg->GetExtRomFile().empty() ) if( !mem->MountExtRom( cnfg->GetExtRomFile() ) ) return false;
	
	// VDG -----
	if( !vdg->Init() ) return false;
	vdg->SetMode4Color( cnfg->GetMode4Color() );
	
	// PSG/OPN -----
	psg->SetVolume( cnfg->GetPsgVol() );		// 音量設定
	psg->SetLPF( cnfg->GetPsgLPF() );			// ローパスフィルタ カットオフ周波数設定
	for( auto &i : *DevTable.Psg ){				// ウェイト設定
		if( i.rule == IOBus::portout ) iom->SetOutWait( i.bank, 1 );
		else						   iom->SetInWait ( i.bank, 1 );
	}
	if( !psg->Init( pclock, cnfg->GetSampleRate() ) ) return false;
	
	// 8255 -----
	PIO6::Reset();
	PIO6::cPRT::SetFile( cnfg->GetPrinterFile() );
	
	// キー -----
	if( !KEY6::Init( cnfg->GetKeyRepeat() ) ) return false;
	std::vector<VKeyConv> vk;
	if( cnfg->GetVKeyDef( vk ) )				// キー定義配列取得
		KEY6::SetVKeySymbols( vk );				// 仮想キーコード -> P6キーコード 設定
	
	// CMT(LOAD) -----
	if( !CMTL::Init( cnfg->GetSampleRate() ) ) return false;
	CMTL::SetVolume( cnfg->GetCmtVol() );		// 音量設定
	CMTL::SetLPF( cnfg->GetCmtLPF() );			// ローパスフィルタ カットオフ周波数設定
	CMTL::SetBoost( cnfg->GetBoostUp() );
	CMTL::SetMaxBoost( cnfg->GetMaxBoost1(), cnfg->GetMaxBoost2() );
	CMTL::SetStopBit( cnfg->GetStopBit() );		// ストップビット数
	
	// CMT(SAVE) -----
	if( !CMTS::Init( cnfg->GetSaveFile() ) ) return false;
	
	// DISK -----
	if( !disk->Init( cnfg->GetFddNum() ) ) return false;
	disk->WaitEnable( cnfg->GetFddWaitEnable() );
	
	// 音声合成 -----
	if( DevTable.Voice ){
		if( !voice->Init( cnfg->GetSampleRate() ) ) return false;
		voice->SetVolume( cnfg->GetVoiceVol() );	// 音量設定
		voice->SetPath( cnfg->GetWavePath() );
	}
	
	
	// I/Oポートにデバイスを接続
	if( !iom->Connect( (PIO6*)this, DevTable.M8255 ) ) return false;	// 8255(Z80側)
	if( !ios->Connect( (PIO6*)this, DevTable.S8255 ) ) return false;	// 8255(SUB CPU側)
	if( !iom->Connect( (CMTL*)this, DevTable.CmtL  ) ) return false;	// CMT(LOAD)
	if( !iom->Connect( intr,        DevTable.Intr  ) ) return false;	// 割込み
	if( !iom->Connect( vdg,         DevTable.Vdg   ) ) return false;	// VDG
	if( !iom->Connect( psg,         DevTable.Psg   ) ) return false;	// PSG/OPN
	if( cnfg->GetFddNum() || (cnfg->GetModel() == 66) || (cnfg->GetModel() == 68) )	// DISK
		if( !iom->Connect( disk,  DevTable.Disk   ) ) return false;
	if( DevTable.Memory )										// メモリ
		if( !iom->Connect( mem,   DevTable.Memory ) ) return false;
	if( DevTable.Voice )										// 音声合成
		if( !iom->Connect( voice, DevTable.Voice  ) ) return false;
	
	// オプション機能 -----
	if( cnfg->GetUseSoldier() )									// 戦士のカートリッジ
		if( !iom->Connect( mem, DevTable.Soldier ) ) return false;
	
	
	return true;
}


////////////////////////////////////////////////////////////////
// リセット
////////////////////////////////////////////////////////////////
void VM6::Reset( void )
{
	PRINTD( VM_LOG, "[VM][Reset]\n" );
	
	intr->Reset();
	CPU6::Reset();
	SUB6::Reset();
	mem->Reset();
	vdg->Reset();
	psg->Reset();
	PIO6::Reset();
	CMTL::Rewind();
	disk->Reset();
	voice->Reset();
}


////////////////////////////////////////////////////////////////
// 1命令実行
////////////////////////////////////////////////////////////////
int VM6::Emu( void )
{
	PRINTD( VM_LOG, "[VM][Emu]\n" );
	
	return CPU6::Exec();	// CPU 1命令実行
}


////////////////////////////////////////////////////////////////
// CPUクロック取得
////////////////////////////////////////////////////////////////
int VM6::GetCPUClock( void ) const
{
	PRINTD( VM_LOG, "[VM][GetCPUClock]\n" );
	
	return cclock;
}








////////////////////////////////////////////////////////////////
// デバイスコネクタ
////////////////////////////////////////////////////////////////

// 戦士のカートリッジ
const std::vector<IOBus::Connector> VM6::c_soldier = {
	{ 0x06, IOBus::portout, MEM6::out06H },
	{ 0x7f, IOBus::portout, MEM6::out7FH },
	{ 0x30, IOBus::portout, MEM6::out3xH },
	{ 0x31, IOBus::portout, MEM6::out3xH },
	{ 0x32, IOBus::portout, MEM6::out3xH },
	{ 0x33, IOBus::portout, MEM6::out3xH },
	{ 0x34, IOBus::portout, MEM6::out3xH },
	{ 0x35, IOBus::portout, MEM6::out3xH },
	{ 0x36, IOBus::portout, MEM6::out3xH },
	{ 0x37, IOBus::portout, MEM6::out3xH },
	{ 0x38, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x39, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3a, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3b, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3c, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3d, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3e, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3f, IOBus::portout, MEM6::out3xH }	// イメージ
};

const std::vector<IOBus::Connector> VM60::c_soldier = {
	{ 0x06, IOBus::portout, MEM6::out06H },
	{ 0x7f, IOBus::portout, MEM6::out7FH },
	{ 0x30, IOBus::portout, MEM6::out3xH },
	{ 0x31, IOBus::portout, MEM6::out3xH },
	{ 0x32, IOBus::portout, MEM6::out3xH },
	{ 0x33, IOBus::portout, MEM6::out3xH },
	{ 0x34, IOBus::portout, MEM6::out3xH },
	{ 0x35, IOBus::portout, MEM6::out3xH },
	{ 0x36, IOBus::portout, MEM6::out3xH },
	{ 0x37, IOBus::portout, MEM6::out3xH },
	{ 0x38, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x39, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3a, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3b, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3c, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3d, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3e, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0x3f, IOBus::portout, MEM6::out3xH },	// イメージ
	{ 0xf0, IOBus::portout, MEM60::outF0H },
	{ 0xf2, IOBus::portout, MEM60::outF2H }
};


// PC-6001,PC-6001A --------------------------------------------

// 割込み -----
const std::vector<IOBus::Connector> VM60::c_intr = {
	{ 0xb0, IOBus::portout, IRQ60::outB0H },
	{ 0xb1, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb2, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb3, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb4, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb5, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb6, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb7, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb8, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xb9, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xba, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xbb, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xbc, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xbd, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xbe, IOBus::portout, IRQ60::outB0H },	// イメージ
	{ 0xbf, IOBus::portout, IRQ60::outB0H }		// イメージ
};

// VDG -----
const std::vector<IOBus::Connector> VM60::c_vdg = {
	{ 0xb0, IOBus::portout, VDG60::outB0H },
	{ 0xb1, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb2, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb3, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb4, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb5, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb6, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb7, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb8, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xb9, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xba, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xbb, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xbc, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xbd, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xbe, IOBus::portout, VDG60::outB0H },	// イメージ
	{ 0xbf, IOBus::portout, VDG60::outB0H }		// イメージ
};

// PSG -----
const std::vector<IOBus::Connector> VM60::c_psg = {
	{ 0xa0, IOBus::portout, PSG60::outA0H },
	{ 0xa1, IOBus::portout, PSG60::outA1H },
	{ 0xa3, IOBus::portout, PSG60::outA3H },
	{ 0xa4, IOBus::portout, PSG60::outA0H },	// イメージ
	{ 0xa5, IOBus::portout, PSG60::outA1H },	// イメージ
	{ 0xa7, IOBus::portout, PSG60::outA3H },	// イメージ
	{ 0xa8, IOBus::portout, PSG60::outA0H },	// イメージ
	{ 0xa9, IOBus::portout, PSG60::outA1H },	// イメージ
	{ 0xab, IOBus::portout, PSG60::outA3H },	// イメージ
	{ 0xac, IOBus::portout, PSG60::outA0H },	// イメージ
	{ 0xad, IOBus::portout, PSG60::outA1H },	// イメージ
	{ 0xaf, IOBus::portout, PSG60::outA3H },	// イメージ
	{ 0xa2, IOBus::portin,  PSG60::inA2H },
	{ 0xa6, IOBus::portin,  PSG60::inA2H },		// イメージ
	{ 0xaa, IOBus::portin,  PSG60::inA2H },		// イメージ
	{ 0xae, IOBus::portin,  PSG60::inA2H }		// イメージ
};

// 8255(SUB CPU側) -----
const std::vector<IOBus::Connector> VM60::c_8255s = {
	{ IO8049_BUS, IOBus::portout, PIO6::outPBH },
	{ IO8049_BUS, IOBus::portin,  PIO6::inPBH  },
	{ IO8049_T0,  IOBus::portin,  PIO6::inIBF  },
	{ IO8049_INT, IOBus::portin,  PIO6::inOBF  }
};

// 8255(Z80側) -----
const std::vector<IOBus::Connector> VM60::c_8255m = {
	{ 0x90, IOBus::portout, PIO6::out90H },
	{ 0x91, IOBus::portout, PIO6::out91H },
	{ 0x92, IOBus::portout, PIO6::out92H },
	{ 0x93, IOBus::portout, PIO6::out93H },
	{ 0x94, IOBus::portout, PIO6::out90H },	// イメージ
	{ 0x95, IOBus::portout, PIO6::out91H },	// イメージ
	{ 0x96, IOBus::portout, PIO6::out92H },	// イメージ
	{ 0x97, IOBus::portout, PIO6::out93H },	// イメージ
	{ 0x98, IOBus::portout, PIO6::out90H },	// イメージ
	{ 0x99, IOBus::portout, PIO6::out91H },	// イメージ
	{ 0x9a, IOBus::portout, PIO6::out92H },	// イメージ
	{ 0x9b, IOBus::portout, PIO6::out93H },	// イメージ
	{ 0x9c, IOBus::portout, PIO6::out90H },	// イメージ
	{ 0x9d, IOBus::portout, PIO6::out91H },	// イメージ
	{ 0x9e, IOBus::portout, PIO6::out92H },	// イメージ
	{ 0x9f, IOBus::portout, PIO6::out93H },	// イメージ
	{ 0x90, IOBus::portin,  PIO6::in90H },
	{ 0x92, IOBus::portin,  PIO6::in92H },
	{ 0x93, IOBus::portin,  PIO6::in93H },
	{ 0x94, IOBus::portin,  PIO6::in90H },	// イメージ
	{ 0x96, IOBus::portin,  PIO6::in92H },	// イメージ
	{ 0x97, IOBus::portin,  PIO6::in93H },	// イメージ
	{ 0x98, IOBus::portin,  PIO6::in90H },	// イメージ
	{ 0x9a, IOBus::portin,  PIO6::in92H },	// イメージ
	{ 0x9b, IOBus::portin,  PIO6::in93H },	// イメージ
	{ 0x9c, IOBus::portin,  PIO6::in90H },	// イメージ
	{ 0x9e, IOBus::portin,  PIO6::in92H },	// イメージ
	{ 0x9f, IOBus::portin,  PIO6::in93H }	// イメージ
};

// DISK -----
const std::vector<IOBus::Connector> VM60::c_disk = {
	{ 0xd1, IOBus::portout, DSK60::outD1H },
	{ 0xd2, IOBus::portout, DSK60::outD2H },
	{ 0xd3, IOBus::portout, DSK60::outD3H },
	{ 0xd5, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xd6, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xd7, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xd9, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xda, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xdb, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xdd, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xde, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xdf, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xd0, IOBus::portin,  DSK60::inD0H },
	{ 0xd1, IOBus::portin,  DSK60::inD1H },
	{ 0xd2, IOBus::portin,  DSK60::inD2H },
	{ 0xd3, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xd4, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xd5, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xd6, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xd7, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xd8, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xd9, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xda, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xdb, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xdc, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xdd, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xde, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xdf, IOBus::portin,  DSK60::inD2H }		// イメージ?
};

// CMT(LOAD) -----
const std::vector<IOBus::Connector> VM60::c_cmtl = {
	{ 0xb0, IOBus::portout, CMTL::outB0H },
	{ 0xb1, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb2, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb3, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb4, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb5, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb6, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb7, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb8, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xb9, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xba, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xbb, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xbc, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xbd, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xbe, IOBus::portout, CMTL::outB0H },	// イメージ
	{ 0xbf, IOBus::portout, CMTL::outB0H }	// イメージ
};




// PC-6001mk2 --------------------------------------------------

// 割込み -----
const std::vector<IOBus::Connector> VM62::c_intr = {
	{ 0xb0, IOBus::portout, IRQ62::outB0H },
	{ 0xf3, IOBus::portout, IRQ62::outF3H },
	{ 0xf4, IOBus::portout, IRQ62::outF4H },
	{ 0xf5, IOBus::portout, IRQ62::outF5H },
	{ 0xf6, IOBus::portout, IRQ62::outF6H },
	{ 0xf7, IOBus::portout, IRQ62::outF7H },
	{ 0xf3, IOBus::portin,  IRQ62::inF3H },
	{ 0xf4, IOBus::portin,  IRQ62::inF4H },
	{ 0xf5, IOBus::portin,  IRQ62::inF5H },
	{ 0xf6, IOBus::portin,  IRQ62::inF6H },
	{ 0xf7, IOBus::portin,  IRQ62::inF7H }
};

// メモリ -----
const std::vector<IOBus::Connector> VM62::c_memory = {
	{ 0xc1, IOBus::portout, MEM62::outC1H },
	{ 0xc2, IOBus::portout, MEM62::outC2H },
	{ 0xc3, IOBus::portout, MEM62::outC3H },
	{ 0xf0, IOBus::portout, MEM62::outF0H },
	{ 0xf1, IOBus::portout, MEM62::outF1H },
	{ 0xf2, IOBus::portout, MEM62::outF2H },
	{ 0xf3, IOBus::portout, MEM62::outF3H },
	{ 0xf8, IOBus::portout, MEM62::outF8H },
	{ 0xc2, IOBus::portin,  MEM62::inC2H },
	{ 0xf0, IOBus::portin,  MEM62::inF0H },
	{ 0xf1, IOBus::portin,  MEM62::inF1H },
	{ 0xf2, IOBus::portin,  MEM62::inF2H },
	{ 0xf3, IOBus::portin,  MEM62::inF3H }
};

// VDG -----
const std::vector<IOBus::Connector> VM62::c_vdg = {
	{ 0xb0, IOBus::portout, VDG62::outB0H },
	{ 0xc0, IOBus::portout, VDG62::outC0H },
	{ 0xc1, IOBus::portout, VDG62::outC1H },
	{ 0xa2, IOBus::portin,  VDG62::inA2H }
};

// PSG -----
const std::vector<IOBus::Connector> VM62::c_psg = {
	{ 0xa0, IOBus::portout, PSG60::outA0H },
	{ 0xa1, IOBus::portout, PSG60::outA1H },
	{ 0xa3, IOBus::portout, PSG60::outA3H },
	{ 0xa2, IOBus::portin,  PSG60::inA2H }
};

// 8255(Z80側) -----
const std::vector<IOBus::Connector> VM62::c_8255m = {
	{ 0x90, IOBus::portout, PIO6::out90H },
	{ 0x91, IOBus::portout, PIO6::out91H },
	{ 0x92, IOBus::portout, PIO6::out92H },
	{ 0x93, IOBus::portout, PIO6::out93H },
	{ 0x90, IOBus::portin,  PIO6::in90H },
	{ 0x92, IOBus::portin,  PIO6::in92H },
	{ 0x93, IOBus::portin,  PIO6::in93H }
};

// 8255(SUB CPU側) -----
const std::vector<IOBus::Connector> VM62::c_8255s = {
	{ IO8049_BUS, IOBus::portout, PIO6::outPBH },
	{ IO8049_BUS, IOBus::portin,  PIO6::inPBH  },
	{ IO8049_T0,  IOBus::portin,  PIO6::inIBF  },
	{ IO8049_INT, IOBus::portin,  PIO6::inOBF  }
};

// 音声合成 -----
const std::vector<IOBus::Connector> VM62::c_voice = {
	{ 0xe0, IOBus::portout, VCE62::outE0H },
	{ 0xe2, IOBus::portout, VCE62::outE2H },
	{ 0xe3, IOBus::portout, VCE62::outE3H },
	{ 0xe4, IOBus::portout, VCE62::outE0H },	// イメージ
	{ 0xe6, IOBus::portout, VCE62::outE2H },	// イメージ
	{ 0xe7, IOBus::portout, VCE62::outE3H },	// イメージ
	{ 0xe8, IOBus::portout, VCE62::outE0H },	// イメージ
	{ 0xea, IOBus::portout, VCE62::outE2H },	// イメージ
	{ 0xeb, IOBus::portout, VCE62::outE3H },	// イメージ
	{ 0xec, IOBus::portout, VCE62::outE0H },	// イメージ
	{ 0xee, IOBus::portout, VCE62::outE2H },	// イメージ
	{ 0xef, IOBus::portout, VCE62::outE3H },	// イメージ
	{ 0xe0, IOBus::portin,  VCE62::inE0H },
	{ 0xe2, IOBus::portin,  VCE62::inE2H },
	{ 0xe3, IOBus::portin,  VCE62::inE3H },
	{ 0xe4, IOBus::portin,  VCE62::inE0H },		// イメージ
	{ 0xe6, IOBus::portin,  VCE62::inE2H },		// イメージ
	{ 0xe7, IOBus::portin,  VCE62::inE3H },		// イメージ
	{ 0xe8, IOBus::portin,  VCE62::inE0H },		// イメージ
	{ 0xea, IOBus::portin,  VCE62::inE2H },		// イメージ
	{ 0xeb, IOBus::portin,  VCE62::inE3H },		// イメージ
	{ 0xec, IOBus::portin,  VCE62::inE0H },		// イメージ
	{ 0xee, IOBus::portin,  VCE62::inE2H },		// イメージ
	{ 0xef, IOBus::portin,  VCE62::inE3H }		// イメージ
};

// DISK -----
const std::vector<IOBus::Connector> VM62::c_disk = {
	{ 0xd1, IOBus::portout, DSK60::outD1H },
	{ 0xd2, IOBus::portout, DSK60::outD2H },
	{ 0xd3, IOBus::portout, DSK60::outD3H },
	{ 0xd5, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xd6, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xd7, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xd9, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xda, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xdb, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xdd, IOBus::portout, DSK60::outD1H },	// イメージ
	{ 0xde, IOBus::portout, DSK60::outD2H },	// イメージ
	{ 0xdf, IOBus::portout, DSK60::outD3H },	// イメージ
	{ 0xd0, IOBus::portin,  DSK60::inD0H },
	{ 0xd1, IOBus::portin,  DSK60::inD1H },
	{ 0xd2, IOBus::portin,  DSK60::inD2H },
	{ 0xd3, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xd4, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xd5, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xd6, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xd7, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xd8, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xd9, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xda, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xdb, IOBus::portin,  DSK60::inD2H },		// イメージ?
	{ 0xdc, IOBus::portin,  DSK60::inD0H },		// イメージ
	{ 0xdd, IOBus::portin,  DSK60::inD1H },		// イメージ
	{ 0xde, IOBus::portin,  DSK60::inD2H },		// イメージ
	{ 0xdf, IOBus::portin,  DSK60::inD2H }		// イメージ?
};

// CMT(LOAD) -----
const std::vector<IOBus::Connector> VM62::c_cmtl = {
	{ 0xb0, IOBus::portout, CMTL::outB0H }
};



// PC-6601 -----------------------------------------------------

// DISK -----
const std::vector<IOBus::Connector> VM66::c_disk = {
	{ 0xb1, IOBus::portout, DSK66::outB1H },
	{ 0xb3, IOBus::portout, DSK66::outB3H },
	{ 0xd0, IOBus::portout, DSK66::outD0H },
	{ 0xd1, IOBus::portout, DSK66::outD1H },
	{ 0xd2, IOBus::portout, DSK66::outD2H },
	{ 0xd3, IOBus::portout, DSK66::outD3H },
	{ 0xd6, IOBus::portout, DSK66::outD6H },
	{ 0xd8, IOBus::portout, DSK66::outD8H },
	{ 0xda, IOBus::portout, DSK66::outDAH },
	{ 0xdd, IOBus::portout, DSK66::outDDH },
	{ 0xde, IOBus::portout, DSK66::outDEH },
	{ 0xb2, IOBus::portin,  DSK66::inB2H },
	{ 0xd0, IOBus::portin,  DSK66::inD0H },
	{ 0xd1, IOBus::portin,  DSK66::inD1H },
	{ 0xd2, IOBus::portin,  DSK66::inD2H },
	{ 0xd3, IOBus::portin,  DSK66::inD3H },
	{ 0xd4, IOBus::portin,  DSK66::inD4H },
	{ 0xdc, IOBus::portin,  DSK66::inDCH },
	{ 0xdd, IOBus::portin,  DSK66::inDDH }
};



// PC-6001mk2SR ------------------------------------------------

// 割込み -----
const std::vector<IOBus::Connector> VM64::c_intr = {
	{ 0xb0, IOBus::portout, IRQ64::outB0H },
	{ 0xb8, IOBus::portout, IRQ64::outBxH },
	{ 0xb9, IOBus::portout, IRQ64::outBxH },
	{ 0xba, IOBus::portout, IRQ64::outBxH },
	{ 0xbb, IOBus::portout, IRQ64::outBxH },
	{ 0xbc, IOBus::portout, IRQ64::outBxH },
	{ 0xbd, IOBus::portout, IRQ64::outBxH },
	{ 0xbe, IOBus::portout, IRQ64::outBxH },
	{ 0xbf, IOBus::portout, IRQ64::outBxH },
	{ 0xf3, IOBus::portout, IRQ64::outF3H },
	{ 0xf4, IOBus::portout, IRQ64::outF4H },
	{ 0xf5, IOBus::portout, IRQ64::outF5H },
	{ 0xf6, IOBus::portout, IRQ64::outF6H },
	{ 0xf7, IOBus::portout, IRQ64::outF7H },
	{ 0xfa, IOBus::portout, IRQ64::outFAH },
	{ 0xfb, IOBus::portout, IRQ64::outFBH },
	{ 0xf3, IOBus::portin,  IRQ64::inF3H },
	{ 0xf4, IOBus::portin,  IRQ64::inF4H },
	{ 0xf5, IOBus::portin,  IRQ64::inF5H },
	{ 0xf6, IOBus::portin,  IRQ64::inF6H },
	{ 0xf7, IOBus::portin,  IRQ64::inF7H },
	{ 0xfa, IOBus::portin,  IRQ64::inFAH },
	{ 0xfb, IOBus::portin,  IRQ64::inFBH }
};

// メモリ -----
const std::vector<IOBus::Connector> VM64::c_memory = {
	{ 0x60, IOBus::portout, MEM64::out6xH },
	{ 0x61, IOBus::portout, MEM64::out6xH },
	{ 0x62, IOBus::portout, MEM64::out6xH },
	{ 0x63, IOBus::portout, MEM64::out6xH },
	{ 0x64, IOBus::portout, MEM64::out6xH },
	{ 0x65, IOBus::portout, MEM64::out6xH },
	{ 0x66, IOBus::portout, MEM64::out6xH },
	{ 0x67, IOBus::portout, MEM64::out6xH },
	{ 0x68, IOBus::portout, MEM64::out6xH },
	{ 0x69, IOBus::portout, MEM64::out6xH },
	{ 0x6a, IOBus::portout, MEM64::out6xH },
	{ 0x6b, IOBus::portout, MEM64::out6xH },
	{ 0x6c, IOBus::portout, MEM64::out6xH },
	{ 0x6d, IOBus::portout, MEM64::out6xH },
	{ 0x6e, IOBus::portout, MEM64::out6xH },
	{ 0x6f, IOBus::portout, MEM64::out6xH },
	
	{ 0xc1, IOBus::portout, MEM64::outC1H },
	{ 0xc2, IOBus::portout, MEM64::outC2H },
	{ 0xc3, IOBus::portout, MEM64::outC3H },
	
	{ 0xf0, IOBus::portout, MEM64::outF0H },
	{ 0xf1, IOBus::portout, MEM64::outF1H },
	{ 0xf2, IOBus::portout, MEM64::outF2H },
	{ 0xf3, IOBus::portout, MEM64::outF3H },
	{ 0xf8, IOBus::portout, MEM64::outF8H },
	
	{ 0x60, IOBus::portin,  MEM64::in6xH },
	{ 0x61, IOBus::portin,  MEM64::in6xH },
	{ 0x62, IOBus::portin,  MEM64::in6xH },
	{ 0x63, IOBus::portin,  MEM64::in6xH },
	{ 0x64, IOBus::portin,  MEM64::in6xH },
	{ 0x65, IOBus::portin,  MEM64::in6xH },
	{ 0x66, IOBus::portin,  MEM64::in6xH },
	{ 0x67, IOBus::portin,  MEM64::in6xH },
	{ 0x68, IOBus::portin,  MEM64::in6xH },
	{ 0x69, IOBus::portin,  MEM64::in6xH },
	{ 0x6a, IOBus::portin,  MEM64::in6xH },
	{ 0x6b, IOBus::portin,  MEM64::in6xH },
	{ 0x6c, IOBus::portin,  MEM64::in6xH },
	{ 0x6d, IOBus::portin,  MEM64::in6xH },
	{ 0x6e, IOBus::portin,  MEM64::in6xH },
	{ 0x6f, IOBus::portin,  MEM64::in6xH },
	
	{ 0xc2, IOBus::portin,  MEM64::inC2H },
	
	{ 0xf0, IOBus::portin,  MEM64::inF0H },
	{ 0xf1, IOBus::portin,  MEM64::inF1H },
	{ 0xf2, IOBus::portin,  MEM64::inF2H },
	{ 0xf3, IOBus::portin,  MEM64::inF3H },
	
	{ 0xb2, IOBus::portin,  MEM64::inB2H }
};

// VDG -----
const std::vector<IOBus::Connector> VM64::c_vdg = {
	{ 0x40, IOBus::portout, VDG64::out4xH },
	{ 0x41, IOBus::portout, VDG64::out4xH },
	{ 0x42, IOBus::portout, VDG64::out4xH },
	{ 0x43, IOBus::portout, VDG64::out4xH },
	{ 0xb0, IOBus::portout, VDG64::outB0H },
	{ 0xc0, IOBus::portout, VDG64::outC0H },
	{ 0xc1, IOBus::portout, VDG64::outC1H },
	{ 0xc8, IOBus::portout, VDG64::outC8H },
	{ 0xc9, IOBus::portout, VDG64::outC9H },
	{ 0xca, IOBus::portout, VDG64::outCAH },
	{ 0xcb, IOBus::portout, VDG64::outCBH },
	{ 0xcc, IOBus::portout, VDG64::outCCH },
	{ 0xce, IOBus::portout, VDG64::outCEH },
	{ 0xcf, IOBus::portout, VDG64::outCFH },
	{ 0xa2, IOBus::portin,  VDG64::inA2H }
};

// PSG -----
const std::vector<IOBus::Connector> VM64::c_psg = {
	{ 0xa0, IOBus::portout, OPN64::outA0H },
	{ 0xa1, IOBus::portout, OPN64::outA1H },
	{ 0xa3, IOBus::portout, OPN64::outA3H },
	{ 0xa2, IOBus::portin,  OPN64::inA2H },
	{ 0xa3, IOBus::portin,  OPN64::inA3H }
};

// 8255(Z80側) -----
const std::vector<IOBus::Connector> VM64::c_8255m = {
	{ 0x90, IOBus::portout, PIO6::out90H },
	{ 0x91, IOBus::portout, PIO6::out91H },
	{ 0x92, IOBus::portout, PIO6::out92H },
	{ 0x93, IOBus::portout, PIO6::out93H },
	{ 0x90, IOBus::portin,  PIO6::in90H },
	{ 0x92, IOBus::portin,  PIO6::in92H },
	{ 0x93, IOBus::portin,  PIO6::in93H }
};

// 8255(SUB CPU側) -----
const std::vector<IOBus::Connector> VM64::c_8255s = {
	{ IO8049_BUS, IOBus::portout, PIO6::outPBH },
	{ IO8049_BUS, IOBus::portin,  PIO6::inPBH  },
	{ IO8049_T0,  IOBus::portin,  PIO6::inIBF  },
	{ IO8049_INT, IOBus::portin,  PIO6::inOBF  }
};

// 音声合成 -----
const std::vector<IOBus::Connector> VM64::c_voice = {
	{ 0xe0, IOBus::portout, VCE64::outE0H },
	{ 0xe2, IOBus::portout, VCE64::outE2H },
	{ 0xe3, IOBus::portout, VCE64::outE3H },
	{ 0xe0, IOBus::portin,  VCE64::inE0H },
	{ 0xe2, IOBus::portin,  VCE64::inE2H },
	{ 0xe3, IOBus::portin,  VCE64::inE3H }
};

// DISK -----
const std::vector<IOBus::Connector> VM64::c_disk = {
	{ 0xd1, IOBus::portout, DSK60::outD1H },
	{ 0xd2, IOBus::portout, DSK60::outD2H },
	{ 0xd3, IOBus::portout, DSK60::outD3H },
	{ 0xd0, IOBus::portin,  DSK60::inD0H },
	{ 0xd1, IOBus::portin,  DSK60::inD1H },
	{ 0xd2, IOBus::portin,  DSK60::inD2H }
};

// CMT(LOAD) -----
const std::vector<IOBus::Connector> VM64::c_cmtl = {
	{ 0xb0, IOBus::portout, CMTL::outB0H }
};



// PC-6601SR ---------------------------------------------------

// DISK -----
const std::vector<IOBus::Connector> VM68::c_disk = {
	{ 0xb1, IOBus::portout, DSK66::outB1H },
	{ 0xb3, IOBus::portout, DSK66::outB3H },
	{ 0xd0, IOBus::portout, DSK66::outD0H },
	{ 0xd1, IOBus::portout, DSK66::outD1H },
	{ 0xd2, IOBus::portout, DSK66::outD2H },
	{ 0xd3, IOBus::portout, DSK66::outD3H },
	{ 0xd6, IOBus::portout, DSK66::outD6H },
	{ 0xd8, IOBus::portout, DSK66::outD8H },
	{ 0xda, IOBus::portout, DSK66::outDAH },
	{ 0xdd, IOBus::portout, DSK66::outDDH },
	{ 0xde, IOBus::portout, DSK66::outDEH },
	{ 0xb2, IOBus::portin,  DSK66::inB2H },
	{ 0xd0, IOBus::portin,  DSK66::inD0H },
	{ 0xd1, IOBus::portin,  DSK66::inD1H },
	{ 0xd2, IOBus::portin,  DSK66::inD2H },
	{ 0xd3, IOBus::portin,  DSK66::inD3H },
	{ 0xd4, IOBus::portin,  DSK66::inD4H },
	{ 0xdc, IOBus::portin,  DSK66::inDCH },
	{ 0xdd, IOBus::portin,  DSK66::inDDH }
};
