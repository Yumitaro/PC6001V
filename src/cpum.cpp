#include "pc6001v.h"

#include "config.h"
#include "cpum.h"
#include "p6vm.h"


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
CPU6::CPU6( VM6* vm, const ID& id ) : Device( vm, id )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
CPU6::~CPU6( void )
{
}


////////////////////////////////////////////////////////////////
// フェッチ(M1)
////////////////////////////////////////////////////////////////
BYTE CPU6::Fetch( WORD addr, int* m1wait )
{
	return vm->MemFetch( addr, m1wait );
}


////////////////////////////////////////////////////////////////
// メモリアクセス(ウェイトなし)
////////////////////////////////////////////////////////////////
BYTE CPU6::ReadMemNW( WORD addr )
{
	return vm->MemRead( addr );
}


////////////////////////////////////////////////////////////////
// メモリアクセス(ウェイトあり)
////////////////////////////////////////////////////////////////
BYTE CPU6::ReadMem( WORD addr )
{
	return vm->MemRead( addr, &mstate );
}

void CPU6::WriteMem( WORD addr, BYTE data)
{
	vm->MemWrite( addr, data, &mstate );
}


////////////////////////////////////////////////////////////////
// I/Oポートアクセス
////////////////////////////////////////////////////////////////
BYTE CPU6::ReadIO( int addr )
{
	return vm->IomIn( addr, &mstate );
}

void CPU6::WriteIO( int addr, BYTE data )
{
	vm->IomOut( addr, data, &mstate );
}


////////////////////////////////////////////////////////////////
// 割込みベクタ取得
////////////////////////////////////////////////////////////////
int CPU6::GetIntrVector( void )
{
	return vm->IntIntrCheck();
}


////////////////////////////////////////////////////////////////
// バスリクエスト区間停止フラグ取得
////////////////////////////////////////////////////////////////
bool CPU6::IsBUSREQ( void )
{
	return
		#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		vm->IsMonitor() ? false :
		#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		vm->VdgIsBusReqStop();
}


////////////////////////////////////////////////////////////////
// どこでもSAVE
////////////////////////////////////////////////////////////////
bool CPU6::DokoSave( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->PutValue( "Z80", "AF",			"",	AF.W,    "0x%04X" );
	Ini->PutValue( "Z80", "BC",			"",	BC.W,    "0x%04X" );
	Ini->PutValue( "Z80", "DE",			"",	DE.W,    "0x%04X" );
	Ini->PutValue( "Z80", "HL",			"",	HL.W,    "0x%04X" );
	Ini->PutValue( "Z80", "IX",			"",	IX.W,    "0x%04X" );
	Ini->PutValue( "Z80", "IY",			"",	IY.W,    "0x%04X" );
	Ini->PutValue( "Z80", "PC",			"",	PC.W,    "0x%04X" );
	Ini->PutValue( "Z80", "SP",			"",	SP.W,    "0x%04X" );
	Ini->PutValue( "Z80", "AF1",		"",	AF1.W,   "0x%04X" );
	Ini->PutValue( "Z80", "BC1",		"",	BC1.W,   "0x%04X" );
	Ini->PutValue( "Z80", "DE1",		"",	DE1.W,   "0x%04X" );
	Ini->PutValue( "Z80", "HL1",		"",	HL1.W,   "0x%04X" );
	Ini->PutValue( "Z80", "I",			"",	I,       "0x%02X" );
	Ini->PutValue( "Z80", "R",			"",	R,       "0x%02X" );
	Ini->PutValue( "Z80", "R_saved",	"",	R_saved, "0x%02X" );
	Ini->PutValue( "Z80", "IFF",		"",	IFF,     "0x%02X" );
	Ini->PutValue( "Z80", "IFF2",		"",	IFF2,    "0x%02X" );
	Ini->PutValue( "Z80", "IM",			"",	IM,      "0x%02X" );
	Ini->PutValue( "Z80", "Halt",		"",	Halt,    "0x%02X" );
	
	Ini->PutValue( "Z80", "mstate",		"", mstate );
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOAD
////////////////////////////////////////////////////////////////
bool CPU6::DokoLoad( cIni* Ini )
{
	if( !Ini ) return false;
	
	Ini->GetValue( "Z80", "AF",		AF.W    );
	Ini->GetValue( "Z80", "BC",		BC.W    );
	Ini->GetValue( "Z80", "DE",		DE.W    );
	Ini->GetValue( "Z80", "HL",		HL.W    );
	Ini->GetValue( "Z80", "IX",		IX.W    );
	Ini->GetValue( "Z80", "IY",		IY.W    );
	Ini->GetValue( "Z80", "PC",		PC.W    );
	Ini->GetValue( "Z80", "SP",		SP.W    );
	Ini->GetValue( "Z80", "AF1",		AF1.W   );
	Ini->GetValue( "Z80", "BC1",		BC1.W   );
	Ini->GetValue( "Z80", "DE1",		DE1.W   );
	Ini->GetValue( "Z80", "HL1",		HL1.W   );
	Ini->GetValue( "Z80", "I",		I       );
	Ini->GetValue( "Z80", "R",		R       );
	Ini->GetValue( "Z80", "R_saved",	R_saved );
	Ini->GetValue( "Z80", "IFF",		IFF     );
	Ini->GetValue( "Z80", "IFF2",	IFF2    );
	Ini->GetValue( "Z80", "IM",		IM      );
	Ini->GetValue( "Z80", "Halt",	Halt    );
	
	Ini->GetValue( "Z80", "mstate",	mstate  );
	
	return true;
}
