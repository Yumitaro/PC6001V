#include <cstring>
#include <fstream>
#include <new>
#include <unordered_map>

#include "common.h"
#include "error.h"
#include "log.h"
#include "memory.h"
#include "osd.h"
#include "p6vm.h"


// メモリブロック割り当て
#define EMPTYROM	(RomB[0])	// EmptyRom
#define EMPTYRAM	(RomB[1])	// EmptyRam
#define EXTROM0		(RomB[2])	// ExtRom
#define EXTROM1		(RomB[3])	// ExtRom+0x2000
#define MAINROM0	(RomB[4])	// SysRom1
#define MAINROM1	(RomB[5])	// SysRom1+0x2000
#define MAINROM2	(RomB[6])	// SysRom1+0x4000
#define MAINROM3	(RomB[7])	// SysRom1+0x6000
#define MAINROM4	(RomB[8])	// SysRom1+0x8000
#define MAINROM5	(RomB[9])	// SysRom1+0xa000
#define MAINROM6	(RomB[10])	// SysRom1+0xc000
#define MAINROM7	(RomB[11])	// SysRom1+0xe000
#define CGROM1		(RomB[12])	// CGRom1
#define CGROM2		(RomB[13])	// CGRom2			CGRom1+0x2000
#define KANJIROM0	(RomB[14])	// KanjiRom			SysRom2+0x8000
#define KANJIROM1	(RomB[15])	// KanjiRom+0x2000	SysRom2+0xa000
#define KANJIROM2	(RomB[16])	// KanjiRom+0x4000	SysRom2+0xc000
#define KANJIROM3	(RomB[17])	// KanjiRom+0x6000	SysRom2+0xe000
#define VOICEROM0	(RomB[18])	// VoiceRom			SysRom2+0x4000
#define VOICEROM1	(RomB[19])	// VoiceRom+0x2000	SysRom2+0x6000
#define SRMENROM0	(RomB[20])	//					SysRom2
#define SRMENROM1	(RomB[21])	//					SysRom2+0x2000

#define INTRAM0		(RamB[0])	// IntRam
#define INTRAM1		(RamB[1])	// IntRam+0x2000
#define INTRAM2		(RamB[2])	// IntRam+0x4000
#define INTRAM3		(RamB[3])	// IntRam+0x6000
#define INTRAM4		(RamB[4])	// IntRam+0x8000
#define INTRAM5		(RamB[5])	// IntRam+0xa000
#define INTRAM6		(RamB[6])	// IntRam+0xc000
#define INTRAM7		(RamB[7])	// IntRam+0xe000
#define EXTRAM0		(RamB[8])	// ExtRam
#define EXTRAM1		(RamB[9])	// ExtRam+0x2000
#define EXTRAM2		(RamB[10])	// ExtRam+0x4000
#define EXTRAM3		(RamB[11])	// ExtRam+0x6000
#define EXTRAM4		(RamB[12])	// ExtRam+0x8000
#define EXTRAM5		(RamB[13])	// ExtRam+0xa000
#define EXTRAM6		(RamB[14])	// ExtRam+0xc000
#define EXTRAM7		(RamB[15])	// ExtRam+0xe000
#define INEXRAM		(RamB[16])	// IntRam+ExtRam


#define rMAINROM0	((SolVer && Sol60Mode) ? &EXTRAM0 : &MAINROM0)
#define rMAINROM1	((SolVer && Sol60Mode) ? &EXTRAM1 : &MAINROM1)
#define wMAINROM0	((SolVer && Sol60Mode) ? &EXTRAM0 : &EMPTYROM)
#define wMAINROM1	((SolVer && Sol60Mode) ? &EXTRAM1 : &EMPTYROM)
#define pEXTRAM0	(SolVer ? &EXTRAM4 : &EXTRAM0)
#define pEXTRAM1	(SolVer ? &EXTRAM5 : &EXTRAM1)
#define wEXTROM0	(SolVer ? &EXTRAM2 : &EMPTYROM)
#define wEXTROM1	(SolVer ? &EXTRAM3 : &EMPTYROM)

#define pKNJROM0	(kj_rom ? ( kj_LR ? &KANJIROM2 : &KANJIROM0 ) : &SRMENROM0 )
#define pKNJROM1	(kj_rom ? ( kj_LR ? &KANJIROM3 : &KANJIROM1 ) : &SRMENROM1 )
#define pKNJROM2	(kj_rom ? ( kj_LR ? &KANJIROM2 : &KANJIROM0 ) : &VOICEROM0 )
#define pKNJROM3	(kj_rom ? ( kj_LR ? &KANJIROM3 : &KANJIROM1 ) : &VOICEROM1 )



// メモリコントローラ内部レジスタ初期値
#define	INIT_RF0	(0x71)
#define INIT_RF1	(0xdd)
#define INIT_RF2	(0x50)

#define ROMBANK		(0x00)
#define RAMBANK		(0x40)
#define SCCBANK		(0x80)
#define NONBANK		(0xc0)



////////////////////////////////////////////////////////////////
// ROM情報
////////////////////////////////////////////////////////////////

// Dummy                                   ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> NOROM     = {};

// PC-6001                                 ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> SYSROM160 = { { "BASICROM.60",	0x04000,	0x54c03109 } };
const std::vector<ROMINFO> CGROM160  = { { "CGROM60.60",	0x01000,	0xb0142d32 } };

// PC-6001A                                ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> SYSROM161 = { { "BASICROM.61",	0x04000,	0xfa8e88d9 } };
const std::vector<ROMINFO> CGROM161  = { { "CGROM60.61",	0x01000,	0x49c21d08 } };

// PC-6001mk2                              ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> SYSROM162 = { { "BASICROM.62",	0x08000,	0x950ac401 },	// 前期
										 { "BASICROM.62",	0x08000,	0xd7e61957 } };	// 後期
const std::vector<ROMINFO> CGROM162  = { { "CGROM60.62",	0x02000,	0x81eb5d95 } };
const std::vector<ROMINFO> CGROM262  = { { "CGROM60m.62",	0x02000,	0x3ce48c33 } };
const std::vector<ROMINFO> KANJI62   = { { "KANJIROM.62",	0x08000,	0x20c8f3eb } };
const std::vector<ROMINFO> VOICE62   = { { "VOICEROM.62",	0x04000,	0x49b4f917 } };

// PC-6601                                 ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> SYSROM166 = { { "BASICROM.66",	0x08000,	0xc0b01772 } };
const std::vector<ROMINFO> CGROM166  = { { "CGROM60.66",	0x02000,	0xd2434f29 } };
const std::vector<ROMINFO> CGROM266  = { { "CGROM66.66",	0x02000,	0x3ce48c33 } };
const std::vector<ROMINFO> KANJI66   = { { "KANJIROM.66",	0x08000,	0x20c8f3eb } };
const std::vector<ROMINFO> VOICE66   = { { "VOICEROM.66",	0x04000,	0x91d078c1 } };

// PC-6001mk2SR / PC-6601SR                ROMファイル名	サイズ		CRC32
const std::vector<ROMINFO> SYSROM164 = { { "SYSTEMROM1.64",	0x10000,	0xb6fc2db2 },
										 { "SYSTEMROM1.68",	0x10000,	0xb6fc2db2 } };
const std::vector<ROMINFO> SYSROM264 = { { "SYSTEMROM2.64",	0x10000,	0x55a62a1d },
										 { "SYSTEMROM2.68",	0x10000,	0x55a62a1d } };
const std::vector<ROMINFO> CGROM164  = { { "CGROM68.64",	0x04000,	0x73bc3256 },
										 { "CGROM68.68",	0x04000,	0x73bc3256 } };



////////////////////////////////////////////////////////////////
// ROMセット情報
////////////////////////////////////////////////////////////////
const std::unordered_map<int, const std::vector<std::vector<ROMINFO>>> ROMSET {
										{ 0,  {}													},
										{ 60, { SYSROM160, CGROM160 }								},
										{ 61, { SYSROM161, CGROM161 }								},
										{ 62, { SYSROM162, CGROM162,  CGROM262, KANJI62, VOICE62 }	},
										{ 66, { SYSROM166, CGROM166,  CGROM266, KANJI66, VOICE66 }	},
										{ 64, { SYSROM164, SYSROM264, CGROM164 }					},
										{ 68, { SYSROM164, SYSROM264, CGROM164 }					} };



////////////////////////////////////////////////////////////////
// ROMセット情報取得
////////////////////////////////////////////////////////////////
const std::vector<std::vector<ROMINFO>>& GetRomSetList( const int model )
{
	try{
		return ROMSET.at( model );
	}
	catch( std::out_of_range& ){
		return ROMSET.at( 0 );
	}
}




////////////////////////////////////////////////////////////////
// メモリ情報
////////////////////////////////////////////////////////////////
// 共通                                 ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM6::IEMPTROM   = { NOROM,		0x02000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEMPTRAM   = { NOROM,		0x02000,	0xff,	0 };
const MEM6::MEMINFO MEM6::IEXTROM16  = { NOROM,		0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM128 = { NOROM,		0x20000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM512 = { NOROM,		0x80000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTRAM16  = { NOROM,		0x04000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM64  = { NOROM,		0x10000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM128 = { NOROM,		0x20000,	0x00,	0 };
									 	// 戦士のカートリッジ対応のため
									 	// 拡張ROM領域は512KB,拡張RAM領域は128KB確保しておく

// PC-6001                               ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM60::ISYSROM1  = { SYSROM160,	0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM60::ICGROM1   = { CGROM160,	0x01000,	0xff,	1 };
const MEM6::MEMINFO MEM60::IINTRAM   = { NOROM,		0x04000,	0x00,	0 };

// PC-6001A                              ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM61::ISYSROM1  = { SYSROM161,	0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM61::ICGROM1   = { CGROM161,	0x01000,	0xff,	1 };
const MEM6::MEMINFO MEM61::IINTRAM   = { NOROM,		0x04000,	0x00,	0 };

// PC-6001mk2                            ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM62::ISYSROM1  = { SYSROM162,	0x08000,	0xff,	1 };
const MEM6::MEMINFO MEM62::ICGROM1   = { CGROM162,	0x02000,	0xff,	1 };
const MEM6::MEMINFO MEM62::ICGROM2   = { CGROM262,	0x02000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IKANJI    = { KANJI62,	0x08000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IVOICE    = { VOICE62,	0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IINTRAM   = { NOROM,		0x10000,	0x00,	0 };

// PC-6601                               ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM66::ISYSROM1  = { SYSROM166,	0x08000,	0xff,	1 };
const MEM6::MEMINFO MEM66::ICGROM1   = { CGROM166,	0x02000,	0xff,	1 };
const MEM6::MEMINFO MEM66::ICGROM2   = { CGROM266,	0x02000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IKANJI    = { KANJI66,	0x08000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IVOICE    = { VOICE66,	0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IINTRAM   = { NOROM,		0x10000,	0x00,	0 };

// PC-6001mk2SR / PC-6601SR              ROM情報		サイズ		初期値	Wait
const MEM6::MEMINFO MEM64::ISYSROM1  = { SYSROM164,	0x10000,	0xff,	1 };
const MEM6::MEMINFO MEM64::ISYSROM2  = { SYSROM264,	0x10000,	0xff,	1 };
const MEM6::MEMINFO MEM64::ICGROM1   = { CGROM164,	0x04000,	0xff,	1 };
const MEM6::MEMINFO MEM64::IINTRAM   = { NOROM,		0x10000,	0x00,	0 };



// 実験 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ダミーメモリセル
MemCell EmptyCell( 0xff, true );


//--------------------------------------------------------------
// メモリセル(最小単位)
//--------------------------------------------------------------

////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MemCell::MemCell( BYTE idata, bool wp ) : WPt( wp )
{
	Data.assign( PAGESIZE, idata );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
MemCell::~MemCell( void )
{
}


////////////////////////////////////////////////////////////////
// ROMデータをファイルから読込み
////////////////////////////////////////////////////////////////
void MemCell::SetData( std::fstream& fs )
{
	size_t sz = fs.read( (char*)Data.data(), PAGESIZE ).gcount();
	if( sz < PAGESIZE ){	// 端数だったらセルサイズ縮小
		Data.resize( sz );
	}
	WPt = true;	// データ読込んだらROM扱い
}


////////////////////////////////////////////////////////////////
// サイズ取得
////////////////////////////////////////////////////////////////
size_t MemCell::Size( void ) const
{
	return Data.size();
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MemCell::Read( WORD addr ) const
{
	try{
		return Data.at( addr & PAGEMASK );
	}
	catch( std::out_of_range& ){}
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MemCell::Write( WORD addr, BYTE data )
{
	try{
		if( WPt ){
			Data.at( addr & PAGEMASK ) = data;
		}
	}
	catch( std::out_of_range& ){}
}


////////////////////////////////////////////////////////////////
// ※暫定※ データポインタ取得
////////////////////////////////////////////////////////////////
BYTE* MemCell::GetData( void )
{
	return Data.data();
}




//--------------------------------------------------------------
// メモリセル集合体(ROM/RAMチップ相当)
//--------------------------------------------------------------

////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MemCells::MemCells(  size_t cnum, BYTE idata, bool wp  )
{
	Cells.assign( cnum, MemCell( idata, wp ) );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
MemCells::~MemCells( void )
{
}


////////////////////////////////////////////////////////////////
// ROMデータをファイルから読込み
////////////////////////////////////////////////////////////////
bool MemCells::SetData( const P6VPATH& filepath )
{
	try{
		// ファイルサイズに合わせてメモリセル再設定(端数考慮)
		Cells.resize( (OSD_GetFileSize( filepath ) + MemCell::PAGEBITS) >> MemCell::PAGEBITS );
		
		std::fstream fs;
		if( !OSD_FSopen( fs, filepath, std::ios_base::in|std::ios_base::binary ) ) throw Error::NoRom;
		
		std::for_each( Cells.begin(), Cells.end(), [&]( MemCell& mc ){
			mc.SetData( fs );
		});
		fs.close();
	}
	// 例外発生
	catch( Error::Errno i ){
		Error::SetError( i );
		return false;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// サイズ(メモリセル数)取得
////////////////////////////////////////////////////////////////
size_t MemCells::Size( void ) const
{
	return Cells.size();
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MemCells::Read( WORD addr ) const
{
	try{
		return Cells.at( addr >> MemCell::PAGEBITS ).Read( addr );
	}
	catch( std::out_of_range& ){}
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MemCells::Write( WORD addr, BYTE data )
{
	try{
		Cells.at( addr >> MemCell::PAGEBITS ).Write( addr, data );
	}
	catch( std::out_of_range& ){}
}


////////////////////////////////////////////////////////////////
// メモリセル取得
////////////////////////////////////////////////////////////////
const MemCell& MemCells::GetCell( const int num ) const
{
	try{
		return Cells.at( num );
	}
	catch( std::out_of_range& ){}
	
	return EmptyCell;
}




//--------------------------------------------------------------
// メモリブロック
//--------------------------------------------------------------

////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MemBlk::MemBlk( void ) : Name( "" ), PMem( EmptyCell ), Inst( nullptr ),
						 FRead( nullptr ), FWrite( nullptr ), Wait( 0 )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
MemBlk::~MemBlk( void )
{
}


////////////////////////////////////////////////////////////////
// メモリ割当て
//
// 引数:	name	メモリブロック名への参照
//			data	メモリセルへの参照
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlk::SetMemory( const std::string& name, MemCell& data, int wait )
{
	Name   = name;
	Inst   = nullptr;
	FRead  = nullptr;
	FWrite = nullptr;
	PMem   = data;
	Wait   = wait < 0 ? Wait : wait;
}


////////////////////////////////////////////////////////////////
// 関数割当て
//
// 引数:	name	メモリブロック名への参照
//			data	メモリセルへの参照
//			inst	オブジェクトポインタ
//			rd		読込み関数ポインタ
//			wr		書込み関数ポインタ
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlk::SetFunc( const std::string& name, MemCell& data, IDevice* inst, RFuncPtr rd, WFuncPtr wr, int wait )
{
	Name   = name;
	Inst   = inst;
	FRead  = inst ? rd   : nullptr;
	FWrite = inst ? wr   : nullptr;
	PMem   = inst ? data : EmptyCell;
	Wait   = wait < 0 ? Wait : wait;
}


////////////////////////////////////////////////////////////////
// アクセスウェイト設定
//
// 引数:	wait	アクセスウェイト(0-255)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlk::SetWait( int wait )
{
	Wait = wait & 0xff;
}


////////////////////////////////////////////////////////////////
// アクセスウェイト取得
//
// 引数:	なし
// 返値:	ウェイト
////////////////////////////////////////////////////////////////
int MemBlk::GetWait( void ) const
{
	return Wait;
}


////////////////////////////////////////////////////////////////
// メモリブロック名取得
//
// 引数:	なし
// 返値:	std::string& 	メモリブロック名への参照
////////////////////////////////////////////////////////////////
const std::string& MemBlk::GetName( void ) const
{
	return Name;
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MemBlk::Read( WORD addr, int* wcnt ) const
{
	if( wcnt ){
		*wcnt += Wait;
	}
	
	if( Inst && FRead ){
		return (Inst->*FRead)( PMem.GetData(), addr );
	}else{
		return PMem.Read( addr );
	}
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MemBlk::Write( WORD addr, BYTE data, int* wcnt ) const
{
	if( wcnt ){
		*wcnt += Wait;
	}
	
	if( Inst && FWrite ){
		(Inst->*FWrite)( PMem.GetData(), addr, data );
	}else{
		PMem.Write( addr, data );
	}
}



// 実験 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++













//--------------------------------------------------------------
// メモリブロック
//--------------------------------------------------------------

////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MemBlock::MemBlock( void ) : Name( "" ), PMem( nullptr ), Inst( nullptr ), FRead( nullptr ), FWrite( nullptr ),
							 Wait( 0 ), WPt( false )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
MemBlock::~MemBlock( void )
{
}


////////////////////////////////////////////////////////////////
// メモリ割当て
//
// 引数:	name	メモリブロック名への参照
//			mem		メモリブロックへのポインタ
//			wait	アクセスウェイト(-1:変更しない)
//			prt		ライトプロテクトフラグ true:セット false：解除
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetMemory( const std::string& name, BYTE* mem, int wait, bool prt )
{
	Name   = name;
	Inst   = nullptr;
	FRead  = nullptr;
	FWrite = nullptr;
	PMem   = mem;
	Wait   = wait < 0 ? Wait : wait;
	WPt    = prt;
}


////////////////////////////////////////////////////////////////
// ROM割当て
//
// 引数:	name	メモリブロック名への参照
//			mem		メモリブロックへのポインタ
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetRom( const std::string& name, BYTE* mem, int wait )
{
	SetMemory( name, mem, wait, true );
}


////////////////////////////////////////////////////////////////
// RAM割当て
//
// 引数:	name	メモリブロック名への参照
//			mem		メモリブロックへのポインタ
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetRam( const std::string& name, BYTE* mem, int wait )
{
	SetMemory( name, mem, wait, false );
}


////////////////////////////////////////////////////////////////
// 関数割当て
//
// 引数:	name	メモリブロック名への参照
//			data	関数用データポインタ
//			inst	オブジェクトポインタ
//			rd		読込み関数ポインタ
//			wr		書込み関数ポインタ
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetFunc( const std::string& name, BYTE* data, IDevice* inst, RFuncPtr rd, WFuncPtr wr, int wait )
{
	Name   = name;
	Inst   = inst;
	FRead  = Inst ? rd   : nullptr;
	FWrite = Inst ? wr   : nullptr;
	PMem   = Inst ? data : nullptr;
	Wait   = wait < 0 ? Wait : wait;
	WPt    = wr ? false : true;
}


////////////////////////////////////////////////////////////////
// アクセスウェイト設定
//
// 引数:	wait	アクセスウェイト
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetWait( int wait )
{
	Wait = wait & 0xff;
}


////////////////////////////////////////////////////////////////
// アクセスウェイト取得
//
// 引数:	なし
// 返値:	ウェイト
////////////////////////////////////////////////////////////////
int MemBlock::GetWait( void ) const
{
	return Wait;
}


////////////////////////////////////////////////////////////////
// メモリブロック名取得
//
// 引数:	なし
// 返値:	std::string& 	メモリブロック名への参照
////////////////////////////////////////////////////////////////
const std::string& MemBlock::GetName( void ) const
{
	return Name;
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MemBlock::Read( WORD addr, int* wcnt ) const
{
	if( wcnt ) *wcnt += Wait;
	
	if( Inst && FRead ) return (Inst->*FRead)( PMem, addr );
	else if( PMem )     return ((BYTE*)PMem)[addr & PAGEMASK];
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MemBlock::Write( WORD addr, BYTE data, int* wcnt ) const
{
	if( wcnt ) *wcnt += Wait;
	
	if( WPt ) return;
	
	if( Inst && FWrite ) (Inst->*FWrite)( PMem, addr, data );
	else if( PMem)       ((BYTE*)PMem)[addr & PAGEMASK] = data;
}










////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MEM6::MEM6( VM6* vm, const ID& id ) : Device( vm, id ),
	CGBank( false ), UseExtRom( false ), UseExtRam( false ),
	FilePath( "" ), M1Wait( 1 ), EnableChkCRC( true ),
	cgrom( true ), kj_rom( true ), kj_LR( true ), cgenable( true ), cgaden( 0 ), cgaddr( 3 ), c2acc( 0xff ),
	SolVer( 0 ), Sol60Mode( false ), SolBankSet( 0 )
{
	Rf[0] = INIT_RF0;
	Rf[1] = INIT_RF1;
	Rf[2] = INIT_RF2;
	
	INITARRAY( Rm_blk, nullptr );
	INITARRAY( Wm_blk, nullptr );
	
	INITARRAY( Rm_blkSR, nullptr );
	INITARRAY( Wm_blkSR, nullptr );
	INITARRAY( RfSR, 0 );
	
	MemTable.ExtRom  = &MEM6::IEXTROM16;
	MemTable.ExtRam  = &MEM6::IEXTRAM16;
	
	INITARRAY( SolBank, NONBANK );
}

MEM60::MEM60( VM6* vm, const ID& id ) : MEM6( vm, id )
{
	MemTable.IntRam  = &MEM60::IINTRAM;
	MemTable.System1 = &MEM60::ISYSROM1;
	MemTable.CGRom1  = &MEM60::ICGROM1;
	
	// Dvice Description (Out)
	descs.outdef.emplace( out06H, STATIC_CAST( Device::OutFuncPtr, &MEM60::Out06H ) );
	descs.outdef.emplace( out3xH, STATIC_CAST( Device::OutFuncPtr, &MEM60::Out3xH ) );
	descs.outdef.emplace( out7FH, STATIC_CAST( Device::OutFuncPtr, &MEM60::Out7FH ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutF0H ) );	// 戦士のカートリッジ対応
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutF2H ) );	// 戦士のカートリッジ対応
}

MEM61::MEM61( VM6* vm, const ID& id ) : MEM60( vm, id )
{
	MemTable.IntRam  = &MEM61::IINTRAM;
	MemTable.System1 = &MEM61::ISYSROM1;
	MemTable.CGRom1  = &MEM61::ICGROM1;
}

MEM62::MEM62( VM6* vm, const ID& id ) : MEM6( vm, id )
{
	MemTable.IntRam  = &MEM62::IINTRAM;
	MemTable.System1 = &MEM62::ISYSROM1;
	MemTable.CGRom1  = &MEM62::ICGROM1;
	MemTable.CGRom2  = &MEM62::ICGROM2;
	MemTable.Kanji   = &MEM62::IKANJI;
	MemTable.Voice   = &MEM62::IVOICE;
	MemTable.ExtRam  = &MEM6::IEXTRAM64;
	
	// Dvice Description (In)
	descs.outdef.emplace( out06H, STATIC_CAST( Device::OutFuncPtr, &MEM62::Out06H ) );
	descs.outdef.emplace( out3xH, STATIC_CAST( Device::OutFuncPtr, &MEM62::Out3xH ) );
	descs.outdef.emplace( out7FH, STATIC_CAST( Device::OutFuncPtr, &MEM62::Out7FH ) );
	descs.outdef.emplace( outC1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC1H ) );
	descs.outdef.emplace( outC2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC2H ) );
	descs.outdef.emplace( outC3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC3H ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF0H ) );
	descs.outdef.emplace( outF1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF1H ) );
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF2H ) );
	descs.outdef.emplace( outF3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF3H ) );
	descs.outdef.emplace( outF8H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF8H ) );
	
	// Dvice Description (Out)
	descs.indef.emplace ( inC2H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InC2H  ) );
	descs.indef.emplace ( inF0H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF0H  ) );
	descs.indef.emplace ( inF1H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF1H  ) );
	descs.indef.emplace ( inF2H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF2H  ) );
	descs.indef.emplace ( inF3H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF3H  ) );
}

MEM66::MEM66( VM6* vm, const ID& id ) : MEM62( vm, id )
{
	MemTable.IntRam  = &MEM66::IINTRAM;
	MemTable.System1 = &MEM66::ISYSROM1;
	MemTable.CGRom1  = &MEM66::ICGROM1;
	MemTable.CGRom2  = &MEM66::ICGROM2;
	MemTable.Kanji   = &MEM66::IKANJI;
	MemTable.Voice   = &MEM66::IVOICE;
	MemTable.ExtRam  = &MEM6::IEXTRAM64;
}

MEM64::MEM64( VM6* vm, const ID& id ) : MEM62( vm, id )
{
	MemTable.IntRam  = &MEM64::IINTRAM;
	MemTable.System1 = &MEM64::ISYSROM1;
	MemTable.System2 = &MEM64::ISYSROM2;
	MemTable.CGRom1  = &MEM64::ICGROM1;
	MemTable.CGRom2  = nullptr;
	MemTable.Kanji   = nullptr;
	MemTable.Voice   = nullptr;
	MemTable.ExtRam  = &MEM6::IEXTRAM64;
	
	// Dvice Description (In)
	descs.outdef.clear();
	descs.outdef.emplace( out06H, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out06H ) );
	descs.outdef.emplace( out3xH, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out3xH ) );
	descs.outdef.emplace( out7FH, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out7FH ) );
	descs.outdef.emplace( out6xH, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out6xH ) );
	descs.outdef.emplace( outC1H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC1H ) );
	descs.outdef.emplace( outC2H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC2H ) );
	descs.outdef.emplace( outC3H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC3H ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF0H ) );
	descs.outdef.emplace( outF1H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF1H ) );
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF2H ) );
	descs.outdef.emplace( outF3H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF3H ) );
	descs.outdef.emplace( outF8H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF8H ) );
	
	// Dvice Description (Out)
	descs.indef.clear();
	descs.indef.emplace ( in6xH,  STATIC_CAST( Device::InFuncPtr,  &MEM64::In6xH  ) );
	descs.indef.emplace ( inC2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InC2H  ) );
	descs.indef.emplace ( inF0H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF0H  ) );
	descs.indef.emplace ( inF1H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF1H  ) );
	descs.indef.emplace ( inF2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF2H  ) );
	descs.indef.emplace ( inF3H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF3H  ) );
	descs.indef.emplace ( inB2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InB2H  ) );
}

MEM68::MEM68( VM6* vm, const ID& id ) : MEM64( vm, id )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
MEM6::~MEM6( void )
{
}

MEM60::~MEM60( void )
{
}

MEM61::~MEM61( void )
{
}

MEM62::~MEM62( void )
{
}

MEM66::~MEM66( void )
{
}

MEM64::~MEM64( void )
{
}

MEM68::~MEM68( void )
{
}



////////////////////////////////////////////////////////////////
// メモリブロック用関数
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 戦士のカートリッジ読込み(拡張ROM領域)
//   mkII以降の場合,拡張ROMは4000-7FFFH以外に割当てることができるので
//   どこに割当てられてもアクセスできるように一段階かませる
////////////////////////////////////////////////////////////////
BYTE MEM6::SolReadEx( BYTE* ptr, WORD addr )
{
	MemBlock* mb = &EXTRAM0;
	
	return mb[addr>>MemBlock::PAGEBITS].Read( addr );
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジROM/RAM読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::SolMemRead( BYTE* ptr, WORD addr )
{
	return ptr[addr & MemBlock::PAGEMASK];
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジRAM書込み
////////////////////////////////////////////////////////////////
void MEM6::SolMemWrite( BYTE* ptr, WORD addr, BYTE data )
{
	ptr[addr & MemBlock::PAGEMASK] = data;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::SolSccRead( BYTE* ptr, WORD addr )
{
	// 後で書く
	return 0xff;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC書込み
////////////////////////////////////////////////////////////////
void MEM6::SolSccWrite( BYTE* ptr, WORD addr, BYTE data )
{
	// 後で書く
}


////////////////////////////////////////////////////////////////
// PC-6001 CGROM読込み
////////////////////////////////////////////////////////////////
BYTE MEM60::CGromRead( BYTE* ptr, WORD addr )
{
	// 前半4KBはそのままデータを返す
	if( !(addr & 0x1000) )	return ptr[addr & MemBlock::PAGEMASK];
	
	// 後半4KBを読込んだ時の挙動を記述
	// 拡張ROMが無い場合はCGROMのイメージとなるらしい
	// 戦士のカートリッジの場合はホントはコレジャナイんだけど後回し
	return UseExtRom ? ExtRom[((((addr+0x1000) & MemBlock::PAGEMASK))>>MemBlock::PAGEBITS)+1][addr & MemBlock::PAGEMASK] : ptr[addr & (MemBlock::PAGEMASK>>1)];
}


////////////////////////////////////////////////////////////////
// PC-6001mk2以降 内部/外部RAM書込み
////////////////////////////////////////////////////////////////
void MEM62::IERamWrite( BYTE* ptr, WORD addr, BYTE data )
{
	MemBlock* imb = &INTRAM0;
	MemBlock* emb = &EXTRAM0;
	
	imb[addr>>MemBlock::PAGEBITS].Write( addr, data );
	emb[addr>>MemBlock::PAGEBITS].Write( addr, data );
}



////////////////////////////////////////////////////////////////
// メモリブロック用関数 ここまで
////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////
// 拡張ROM マウント
////////////////////////////////////////////////////////////////
bool MEM6::MountExtRom( const P6VPATH& filepath )
{
	PRINTD( MEM_LOG, "[MEM][MountExtRom] -> %s -> ", P6VPATH2STR( filepath ).c_str() );
	
	// マウント済みなら一旦開放
	if( UseExtRom ) UnmountExtRom();
	
	try{
		std::fstream fs;
		if( !OSD_FSopen( fs, filepath, std::ios_base::in|std::ios_base::binary ) ) throw Error::ExtRomMountFailed;
		std::for_each( ExtRom.begin(), ExtRom.end(), [&]( std::vector<BYTE>& db ){
			fs.read( (char*)db.data(), MemBlock::PAGESIZE );
		});
		fs.close();
		
		// ファイルパス保存
		FilePath = filepath;
	}
	// 例外発生
	catch( Error::Errno i ){
		Error::SetError( i );
		
		PRINTD( MEM_LOG, "Error\n" );
		
		UnmountExtRom();
		return false;
	}
	
	UseExtRom = true;
	
	PRINTD( MEM_LOG, "OK %s\n", P6VPATH2STR( FilePath ).c_str() );
	
	return true;
}


////////////////////////////////////////////////////////////////
// 拡張ROM アンマウント
////////////////////////////////////////////////////////////////
void MEM6::UnmountExtRom( void )
{
	PRINTD( MEM_LOG, "[MEM][UnmountExtRom]\n" );
	
	std::for_each( ExtRom.begin(), ExtRom.end(), [&]( std::vector<BYTE>& db ){
		db.assign( MemBlock::PAGESIZE, MemTable.ExtRom->Init );
	});
	FilePath.clear();
	
	UseExtRom = false;
}


////////////////////////////////////////////////////////////////
// 拡張ROMファイルパス取得
////////////////////////////////////////////////////////////////
const P6VPATH& MEM6::GetFile( void ) const
{
	return FilePath;
}


////////////////////////////////////////////////////////////////
// CRC32計算
//
// 引数:	buf				データバッファへのポインタ
//			num				データ数(バイト)
// 返値:	DWORD			CRC32値
////////////////////////////////////////////////////////////////
DWORD MEM6::CalcCrc32( std::vector<std::vector<BYTE>>& buf, int num )
{
	DWORD crc = 0xffffffff;
	
	for( int i=0; i < num; i++ ){
		crc ^= buf[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK];
		for( int j=0; j < 8; j++ )
			if( crc & 1 ) crc   = (crc >> 1) ^ 0xedb88320;
			else		  crc >>= 1;
	}
	return crc ^ 0xffffffff;
}


////////////////////////////////////////////////////////////////
// メモリ確保とROMファイル読込み
////////////////////////////////////////////////////////////////
bool MEM6::AllocMemory( std::vector<std::vector<BYTE>>& buf, const MEMINFO* info, const P6VPATH& path )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemory] " );
	
	int i = 0;
	bool ErrSize = false;
	bool ErrCrc = false;
	
	try{
		// 一旦メモリ解放
		buf.clear();
		
		// メモリ確保
		buf.assign( (info->Size+MemBlock::PAGEMASK)>>MemBlock::PAGEBITS, std::vector<BYTE>( MemBlock::PAGESIZE, info->Init ) );
		
		// ROM情報なし ならばRAMまたはnullptr
		if( info->Rinfo.empty() ) return true;
		
		// ファイル候補の数だけ繰り返し
		do{
			PRINTD( MEM_LOG, "-> %s ", info->Rinfo[i].FileName.c_str() );
			
			// ファイルから読込み
			P6VPATH fpath = path;
			OSD_AddPath( fpath, fpath, STR2P6VPATH( info->Rinfo[i].FileName ) );
			
			// ファイルの存在チェック
			if( !OSD_FileExist( fpath ) ) continue;
			
			// ファイルサイズチェック
			if( OSD_GetFileSize( fpath ) != info->Size ) ErrSize = true;
			
			std::fstream fs;
			if( OSD_FSopen( fs, fpath, std::ios_base::in|std::ios_base::binary ) ){
				std::for_each( buf.begin(), buf.end(), [&]( std::vector<BYTE>& db ){
					fs.read( (char*)db.data(), db.size() );
				});
				// CRCチェック
				// EnableChkCRC=false または CRC=0の時はチェックしない
				if( EnableChkCRC && (info->Rinfo[i].Crc != 0) &&
					( CalcCrc32( buf, info->Size ) != info->Rinfo[i].Crc ) ){
					ErrCrc = true;
				}else{
					PRINTD( MEM_LOG, "-> OK\n" );
					return true;
				}
			}
		}while( ++i < (int)info->Rinfo.size() );
		
		if     ( ErrCrc )  throw Error::RomCrcNG;
		else if( ErrSize ) throw Error::RomSizeNG;
		else               throw Error::NoRom;
	}
	catch( std::bad_alloc& ){	// new に失敗した場合
		PRINTD( MEM_LOG, "-> MemAlloc Failed\n" );
		Error::SetError( Error::MemAllocFailed );
		return false;
	}
	catch( Error::Errno i ){	// 例外発生
		PRINTD( MEM_LOG, "-> Failed\n" );
		
		Error::SetError( i );
		
		switch( i ){
		case Error::NoRom:		// ファイルのオープンに失敗した場合
		case Error::RomSizeNG:	// サイズが合わない場合
		case Error::RomCrcNG:	// CRCが合わない場合
		default:				// メモリを開放
			buf.clear();
		}
		return false;
	}
	
	return false;
}


////////////////////////////////////////////////////////////////
// 全メモリ確保とROMファイル読込み
////////////////////////////////////////////////////////////////
bool MEM6::AllocAllMemory( const P6VPATH& path, BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][AllocAllMemory]\n" );
	
	EnableChkCRC = data & MCRCCHK   ? true : false;		// CRCチェック有効
	UseExtRam    = data & MUSEEXRAM ? true : false;		// 外部RAM有効
	SolVer       = data & MUSESOL;						// 戦士のカートリッジ有無&バージョン
	
	// 戦士のカートリッジ使用時はRAM128KB
	if( SolVer ){
		MemTable.ExtRom = &MEM6::IEXTROM512;
		MemTable.ExtRam = &MEM6::IEXTRAM128;
	}
	
	// 共通
	if( !AllocMemory( ExtRom,   MemTable.ExtRom,  ""   ) ) return false;
	if( !AllocMemory( ExtRam,   MemTable.ExtRam,  ""   ) ) return false;
	if( !AllocMemory( IntRam,   MemTable.IntRam,  ""   ) ) return false;
	
	if( !AllocMemory( SysRom1,  MemTable.System1, path ) ) return false;
	if( !AllocMemory( CGRom1,   MemTable.CGRom1,  path ) ) return false;
	
	 // 全メモリ確保とROMファイル読込み(機種別)
	if( !AllocMemorySpecific( path ) ) return false;
	
	SetRamValue();
	
	return true;
}


////////////////////////////////////////////////////////////////
// 全メモリ確保とROMファイル読込み(機種別)
////////////////////////////////////////////////////////////////
bool MEM60::AllocMemorySpecific( const P6VPATH& path )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemorySpecific]\n" );
	
	return true;
}

bool MEM62::AllocMemorySpecific( const P6VPATH& path )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemorySpecific]\n" );
	
	if( !AllocMemory( CGRom2,   MemTable.CGRom2, path ) ) return false;
	if( !AllocMemory( KanjiRom, MemTable.Kanji,  path ) ) return false;
	if( !AllocMemory( VoiceRom, MemTable.Voice,  path ) ) return false;
	
	return true;
}

bool MEM64::AllocMemorySpecific( const P6VPATH& path )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemorySpecific]\n" );
	
	if( !AllocMemory( SysRom2,  MemTable.System2, path ) ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// RAMの初期値を設定
////////////////////////////////////////////////////////////////
void MEM60::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK] = i&0x40 ? 0x00 : 0xff;
	}
}

void MEM62::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK] = ((i>>7)^i)&1 ? 0xff : 0x00;
	}
}

void MEM64::SetRamValue( void )
{
	// 0000-FF9FH
	for( int i=0; i<0xffa0; i++ ){
		IntRam[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK] = ((i>>9)^(i>>1))&1 ? 0xff : 0x00;
	}
	// FFA0-FFFFH
	for( int i=0xffa0; i<0x10000; i++ ){
		IntRam[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK] = 0x00;
	}
}

void MEM68::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam[i>>MemBlock::PAGEBITS][i&MemBlock::PAGEMASK] = i&0x100 ? 0xff : 0x00;
	}
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool MEM6::Init( void )
{
	PRINTD( MEM_LOG, "[MEM][Init]\n" );
	
	// メモリブロック設定
	// とりあえず全てEmptyに設定(ROMはウェイトあり)
	for( int i=0; i<MAXRMB; i++ ){
		RomB[i].SetFunc( "EMPTY", nullptr, nullptr, nullptr, nullptr, 1 );
	}
	for( int i=0; i<MAXWMB; i++ ){
		RamB[i].SetFunc( "EMPTY", nullptr, nullptr, nullptr, nullptr, 0 );
	}
	
	// 拡張ROM領域
	if( SolVer ){			// 戦士のカートリッジ
		EXTROM0.SetFunc( "EXROM0", ExtRom[0].data(), this, (RFuncPtr)&MEM6::SolReadEx, nullptr, MemTable.ExtRom->Wait );
		EXTROM1.SetFunc( "EXROM1", ExtRom[1].data(), this, (RFuncPtr)&MEM6::SolReadEx, nullptr, MemTable.ExtRom->Wait );
	}else{
		EXTROM0.SetRom ( "EXROM0", ExtRom[0].data(), MemTable.ExtRom->Wait );
		EXTROM1.SetRom ( "EXROM1", ExtRom[1].data(), MemTable.ExtRom->Wait );
	}
	
	// 外部RAM設定(排他)
	if( SolVer ){			// 戦士のカートリッジ
		SetSolBank( 3, RAMBANK | 3 );
		SetSolBank( 4, RAMBANK | 4 );
		SetSolBank( 5, RAMBANK | 5 );
	}else if( UseExtRam ){	// RAMカートリッジ
		MemBlock* mb = &EXTRAM0;
		
		for( int i=0; i<(int)MemTable.ExtRam->Size / MemBlock::PAGESIZE; i++ ){
			(mb++)->SetRam( Stringf( "EXRAM%d", i ), ExtRam[i].data(), MemTable.ExtRam->Wait );
		}
	}
	
	 // 初期化(機種別)
	if( !InitSpecific() ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// 初期化(機種別)
////////////////////////////////////////////////////////////////
bool MEM60::InitSpecific( void )
{
	PRINTD( MEM_LOG, "[MEM][InitSpecific]\n" );
	
	// メモリブロック設定
	// BASIC ROM
	MAINROM0.SetRom( "BASIC0", SysRom1[0].data(), MemTable.System1->Wait );
	MAINROM1.SetRom( "BASIC1", SysRom1[1].data(), MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetFunc ( "CGROM1", CGRom1[0].data(), this, (RFuncPtr)&MEM60::CGromRead, nullptr, MemTable.CGRom1->Wait );
	
	// 内部RAM
	INTRAM0.SetRam ( "INRAM0", IntRam[0].data(),  MemTable.IntRam->Wait );
	INTRAM1.SetRam ( "INRAM1", IntRam[1].data(),  MemTable.IntRam->Wait );
	
	if( SolVer ){			// 戦士のカートリッジ
		EXTRAM0.SetWait( MemTable.System1->Wait );
		EXTRAM1.SetWait( MemTable.System1->Wait );
		EXTRAM2.SetWait( MemTable.ExtRom->Wait );
		EXTRAM3.SetWait( MemTable.ExtRom->Wait );
	}
	
	return true;
}

bool MEM62::InitSpecific( void )
{
	PRINTD( MEM_LOG, "[MEM][InitSpecific]\n" );
	
	// メモリブロック設定
	// BASIC ROM
	MAINROM0.SetRom ( "BASIC0", SysRom1[0].data(),  MemTable.System1->Wait );
	MAINROM1.SetRom ( "BASIC1", SysRom1[1].data(),  MemTable.System1->Wait );
	MAINROM2.SetRom ( "BASIC2", SysRom1[2].data(),  MemTable.System1->Wait );
	MAINROM3.SetRom ( "BASIC3", SysRom1[3].data(),  MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetRom   ( "CGROM1", CGRom1[0].data(),   MemTable.CGRom1->Wait );
	CGROM2.SetRom   ( "CGROM2", CGRom2[0].data(),   MemTable.CGRom2->Wait );
	
	// 漢字ROM
	KANJIROM0.SetRom( "KJROM0", KanjiRom[0].data(), MemTable.Kanji->Wait );
	KANJIROM1.SetRom( "KJROM1", KanjiRom[1].data(), MemTable.Kanji->Wait );
	KANJIROM2.SetRom( "KJROM2", KanjiRom[2].data(), MemTable.Kanji->Wait );
	KANJIROM3.SetRom( "KJROM3", KanjiRom[3].data(), MemTable.Kanji->Wait );
	
	// 音声合成ROM
	VOICEROM0.SetRom( "VOROM0", VoiceRom[0].data(), MemTable.Voice->Wait );
	VOICEROM1.SetRom( "VOROM1", VoiceRom[1].data(), MemTable.Voice->Wait );
	
	// 内部RAM
	INTRAM0.SetRam  ( "INRAM0", IntRam[0].data(),   MemTable.IntRam->Wait );
	INTRAM1.SetRam  ( "INRAM1", IntRam[1].data(),   MemTable.IntRam->Wait );
	INTRAM2.SetRam  ( "INRAM2", IntRam[2].data(),   MemTable.IntRam->Wait );
	INTRAM3.SetRam  ( "INRAM3", IntRam[3].data(),   MemTable.IntRam->Wait );
	INTRAM4.SetRam  ( "INRAM4", IntRam[4].data(),   MemTable.IntRam->Wait );
	INTRAM5.SetRam  ( "INRAM5", IntRam[5].data(),   MemTable.IntRam->Wait );
	INTRAM6.SetRam  ( "INRAM6", IntRam[6].data(),   MemTable.IntRam->Wait );
	INTRAM7.SetRam  ( "INRAM7", IntRam[7].data(),   MemTable.IntRam->Wait );
	
	// 内部/外部RAM書込み
	INEXRAM.SetFunc( "IERAM", nullptr, this, nullptr, (WFuncPtr)&MEM62::IERamWrite, MemTable.IntRam->Wait );
	
	return true;
}

bool MEM64::InitSpecific( void )
{
	PRINTD( MEM_LOG, "[MEM][InitSpecific]\n" );
	
	// メモリブロック設定
	// N66-BASIC ROM
	MAINROM0.SetRom ( "SYS1-0", SysRom1[0].data(), MemTable.System1->Wait );
	MAINROM1.SetRom ( "SYS1-1", SysRom1[1].data(), MemTable.System1->Wait );
	MAINROM2.SetRom ( "SYS1-2", SysRom1[2].data(), MemTable.System1->Wait );
	MAINROM3.SetRom ( "SYS1-3", SysRom1[3].data(), MemTable.System1->Wait );
	
	// N66SR-BASIC ROM
	MAINROM4.SetRom ( "SYS1-4", SysRom1[4].data(), MemTable.System1->Wait );
	MAINROM5.SetRom ( "SYS1-5", SysRom1[5].data(), MemTable.System1->Wait );
	MAINROM6.SetRom ( "SYS1-6", SysRom1[6].data(), MemTable.System1->Wait );
	MAINROM7.SetRom ( "SYS1-7", SysRom1[7].data(), MemTable.System1->Wait );
	
	// SR メニューROM
	SRMENROM0.SetRom( "SYS2-0", SysRom2[0].data(), MemTable.System1->Wait );
	SRMENROM1.SetRom( "SYS2-1", SysRom2[1].data(), MemTable.System1->Wait );
	
	// 音声合成ROM
	VOICEROM0.SetRom( "SYS2-2", SysRom2[2].data(), MemTable.System1->Wait );
	VOICEROM1.SetRom( "SYS2-3", SysRom2[3].data(), MemTable.System1->Wait );
	
	// 漢字ROM
	KANJIROM0.SetRom( "SYS2-4", SysRom2[4].data(), MemTable.System1->Wait );
	KANJIROM1.SetRom( "SYS2-5", SysRom2[5].data(), MemTable.System1->Wait );
	KANJIROM2.SetRom( "SYS2-6", SysRom2[6].data(), MemTable.System1->Wait );
	KANJIROM3.SetRom( "SYS2-7", SysRom2[7].data(), MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetRom   ( "CGROM1", CGRom1[0].data(),  MemTable.CGRom1->Wait );
	CGROM2.SetRom   ( "CGROM2", CGRom1[1].data(),  MemTable.CGRom1->Wait );
	
	// 内部RAM
	INTRAM0.SetRam  ( "INRAM0", IntRam[0].data(),  MemTable.IntRam->Wait );
	INTRAM1.SetRam  ( "INRAM1", IntRam[1].data(),  MemTable.IntRam->Wait );
	INTRAM2.SetRam  ( "INRAM2", IntRam[2].data(),  MemTable.IntRam->Wait );
	INTRAM3.SetRam  ( "INRAM3", IntRam[3].data(),  MemTable.IntRam->Wait );
	INTRAM4.SetRam  ( "INRAM4", IntRam[4].data(),  MemTable.IntRam->Wait );
	INTRAM5.SetRam  ( "INRAM5", IntRam[5].data(),  MemTable.IntRam->Wait );
	INTRAM6.SetRam  ( "INRAM6", IntRam[6].data(),  MemTable.IntRam->Wait );
	INTRAM7.SetRam  ( "INRAM7", IntRam[7].data(),  MemTable.IntRam->Wait );
	
	return true;
}


////////////////////////////////////////////////////////////////
// リセット
////////////////////////////////////////////////////////////////
void MEM6::Reset()
{
	PRINTD( MEM_LOG, "[MEM][Reset]\n" );
	
	CGBank = false;	// CG ROM BANK 無効
	
	Rf[0] = INIT_RF0;	// メモリコントローラ内部レジスタ初期値設定
	Rf[1] = INIT_RF1;	// メモリコントローラ内部レジスタ初期値設定
	Rf[2] = INIT_RF2;	// メモリコントローラ内部レジスタ初期値設定
	
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
	
	// 戦士のカートリッジ ------------------------------------------
	if( SolVer ){
		Sol60Mode = false;
		
		// メモリバンク初期化
		BYTE bk[] = { NONBANK,     NONBANK,     ROMBANK | 0, RAMBANK | 3,
					  RAMBANK | 4, RAMBANK | 5, NONBANK,     NONBANK      };
		for( int i=0; i<8; i++ ){
			SetSolBank( i, bk[i] );
		}
	}
	// -------------------------------------------------------------
}

void MEM64::Reset( void )
{
	MEM6::Reset();
	
	// TO DO
	const BYTE initmb[] = { 0xf8, 0xfa, 0xfc, 0xfe, 0x08, 0x0a, 0x0c, 0x0e,
							0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e };
	for( int i=0; i<16; i++ ){
		SetMemBlockSR( i, initmb[i] );
	}
}


////////////////////////////////////////////////////////////////
// フェッチ(M1)
////////////////////////////////////////////////////////////////
BYTE MEM6::Fetch( WORD addr, int* m1wait ) const
{
	BYTE data = vm->VdgIsSRmode() ? Rm_blkSR[addr>>13]->Read( addr )
								  : Rm_blk  [addr>>13]->Read( addr );
	
	PRINTD( MEM_LOG, "[MEM][Fetch] -> %04X:%02X\n", addr, data );
	
	// M1ウェイト追加
	if( m1wait ){
		(*m1wait) += M1Wait;
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( vm->VdgIsBusReqExec() ){
		(*m1wait)++;
	}
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MEM6::Read( WORD addr, int* wcnt ) const
{
	BYTE data = 0xff;
	
	if( vm->VdgIsSRmode() ){
		if( vm->VdgIsSRBitmap( addr ) && (RfSR[addr>>13] == 0) ){	// ビットマップモード(内部RAMアクセス)
			WORD ad = vm->VdgSRGVramAddr( addr );
			data = addr&1 ? (IntRam[ad>>MemBlock::PAGEBITS][ad&MemBlock::PAGEMASK]>>4)&0x0f : IntRam[ad>>MemBlock::PAGEBITS][ad&MemBlock::PAGEMASK]&0x0f;
		}else{														// 直接アクセスモード
			data = Rm_blkSR[addr>>13]->Read( addr, wcnt );
		}
	}else{
		data = Rm_blk[addr>>13]->Read( addr, wcnt );
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){
		(*wcnt)++;
	}
	
	PRINTD( MEM_LOG, "[MEM][Read]  -> %04X:%02X\n", addr, data );
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MEM6::Write( WORD addr, BYTE data, int* wcnt )
{
	PRINTD( MEM_LOG, "[MEM][Write] %04X:%02X -> %s[%04X]'%c'\n", addr, data, vm->VdgIsSRmode() ? Wm_blkSR[addr>>13]->GetName().c_str() : Wm_blk[addr>>13]->GetName().c_str(), addr&0x1fff, data );
	
	if( vm->VdgIsSRmode() ){
		if( vm->VdgIsSRBitmap( addr ) && (RfSR[(addr>>13)+8] == 0) ){	// ビットマップモード(内部RAMアクセス)
			WORD ad = vm->VdgSRGVramAddr( addr );
			IntRam[ad>>MemBlock::PAGEBITS][ad&MemBlock::PAGEMASK] = addr&1 ? ((IntRam[ad>>MemBlock::PAGEBITS][ad&MemBlock::PAGEMASK]&0x0f)|((data<<4)&0xf0)) : ((IntRam[ad>>MemBlock::PAGEBITS][ad&MemBlock::PAGEMASK]&0xf0)|( data    &0x0f));
		}else{															// 直接アクセスモード
			Wm_blkSR[addr>>13]->Write( addr, data, wcnt );
		}
	}else{
		Wm_blk[addr>>13]->Write( addr, data, wcnt );
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ) (*wcnt)++;
	
	// 内部/外部RAMとも書込みの場合はひとまず内部だけ
}


////////////////////////////////////////////////////////////////
// メモリアクセスウェイト設定
////////////////////////////////////////////////////////////////
void MEM6::SetWait( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetWait] -> M1:%d ROM:%d RAM:%d\n", (data>>7)&1, (data>>6)&1, (data>>5)&1 );
	
	// M1
	M1Wait = data&0x80 ? 1 : 0;
	
	// ROM
	int romwait = data&0x40 ? 1 : 0;
	EMPTYROM.SetWait ( romwait );
	EXTROM0.SetWait  ( romwait );
	EXTROM1.SetWait  ( romwait );
	MAINROM0.SetWait ( romwait );
	MAINROM1.SetWait ( romwait );
	MAINROM2.SetWait ( romwait );
	MAINROM3.SetWait ( romwait );
	MAINROM4.SetWait ( romwait );	// SR
	MAINROM5.SetWait ( romwait );	// SR
	MAINROM6.SetWait ( romwait );	// SR
	MAINROM7.SetWait ( romwait );	// SR
	KANJIROM0.SetWait( romwait );
	KANJIROM1.SetWait( romwait );
	KANJIROM2.SetWait( romwait );
	KANJIROM3.SetWait( romwait );
	VOICEROM0.SetWait( romwait );
	VOICEROM1.SetWait( romwait );
	SRMENROM0.SetWait( romwait );	// SR
	SRMENROM1.SetWait( romwait );	// SR
	
	// RAM
	int ramwait = data&0x20 ? 1 : 0;
	EMPTYRAM.SetWait( ramwait );
	INTRAM0.SetWait ( ramwait );
	INTRAM1.SetWait ( ramwait );
	INTRAM2.SetWait ( ramwait );
	INTRAM3.SetWait ( ramwait );
	INTRAM4.SetWait ( ramwait );
	INTRAM5.SetWait ( ramwait );
	INTRAM6.SetWait ( ramwait );
	INTRAM7.SetWait ( ramwait );
	EXTRAM0.SetWait ( ramwait );
	EXTRAM1.SetWait ( ramwait );
	EXTRAM2.SetWait ( ramwait );
	EXTRAM3.SetWait ( ramwait );
	EXTRAM4.SetWait ( ramwait );
	EXTRAM5.SetWait ( ramwait );
	EXTRAM6.SetWait ( ramwait );
	EXTRAM7.SetWait ( ramwait );
}


////////////////////////////////////////////////////////////////
// メモリアクセスウェイト取得
////////////////////////////////////////////////////////////////
BYTE MEM6::GetWait( void ) const
{
	return ( M1Wait ? 0x80 : 0 ) | ( EMPTYROM.GetWait() ? 0x40 : 0 ) | ( EMPTYRAM.GetWait() ? 0x20 : 0 );
}


////////////////////////////////////////////////////////////////
// メモリリード時のメモリブロック指定
////////////////////////////////////////////////////////////////
void MEM60::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR]\n" );
	
	// Port F0H
	switch( mem1 & 0x0f ){	// RF0下位 (0000 - 3FFF)
		case 0x01:
		case 0x02:
		case 0x05:
		case 0x06:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = &MAINROM1;	break;
		default:	Rm_blk[0] = rMAINROM0;	Rm_blk[1] = rMAINROM1;	// 戦士のカートリッジ対応
	}
	
	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = &EXTROM1;
	Rm_blk[4] = pEXTRAM0;	Rm_blk[5] = pEXTRAM1;
	Rm_blk[6] = &INTRAM0;	Rm_blk[7] = &INTRAM1;
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, Rm_blk[i]->GetName().c_str(), i+1, Rm_blk[i+1]->GetName().c_str() );
	}
	#endif
}

void MEM62::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR] -> %02X %02X\n", mem1, mem2 );
	
	// Port F0H
	switch( mem1 & 0x0f ){	// RF0下位 (0000 - 3FFF)
		case 0x00:	Rm_blk[0] = &EMPTYROM;	Rm_blk[1] = &EMPTYROM;	break;
		case 0x01:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = &MAINROM1;	break;
		case 0x02:	Rm_blk[0] = pKNJROM2;	Rm_blk[1] = pKNJROM3;	break;
		case 0x03:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &EXTROM1;	break;
		case 0x04:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = &EXTROM0;	break;
		case 0x05:	Rm_blk[0] = pKNJROM2;	Rm_blk[1] = &MAINROM1;	break;
		case 0x06:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = pKNJROM3;	break;
		case 0x07:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = &EXTROM1;	break;
		case 0x08:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &EXTROM0;	break;
		case 0x09:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &MAINROM1;	break;
		case 0x0a:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = &EXTROM1;	break;
		case 0x0b:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = pKNJROM3;	break;
		case 0x0c:	Rm_blk[0] = pKNJROM2;	Rm_blk[1] = &EXTROM0;	break;
		case 0x0d:	Rm_blk[0] = &INTRAM0;	Rm_blk[1] = &INTRAM1;	break;
		case 0x0e:	Rm_blk[0] = &EXTRAM0;	Rm_blk[1] = &EXTRAM1;	break;
		case 0x0f:	Rm_blk[0] = &EMPTYROM;	Rm_blk[1] = &EMPTYROM;	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	Rm_blk[2] = &EMPTYROM;	Rm_blk[3] = &EMPTYROM;	break;
		case 0x10:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = &MAINROM3;	break;
		case 0x20:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = pKNJROM3;	break;
		case 0x30:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &EXTROM1;	break;
		case 0x40:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = &EXTROM0;	break;
		case 0x50:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = &MAINROM3;	break;
		case 0x60:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = pKNJROM3;	break;
		case 0x70:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = &EXTROM1;	break;
		case 0x80:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &EXTROM0;	break;
		case 0x90:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &MAINROM3;	break;
		case 0xa0:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = &EXTROM1;	break;
		case 0xb0:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = pKNJROM3;	break;
		case 0xc0:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = &EXTROM0;	break;
		case 0xd0:	Rm_blk[2] = &INTRAM2;	Rm_blk[3] = &INTRAM3;	break;
		case 0xe0:	Rm_blk[2] = &EXTRAM2;	Rm_blk[3] = &EXTRAM3;	break;
		case 0xf0:	Rm_blk[2] = &EMPTYROM;	Rm_blk[3] = &EMPTYROM;	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	Rm_blk[4] = &EMPTYROM;	Rm_blk[5] = &EMPTYROM;	break;
		case 0x01:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = &MAINROM1;	break;
		case 0x02:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = pKNJROM3;	break;
		case 0x03:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &EXTROM1;	break;
		case 0x04:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = &EXTROM0;	break;
		case 0x05:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = &MAINROM1;	break;
		case 0x06:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = pKNJROM3;	break;
		case 0x07:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = &EXTROM1;	break;
		case 0x08:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &EXTROM0;	break;
		case 0x09:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &MAINROM1;	break;
		case 0x0a:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = &EXTROM1;	break;
		case 0x0b:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = pKNJROM3;	break;
		case 0x0c:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = &EXTROM0;	break;
		case 0x0d:	Rm_blk[4] = &INTRAM4;	Rm_blk[5] = &INTRAM5;	break;
		case 0x0e:	Rm_blk[4] = &EXTRAM4;	Rm_blk[5] = &EXTRAM5;	break;
		case 0x0f:	Rm_blk[4] = &EMPTYROM;	Rm_blk[5] = &EMPTYROM;	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	Rm_blk[6] = &EMPTYROM;	Rm_blk[7] = &EMPTYROM;	break;
		case 0x10:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = &MAINROM3;	break;
		case 0x20:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = pKNJROM3;	break;
		case 0x30:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &EXTROM1;	break;
		case 0x40:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = &EXTROM0;	break;
		case 0x50:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = &MAINROM3;	break;
		case 0x60:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = pKNJROM3;	break;
		case 0x70:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = &EXTROM1;	break;
		case 0x80:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &EXTROM0;	break;
		case 0x90:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &MAINROM3;	break;
		case 0xa0:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = &EXTROM1;	break;
		case 0xb0:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = pKNJROM3;	break;
		case 0xc0:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = &EXTROM0;	break;
		case 0xd0:	Rm_blk[6] = &INTRAM6;	Rm_blk[7] = &INTRAM7;	break;
		case 0xe0:	Rm_blk[6] = &EXTRAM6;	Rm_blk[7] = &EXTRAM7;	break;
		case 0xf0:	Rm_blk[6] = &EMPTYROM;	Rm_blk[7] = &EMPTYROM;	break;
	}
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	if( CGBank && cgenable ){
		for( int i=0; i<8; i++ ){
			if( ((cgaden & i) | (~cgaden & cgaddr)) == i )
				Rm_blk[i] = cgrom ? &CGROM1 : &CGROM2;
		}
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, Rm_blk[i]->GetName().c_str(), i+1, Rm_blk[i+1]->GetName().c_str() );
	}
	#endif
}

void MEM64::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR] -> %02X %02X\n", mem1, mem2 );
	
	// Port F0H
	switch( mem1 & 0x0f ){	// RF0下位 (0000 - 3FFF)
		// SRの場合，この領域に音声合成ROMを選択するとSYSROM2の先頭16KBが割当てられる
		// mk2,66の場合は音声合成ROMが16KBしかないので
		// 4000-7FFFHは0000-3FFFHのイメージと考えればよいようだ
		case 0x00:	Rm_blk[0] = &EMPTYROM;	Rm_blk[1] = &EMPTYROM;	break;
		case 0x01:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = &MAINROM1;	break;
		case 0x02:	Rm_blk[0] = pKNJROM0;	Rm_blk[1] = pKNJROM1;	break;	// ココ
		case 0x03:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &EXTROM1;	break;
		case 0x04:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = &EXTROM0;	break;
		case 0x05:	Rm_blk[0] = pKNJROM0;	Rm_blk[1] = &MAINROM1;	break;	// ココ
		case 0x06:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = pKNJROM1;	break;	// ココ
		case 0x07:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = &EXTROM1;	break;
		case 0x08:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &EXTROM0;	break;
		case 0x09:	Rm_blk[0] = &EXTROM1;	Rm_blk[1] = &MAINROM1;	break;
		case 0x0a:	Rm_blk[0] = &MAINROM0;	Rm_blk[1] = &EXTROM1;	break;
		case 0x0b:	Rm_blk[0] = &EXTROM0;	Rm_blk[1] = pKNJROM1;	break;	// ココ
		case 0x0c:	Rm_blk[0] = pKNJROM0;	Rm_blk[1] = &EXTROM0;	break;	// ココ
		case 0x0d:	Rm_blk[0] = &INTRAM0;	Rm_blk[1] = &INTRAM1;	break;
		case 0x0e:	Rm_blk[0] = &EXTRAM0;	Rm_blk[1] = &EXTRAM1;	break;
		case 0x0f:	Rm_blk[0] = &EMPTYROM;	Rm_blk[1] = &EMPTYROM;	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	Rm_blk[2] = &EMPTYROM;	Rm_blk[3] = &EMPTYROM;	break;
		case 0x10:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = &MAINROM3;	break;
		case 0x20:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = pKNJROM3;	break;
		case 0x30:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &EXTROM1;	break;
		case 0x40:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = &EXTROM0;	break;
		case 0x50:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = &MAINROM3;	break;
		case 0x60:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = pKNJROM3;	break;
		case 0x70:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = &EXTROM1;	break;
		case 0x80:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &EXTROM0;	break;
		case 0x90:	Rm_blk[2] = &EXTROM1;	Rm_blk[3] = &MAINROM3;	break;
		case 0xa0:	Rm_blk[2] = &MAINROM2;	Rm_blk[3] = &EXTROM1;	break;
		case 0xb0:	Rm_blk[2] = &EXTROM0;	Rm_blk[3] = pKNJROM3;	break;
		case 0xc0:	Rm_blk[2] = pKNJROM2;	Rm_blk[3] = &EXTROM0;	break;
		case 0xd0:	Rm_blk[2] = &INTRAM2;	Rm_blk[3] = &INTRAM3;	break;
		case 0xe0:	Rm_blk[2] = &EXTRAM2;	Rm_blk[3] = &EXTRAM3;	break;
		case 0xf0:	Rm_blk[2] = &EMPTYROM;	Rm_blk[3] = &EMPTYROM;	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	Rm_blk[4] = &EMPTYROM;	Rm_blk[5] = &EMPTYROM;	break;
		case 0x01:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = &MAINROM1;	break;
		case 0x02:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = pKNJROM3;	break;
		case 0x03:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &EXTROM1;	break;
		case 0x04:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = &EXTROM0;	break;
		case 0x05:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = &MAINROM1;	break;
		case 0x06:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = pKNJROM3;	break;
		case 0x07:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = &EXTROM1;	break;
		case 0x08:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &EXTROM0;	break;
		case 0x09:	Rm_blk[4] = &EXTROM1;	Rm_blk[5] = &MAINROM1;	break;
		case 0x0a:	Rm_blk[4] = &MAINROM0;	Rm_blk[5] = &EXTROM1;	break;
		case 0x0b:	Rm_blk[4] = &EXTROM0;	Rm_blk[5] = pKNJROM3;	break;
		case 0x0c:	Rm_blk[4] = pKNJROM2;	Rm_blk[5] = &EXTROM0;	break;
		case 0x0d:	Rm_blk[4] = &INTRAM4;	Rm_blk[5] = &INTRAM5;	break;
		case 0x0e:	Rm_blk[4] = &EXTRAM4;	Rm_blk[5] = &EXTRAM5;	break;
		case 0x0f:	Rm_blk[4] = &EMPTYROM;	Rm_blk[5] = &EMPTYROM;	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	Rm_blk[6] = &EMPTYROM;	Rm_blk[7] = &EMPTYROM;	break;
		case 0x10:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = &MAINROM3;	break;
		case 0x20:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = pKNJROM3;	break;
		case 0x30:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &EXTROM1;	break;
		case 0x40:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = &EXTROM0;	break;
		case 0x50:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = &MAINROM3;	break;
		case 0x60:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = pKNJROM3;	break;
		case 0x70:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = &EXTROM1;	break;
		case 0x80:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &EXTROM0;	break;
		case 0x90:	Rm_blk[6] = &EXTROM1;	Rm_blk[7] = &MAINROM3;	break;
		case 0xa0:	Rm_blk[6] = &MAINROM2;	Rm_blk[7] = &EXTROM1;	break;
		case 0xb0:	Rm_blk[6] = &EXTROM0;	Rm_blk[7] = pKNJROM3;	break;
		case 0xc0:	Rm_blk[6] = pKNJROM2;	Rm_blk[7] = &EXTROM0;	break;
		case 0xd0:	Rm_blk[6] = &INTRAM6;	Rm_blk[7] = &INTRAM7;	break;
		case 0xe0:	Rm_blk[6] = &EXTRAM6;	Rm_blk[7] = &EXTRAM7;	break;
		case 0xf0:	Rm_blk[6] = &EMPTYROM;	Rm_blk[7] = &EMPTYROM;	break;
	}
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	// (推定)SRはA13を無視して16KB単位でCG ROMが現れるようだ
	if( CGBank && cgenable ){
		for( int i=0; i<8; i++ ){
//			if( ((cgaden & i) | (~cgaden & cgaddr)) == i )
			if( (((cgaden|1) & i) | (~cgaden & cgaddr)) == i )
				Rm_blk[i] = cgrom ? &CGROM1 : &CGROM2;
		}
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, Rm_blk[i]->GetName().c_str(), i+1, Rm_blk[i+1]->GetName().c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// メモリライト時のメモリブロック指定
////////////////////////////////////////////////////////////////
void MEM60::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW]\n" );
	
	switch( data & 3 ){	// 0000 - 3FFF
		case 0:	Wm_blk[0] = &EMPTYROM;	Wm_blk[1] = &EMPTYROM;	break;
		case 1:
		case 2:
		case 3:	Wm_blk[0] = wMAINROM0;	Wm_blk[1] = wMAINROM1;	break;	// 戦士のカートリッジ対応
	}
	
	Wm_blk[2] = wEXTROM0;	Wm_blk[3] = wEXTROM1;	// 戦士のカートリッジ対応
	Wm_blk[4] = pEXTRAM0;	Wm_blk[5] = pEXTRAM1;
	Wm_blk[6] = &INTRAM0;	Wm_blk[7] = &INTRAM1;
	
	// 内部レジスタ保存
	Rf[2] = data;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, Wm_blk[i]->GetName().c_str(), i+1, Wm_blk[i+1]->GetName().c_str() );
	}
	#endif
}

void MEM62::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW] -> %02X\n", data );
	
	switch( data & 3 ){			// 0000 - 3FFF
		case 0: Wm_blk[0] = &EMPTYRAM;	Wm_blk[1] = &EMPTYRAM;	break;
		case 1: Wm_blk[0] = &INTRAM0;	Wm_blk[1] = &INTRAM1;	break;
		case 2: Wm_blk[0] = &EXTRAM0;	Wm_blk[1] = &EXTRAM1;	break;
		case 3: Wm_blk[0] = &INEXRAM;	Wm_blk[1] = &INEXRAM;	break;
	}
	switch( (data>>2) & 3 ){	// 4000 - 7FFF
		case 0: Wm_blk[2] = &EMPTYRAM;	Wm_blk[3] = &EMPTYRAM;	break;
		case 1: Wm_blk[2] = &INTRAM2;	Wm_blk[3] = &INTRAM3;	break;
		case 2: Wm_blk[2] = &EXTRAM2;	Wm_blk[3] = &EXTRAM3;	break;
		case 3: Wm_blk[2] = &INEXRAM;	Wm_blk[3] = &INEXRAM;	break;
	}
	switch( (data>>4) & 3 ){	// 8000 - BFFF
		case 0: Wm_blk[4] = &EMPTYRAM;	Wm_blk[5] = &EMPTYRAM;	break;
		case 1: Wm_blk[4] = &INTRAM4;	Wm_blk[5] = &INTRAM5;	break;
		case 2: Wm_blk[4] = &EXTRAM4;	Wm_blk[5] = &EXTRAM5;	break;
		case 3: Wm_blk[4] = &INEXRAM;	Wm_blk[5] = &INEXRAM;	break;
	}
	switch( (data>>6) & 3 ){	// C000 - FFFF
		case 0: Wm_blk[6] = &EMPTYRAM;	Wm_blk[7] = &EMPTYRAM;	break;
		case 1: Wm_blk[6] = &INTRAM6;	Wm_blk[7] = &INTRAM7;	break;
		case 2: Wm_blk[6] = &EXTRAM6;	Wm_blk[7] = &EXTRAM7;	break;
		case 3: Wm_blk[6] = &INEXRAM;	Wm_blk[7] = &INEXRAM;	break;
	}
	
	// 内部レジスタ保存
	Rf[2] = data;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, Wm_blk[i]->GetName().c_str(), i+1, Wm_blk[i+1]->GetName().c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// メモリリード/ライト時のメモリブロック指定(64,68)
////////////////////////////////////////////////////////////////
void MEM64::SetMemBlockSR( BYTE port, BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockSR] -> Port:%02X Data:%02X\n", port, data );
	
	BYTE cs   = data>>4;
	BYTE addr = (data>>1)&0x07;
	MemBlock** mb;
	
	RfSR[port&0x0f] = data;
	if( port&0x08 ) mb = &Wm_blkSR[port&0x07];	// 8-F : Write
	else			mb = &Rm_blkSR[port&0x07];	// 0-7 : Read
	
	switch( cs ){
	case 0x00:	// System RAM (16KB毎)
		switch( addr ){
		case 0x00: *mb = port&1 ? &INTRAM1 : &INTRAM0; break;
		case 0x01: *mb = port&1 ? &INTRAM1 : &INTRAM0; break;
		case 0x02: *mb = port&1 ? &INTRAM3 : &INTRAM2; break;
		case 0x03: *mb = port&1 ? &INTRAM3 : &INTRAM2; break;
		case 0x04: *mb = port&1 ? &INTRAM5 : &INTRAM4; break;
		case 0x05: *mb = port&1 ? &INTRAM5 : &INTRAM4; break;
		case 0x06: *mb = port&1 ? &INTRAM7 : &INTRAM6; break;
		case 0x07: *mb = port&1 ? &INTRAM7 : &INTRAM6; break;
		}
		break;
		
	case 0x02:	// Ext RAM (こっちも16KB毎とすべき?)
		switch( addr ){
		case 0x00: *mb = &EXTRAM0; break;
		case 0x01: *mb = &EXTRAM1; break;
		case 0x02: *mb = &EXTRAM2; break;
		case 0x03: *mb = &EXTRAM3; break;
		case 0x04: *mb = &EXTRAM4; break;
		case 0x05: *mb = &EXTRAM5; break;
		case 0x06: *mb = &EXTRAM6; break;
		case 0x07: *mb = &EXTRAM7; break;
		}
		break;
		
	case 0x0b:	// Ext ROM1
		*mb = &EXTROM1;
		break;
		
	case 0x0c:	// Ext ROM2
		*mb = &EXTROM0;
		break;
		
	case 0x0d:	// CGROM
		switch( addr&0x01 ){
		case 0x00: *mb = &CGROM1; break;
		case 0x01: *mb = &CGROM2; break;
		}
		break;
		
	case 0x0e:	// System Rom2
		switch( addr ){
		case 0x00: *mb = &SRMENROM0; break;
		case 0x01: *mb = &SRMENROM1; break;
		case 0x02: *mb = &VOICEROM0; break;
		case 0x03: *mb = &VOICEROM1; break;
		case 0x04: *mb = &KANJIROM0; break;
		case 0x05: *mb = &KANJIROM1; break;
		case 0x06: *mb = &KANJIROM2; break;
		case 0x07: *mb = &KANJIROM3; break;
		}
		break;
		
	case 0x0f:	// System Rom1
		switch( addr ){
		case 0x00: *mb = &MAINROM0; break;
		case 0x01: *mb = &MAINROM1; break;
		case 0x02: *mb = &MAINROM2; break;
		case 0x03: *mb = &MAINROM3; break;
		case 0x04: *mb = &MAINROM4; break;
		case 0x05: *mb = &MAINROM5; break;
		case 0x06: *mb = &MAINROM6; break;
		case 0x07: *mb = &MAINROM7; break;
		}
		break;
		
	default:
		*mb = &EMPTYROM;
	}
	
	#if (MEM_LOG)
	PRINTD( MEM_LOG, "              [Read]\t\t[Write]\n" );
	for( int i=0; i<8; i++ ){
		PRINTD( MEM_LOG, "               %d:%8s\t%8s\n", i, Rm_blkSR[i]->GetName().c_str(), Wm_blkSR[i]->GetName().c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// CG ROM アドレス等設定(62,66)
////////////////////////////////////////////////////////////////
void MEM6::SetCGrom( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetCGrom] -> %02x\n", data );
	
	// bit 7
	int	cgwait = data&0x80 ? 0 : 1;
	CGROM1.SetWait( cgwait );
	CGROM2.SetWait( cgwait );
	
	// bit 6 CG ROMアクセスフラグ true:アクセス可 false:アクセス不可
	cgenable = data&0x40 ? true : false;
	
	// bit 5,4,3 CG ROMアドレスイネーブル
	cgaden   = (data>>3)&7;
	
	// bit 2,1,0 CG ROMアドレス A13,14,15
	cgaddr   = data&7;
	
	SetMemBlockR( Rf[0], Rf[1] );
}


////////////////////////////////////////////////////////////////
// CG ROM 選択(62,66)
////////////////////////////////////////////////////////////////
void MEM6::SelectCGrom( int mode )
{
	PRINTD( MEM_LOG, "[MEM][SelectCGrom] -> %d\n", mode );
	
	// mode 1:32*16(N60モード) 0:40*20(N60mモード)
	cgrom = mode ? true : false;
}


////////////////////////////////////////////////////////////////
// CG ROM BANK を切り替える
////////////////////////////////////////////////////////////////
void MEM6::SetCGBank( bool data )
{
	PRINTD( MEM_LOG, "[MEM][SetCGBank] -> %d\n", data );
	
	CGBank = data;
	Rm_blk[3] = CGBank ? &CGROM1 : &EXTROM1;
}

void MEM62::SetCGBank( bool data )
{
	PRINTD( MEM_LOG, "[MEM][SetCGBank] -> %d\n", data );
	
	CGBank = data;
	SetMemBlockR( Rf[0], Rf[1] );
}


////////////////////////////////////////////////////////////////
// 漢字ROMおよび音声合成ROM設定(62,66)
////////////////////////////////////////////////////////////////
void MEM6::SetKanjiRom( BYTE mode )
{
	PRINTD( MEM_LOG, "[MEM][SetKanjiRom] -> %02X\n", mode );
	
	// mode bit0 0:音声合成ROM選択 1:漢字ROM選択
	//      bit1 0:漢字ROM左側     1:漢字ROM右側
	if( c2acc&2 ) kj_LR  = mode&2 ? true : false;	// 漢字 左？右？
	if( c2acc&1 ) kj_rom = mode&1 ? true : false;	// 漢字ROM？音声合成ROM？
	SetMemBlockR( Rf[0], Rf[1] );
}


////////////////////////////////////////////////////////////////
// 漢字ROMおよび音声合成ROM取得(62,66)
////////////////////////////////////////////////////////////////
BYTE MEM6::GetKanjiRom( void ) const
{
	return 0xfc | kj_LR ? 2 : 0 | kj_rom ? 1 : 0;
}


////////////////////////////////////////////////////////////////
// PortC2Hアクセス設定(62,66)
////////////////////////////////////////////////////////////////
void MEM6::SetPortC2HAccess( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetPortC2HAccess] -> %02X\n", data );
	
	// 0:入力 1:出力
	c2acc = data;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジ メモリバンクレジスタ設定
//
//  [ROMの場合]
//    bit7   : 0
//    bit6   : 0
//    bit5-4 : Bank Set No. (0-3)
//    bit0-3 : Bank No. (0-15)
//  [RAMの場合]
//    bit7   : 0
//    bit6   : 1
//    bit4-5 : 未使用
//    bit0-3 : Bank No. (0-15)
//  [SCCの場合]
//    bit7   : 1
//    bit6   : 0
//    bit3-5 : 未使用
//    bit2   : 必ず1にする(AA15に出力される)
//    bit1   : 必ず0にする(AA14に出力される)
//    bit0   : 0:互換モード 1:SCC-Iモード
//  [無効の場合]
//    bit7   : 1
//    bit6   : 1
//    bit0-5 : 未使用
////////////////////////////////////////////////////////////////
void MEM6::SetSolBank( BYTE port, BYTE data )
{
	int area     = port & 7;
	MemBlock* mb = &EXTRAM0;
	
	SolBank[area] = data;
	
	switch( data & 0xc0 ){
	case ROMBANK:	// ROM
		SolBankSet = data & 0x30;
		mb[area].SetFunc( Stringf( "EROM%02d", data ), ExtRom[data].data(), this, (RFuncPtr)&MEM6::SolMemRead, nullptr );
		
		// Bank Setが変わった場合の修正
		for( int i=0; i<8; i++ ){
			if( ((SolBank[i] & 0xc0) == ROMBANK) && ((SolBank[i] & 0x30) != SolBankSet) ){
				SolBank[i] = (SolBank[i] & 0x0f) | SolBankSet | ROMBANK;
				mb[i].SetFunc( nullptr, ExtRom[SolBank[i] & 0x3f].data(), this, (RFuncPtr)&MEM6::SolMemRead, nullptr );
			}
		}
		break;
		
	case RAMBANK:	// RAM
		mb[area].SetFunc( Stringf( "ERAM%02d", data & 0x0f ), ExtRam[data & 0x0f].data(), this, (RFuncPtr)&MEM6::SolMemRead, (WFuncPtr)&MEM6::SolMemWrite );
		break;
		
	case SCCBANK:	// SCC
		mb[area].SetFunc( "SCC", nullptr, this, (RFuncPtr)&MEM6::SolSccRead, (WFuncPtr)&MEM6::SolSccWrite );
		break;
		
	case NONBANK:	// 無効
	default:
		mb[area].SetFunc( "EMPTY", nullptr, nullptr, nullptr, nullptr );
	}
}


////////////////////////////////////////////////////////////////
// 直接アクセス関数
////////////////////////////////////////////////////////////////
BYTE MEM6::ReadMainRom  ( WORD addr ) const { return SysRom1 [(  addr                &(MemTable.System1->Size-1))>>MemBlock::PAGEBITS][  addr                &MemBlock::PAGEMASK]; }
BYTE MEM6::ReadIntRam   ( WORD addr ) const { return IntRam  [(  addr                &(MemTable.IntRam ->Size-1))>>MemBlock::PAGEBITS][  addr                &MemBlock::PAGEMASK]; }
BYTE MEM6::ReadExtRom   ( WORD addr ) const { return ExtRom.size() ? ExtRom[(addr&(MemTable.ExtRom->Size-1))>>MemBlock::PAGEBITS][addr&MemBlock::PAGEMASK] : 0xff; }
BYTE MEM6::ReadExtRam   ( WORD addr ) const { return ExtRam  [(  addr                &(MemTable.ExtRam ->Size-1))>>MemBlock::PAGEBITS][  addr                &MemBlock::PAGEMASK]; }

BYTE MEM6::ReadCGrom1   ( WORD addr ) const { return CGRom1  [( (addr&0x1fff)        &(MemTable.CGRom1 ->Size-1))>>MemBlock::PAGEBITS][ (addr&0x1fff)        &MemBlock::PAGEMASK]; }
BYTE MEM6::ReadCGrom2   ( WORD )      const { return 0xff; }
BYTE MEM6::ReadCGrom3   ( WORD )      const { return 0xff; }

BYTE MEM62::ReadCGrom2  ( WORD addr ) const { return CGRom2  [( (addr&0x1fff)        &(MemTable.CGRom2 ->Size-1))>>MemBlock::PAGEBITS][ (addr&0x1fff)        &MemBlock::PAGEMASK]; }
BYTE MEM62::ReadKanjiRom( WORD addr ) const { return KanjiRom[( (addr&0x7fff)        &(MemTable.Kanji  ->Size-1))>>MemBlock::PAGEBITS][ (addr&0x7fff)        &MemBlock::PAGEMASK]; }
BYTE MEM62::ReadVoiceRom( WORD addr ) const { return VoiceRom[( (addr&0x3fff)        &(MemTable.Voice  ->Size-1))>>MemBlock::PAGEBITS][ (addr&0x3fff)        &MemBlock::PAGEMASK]; }

BYTE MEM64::ReadCGrom1  ( WORD addr ) const { return CGRom1  [( (addr&0x0fff)        &(MemTable.CGRom1 ->Size-1))>>MemBlock::PAGEBITS][ (addr&0x0fff)        &MemBlock::PAGEMASK]; }
BYTE MEM64::ReadCGrom2  ( WORD addr ) const { return CGRom1  [(((addr&0x1fff)+0x2000)&(MemTable.CGRom1 ->Size-1))>>MemBlock::PAGEBITS][((addr&0x1fff)+0x2000)&MemBlock::PAGEMASK]; }
BYTE MEM64::ReadCGrom3  ( WORD addr ) const { return CGRom1  [(((addr&0x0fff)+0x1000)&(MemTable.CGRom1 ->Size-1))>>MemBlock::PAGEBITS][((addr&0x0fff)+0x1000)&MemBlock::PAGEMASK]; }
BYTE MEM64::ReadKanjiRom( WORD addr ) const { return SysRom2 [(((addr&0x7fff)+0x8000)&(MemTable.System2->Size-1))>>MemBlock::PAGEBITS][((addr&0x7fff)+0x8000)&MemBlock::PAGEMASK]; }
BYTE MEM64::ReadVoiceRom( WORD addr ) const { return SysRom2 [(((addr&0x3fff)+0x4000)&(MemTable.System2->Size-1))>>MemBlock::PAGEBITS][((addr&0x3fff)+0x4000)&MemBlock::PAGEMASK]; }


////////////////////////////////////////////////////////////////
// I/Oアクセス関数
////////////////////////////////////////////////////////////////
void MEM6::Out06H( int, BYTE data ){ Sol60Mode = (data == 0x66) ? true : false; }
void MEM60::Out06H( int, BYTE data )
{
	Sol60Mode = (data == 0x66) ? true : false;
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
}

void MEM6::Out3xH( int port, BYTE data ){ SetSolBank( port, data ); }
void MEM6::Out7FH( int, BYTE data ){ SetSolBank( 2, ROMBANK | (data & 0xf) ); }

void MEM6::OutC1H( int, BYTE data ){ SelectCGrom( (data>>1)&1 ); }
void MEM6::OutC2H( int, BYTE data ){ SetKanjiRom( data ); }
void MEM6::OutC3H( int, BYTE data ){ SetPortC2HAccess( data ); }
void MEM6::OutF0H( int, BYTE data ){ SetMemBlockR( data, Rf[1] ); }
void MEM6::OutF1H( int, BYTE data ){ SetMemBlockR( Rf[0], data ); }
void MEM6::OutF2H( int, BYTE data ){ SetMemBlockW( data ); }
void MEM6::OutF3H( int, BYTE data ){ SetWait( data ); }
void MEM6::OutF8H( int, BYTE data ){ SetCGrom( data ); }

void MEM64::Out6xH( int port, BYTE data ){ SetMemBlockSR( port, data ); }

BYTE MEM6::InC2H( int ){ return GetKanjiRom(); }
BYTE MEM6::InF0H( int ){ return Rf[0]; }
BYTE MEM6::InF1H( int ){ return Rf[1]; }
BYTE MEM6::InF2H( int ){ return Rf[2]; }
BYTE MEM6::InF3H( int ){ return GetWait() | 0x1f; }

BYTE MEM64::In6xH( int port ){ return RfSR[port&0x0f]; }
BYTE MEM64::InB2H( int port ){ return 0xfd; }	// bit1 0:mk2SR 1:66SR
BYTE MEM68::InB2H( int port ){ return 0xff; }	// bit1 0:mk2SR 1:66SR


////////////////////////////////////////////////////////////////
// どこでもSAVE
//
// 引数:	Ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::DokoSave( cIni* Ini )
{
	PRINTD( MEM_LOG, "[MEM][DokoSave]\n" );
	
	if( !Ini ) return false;
	
	Ini->SetVal( "MEMORY", "CGBank",		"", CGBank );
	Ini->SetVal( "MEMORY", "UseExtRam",		"", UseExtRam );
	Ini->SetVal( "MEMORY", "M1Wait",		"", M1Wait );
	Ini->SetVal( "MEMORY", "Soldier",		"", SolVer );
	Ini->SetVal( "MEMORY", "Soldier60",		"", Sol60Mode );
	Ini->SetVal( "MEMORY", "SoldierBank",	"", SolBankSet );
	
	// 62,66,64,68
	Ini->SetVal( "MEMORY", "cgrom",			"", cgrom    );
	Ini->SetVal( "MEMORY", "kj_rom",		"", kj_rom   );
	Ini->SetVal( "MEMORY", "kj_LR",			"", kj_LR    );
	Ini->SetVal( "MEMORY", "cgenable",		"", cgenable );
	Ini->SetVal( "MEMORY", "cgaden",		"", cgaden );
	Ini->SetVal( "MEMORY", "cgaddr",		"", cgaddr );
	Ini->SetVal( "MEMORY", "Rf0",	 		"", "0x%02X", Rf[0] );
	Ini->SetVal( "MEMORY", "Rf1",	 		"", "0x%02X", Rf[1] );
	Ini->SetVal( "MEMORY", "Rf2",	 		"", "0x%02X", Rf[2] );
	
	// 拡張ROMがマウントされている場合
	if( UseExtRom ){
		P6VPATH tpath = FilePath;
		OSD_RelativePath( tpath );
		Ini->SetVal( "MEMORY", "FilePath",	"", tpath );
	}
	
	// メモリウェイト
	Ini->SetVal( "MEMORY", "Wait",		"", GetWait() );
	// CGRomウェイト
	Ini->SetVal( "MEMORY", "CgRomWait",	"", CGROM1.GetWait() );
	
	// 内部RAM
	for( int i=0; i<(int)MemTable.IntRam->Size; i+=64 ){
		std::string strva;
		for( int j=0; j<64; j++ ){
			strva += Stringf( "%02X", IntRam[(i+j)>>MemBlock::PAGEBITS][(i+j)&MemBlock::PAGEMASK] );
		}
		Ini->SetEntry( "MEMORY", Stringf( "IntRam_%04X", i ), "", strva.c_str() );
	}
	
	// 外部RAM
	if( UseExtRam || SolVer ){
		for( int i=0; i<(int)MemTable.ExtRam->Size; i+=64 ){
			std::string strva;
			for( int j=0; j<64; j++ ){
				strva += Stringf( "%02X", ExtRam[(i+j)>>MemBlock::PAGEBITS][(i+j)&MemBlock::PAGEMASK] );
			}
			Ini->SetEntry( "MEMORY", Stringf( "ExtRam_%06X", i ), "", strva.c_str() );
		}
	}
	
	// 戦士のカートリッジ
	if( SolVer ){
		// メモリバンクレジスタ
		for( int i=0; i<8; i++ ){
			Ini->SetVal( "MEMORY", Stringf( "SolBank%d", i ), "", "0x%02X", SolBank[i] );
		}
	}
	
	return true;
}

bool MEM64::DokoSave( cIni* Ini )
{
	if( !MEM6::DokoSave( Ini ) ) return false;
	
	for( int i=0; i<16; i++ ){
		Ini->SetVal( "MEMORY", Stringf( "RfSR_%02d", i ), "", "0x%02X", RfSR[i] );
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOAD
//
// 引数:	Ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::DokoLoad( cIni* Ini )
{
	P6VPATH tpath = "";
	int st;
	
	PRINTD( MEM_LOG, "[MEM][DokoLoad]\n" );
	
	if( !Ini ) return false;
	
	Ini->GetVal( "MEMORY", "CGBank",		CGBank     );
	Ini->GetVal( "MEMORY", "UseExtRam",		UseExtRam  );
	Ini->GetVal( "MEMORY", "M1Wait",		M1Wait     );
	Ini->GetVal( "MEMORY", "Soldier",		SolVer     );
	Ini->GetVal( "MEMORY", "Soldier60",		Sol60Mode  );
	Ini->GetVal( "MEMORY", "SoldierBank",	SolBankSet );
	
	// 62,66,64,68
	Ini->GetVal( "MEMORY", "cgrom",			cgrom    );
	Ini->GetVal( "MEMORY", "kj_rom",		kj_rom   );
	Ini->GetVal( "MEMORY", "kj_LR",			kj_LR    );
	Ini->GetVal( "MEMORY", "cgenable",		cgenable );
	Ini->GetVal( "MEMORY", "cgaden",		cgaden   );
	Ini->GetVal( "MEMORY", "cgaddr",		cgaddr   );
	Ini->GetVal( "MEMORY", "Rf0",			Rf[0]    );
	Ini->GetVal( "MEMORY", "Rf1",			Rf[1]    );
	Ini->GetVal( "MEMORY", "Rf2",			Rf[2]    );
	
	// 拡張ROM
	if( Ini->GetVal( "MEMORY", "FilePath", tpath ) )
		MountExtRom( tpath );
	
	// メモリブロック設定
	Init();
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
	SetCGBank( CGBank );
	
	// メモリウェイト
	st = GetWait();
	Ini->GetVal( "MEMORY", "Wait",      st );
	SetWait( st );
	// CGRomウェイト
	st = CGROM1.GetWait();
	Ini->GetVal( "MEMORY", "CgRomWait", st );
	st &= 0xff;
	CGROM1.SetWait( st );
	CGROM2.SetWait( st );
	
	
	// 内部RAM
	for( int i=0; i<(int)MemTable.IntRam->Size; i+=64 ){
		std::string strva;
		if( Ini->GetEntry( "MEMORY", Stringf( "IntRam_%04X", i ), strva ) ){
			strva += std::string( 64 * 2 - strva.length(), '0' );
			for( int j=0; j<64; j++ ){
				IntRam[(i+j)>>MemBlock::PAGEBITS][(i+j)&MemBlock::PAGEMASK] = std::stoul( strva.substr( j * 2, 2 ), nullptr, 16 );
			}
		}
	}
	
	// 外部RAM
	if( UseExtRam || SolVer ){
		for( int i=0; i<(int)MemTable.ExtRam->Size; i+=64 ){
			std::string strva;
			if( Ini->GetEntry( "MEMORY", Stringf( "ExtRam_%06X", i ), strva ) ){
				strva += std::string( 64 * 2 - strva.length(), '0' );
				for( int j=0; j<64; j++ ){
					ExtRam[(i+j)>>MemBlock::PAGEBITS][(i+j)&MemBlock::PAGEMASK] = std::stoul( strva.substr( j * 2, 2 ), nullptr, 16 );
				}
			}
		}
	}
	
	// 戦士のカートリッジ
	if( SolVer ){
		// メモリバンクレジスタ
		for( int i=0; i<8; i++ ){
			Ini->GetVal( "MEMORY", Stringf( "SolBank%d", i ), SolBank[i] );
			SetSolBank( i, SolBank[i] );	// メモリバンク設定
		}
	}
	
	return true;
}

bool MEM64::DokoLoad( cIni* Ini )
{
	if( !MEM6::DokoLoad( Ini ) ) return false;
	
	for( int i=0; i<16; i++ ){
		Ini->GetVal( "MEMORY", Stringf( "RfSR_%02d", i ), RfSR[i] );
		SetMemBlockSR( i, RfSR[i] );
	}
	
	return true;
}


#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
////////////////////////////////////////////////////////////////
// モニタモード用関数
////////////////////////////////////////////////////////////////
const std::string& MEM6::GetReadMemBlk( int blk ) const 
{
	return vm->VdgIsSRmode() ? Rm_blkSR[blk]->GetName()
							 : Rm_blk  [blk]->GetName();
}

const std::string& MEM6::GetWriteMemBlk( int blk ) const 
{
	return vm->VdgIsSRmode() ? Wm_blkSR[blk]->GetName()
							 : Wm_blk  [blk]->GetName();
}
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

