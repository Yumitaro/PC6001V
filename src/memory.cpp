#include <cstring>
#include <fstream>
#include <map>

#include "pc6001v.h"

#include "common.h"
#include "error.h"
#include "log.h"
#include "memory.h"
#include "osd.h"
#include "p6vm.h"


// メモリブロック数
#define MAXRIMB		(20)			// 内部ROM Read
#define MAXREMB		(2)				// 外部ROM Read
#define MAXWIMB		(8)				// 内部RAM Write
#define MAXWEMB		(8)				// 外部RAM Write

// メモリブロック割り当て
#define EMPTYROM	(InRomB[0])		// EmptyRom
#define EMPTYRAM	(InRomB[1])		// EmptyRam
#define MAINROM0	(InRomB[2])		// SysRom1
#define MAINROM1	(InRomB[3])		// SysRom1+0x2000
#define MAINROM2	(InRomB[4])		// SysRom1+0x4000
#define MAINROM3	(InRomB[5])		// SysRom1+0x6000
#define MAINROM4	(InRomB[6])		// SysRom1+0x8000
#define MAINROM5	(InRomB[7])		// SysRom1+0xa000
#define MAINROM6	(InRomB[8])		// SysRom1+0xc000
#define MAINROM7	(InRomB[9])		// SysRom1+0xe000
#define CGROM1		(InRomB[10])	// CGRom1
#define CGROM2		(InRomB[11])	// CGRom2			CGRom1+0x2000
#define KANJIROM0	(InRomB[12])	// KanjiRom			SysRom2+0x8000
#define KANJIROM1	(InRomB[13])	// KanjiRom+0x2000	SysRom2+0xa000
#define KANJIROM2	(InRomB[14])	// KanjiRom+0x4000	SysRom2+0xc000
#define KANJIROM3	(InRomB[15])	// KanjiRom+0x6000	SysRom2+0xe000
#define VOICEROM0	(InRomB[16])	// VoiceRom			SysRom2+0x4000
#define VOICEROM1	(InRomB[17])	// VoiceRom+0x2000	SysRom2+0x6000
#define SRMENROM0	(InRomB[18])	//					SysRom2
#define SRMENROM1	(InRomB[19])	//					SysRom2+0x2000

#define EXTROM0		(ExRomB[0])		// ExtRom
#define EXTROM1		(ExRomB[1])		// ExtRom+0x2000

#define INTRAM0		(InRamB[0])		// IntRam
#define INTRAM1		(InRamB[1])		// IntRam+0x2000
#define INTRAM2		(InRamB[2])		// IntRam+0x4000
#define INTRAM3		(InRamB[3])		// IntRam+0x6000
#define INTRAM4		(InRamB[4])		// IntRam+0x8000
#define INTRAM5		(InRamB[5])		// IntRam+0xa000
#define INTRAM6		(InRamB[6])		// IntRam+0xc000
#define INTRAM7		(InRamB[7])		// IntRam+0xe000

#define EXTRAM0		(ExRamB[0])		// ExtRam
#define EXTRAM1		(ExRamB[1])		// ExtRam+0x2000
#define EXTRAM2		(ExRamB[2])		// ExtRam+0x4000
#define EXTRAM3		(ExRamB[3])		// ExtRam+0x6000
#define EXTRAM4		(ExRamB[4])		// ExtRam+0x8000
#define EXTRAM5		(ExRamB[5])		// ExtRam+0xa000
#define EXTRAM6		(ExRamB[6])		// ExtRam+0xc000
#define EXTRAM7		(ExRamB[7])		// ExtRam+0xe000

#define INEXRAM		(InExRamB)		// IntRam+ExtRam

#define pKNJROM0	(kj_rom ? ( kj_LR ? &KANJIROM2 : &KANJIROM0 ) : &SRMENROM0 )
#define pKNJROM1	(kj_rom ? ( kj_LR ? &KANJIROM3 : &KANJIROM1 ) : &SRMENROM1 )
#define pKNJROM2	(kj_rom ? ( kj_LR ? &KANJIROM2 : &KANJIROM0 ) : &VOICEROM0 )
#define pKNJROM3	(kj_rom ? ( kj_LR ? &KANJIROM3 : &KANJIROM1 ) : &VOICEROM1 )



// メモリコントローラ内部レジスタ初期値
#define	INIT_RF0	(0x71)
#define INIT_RF1	(0xdd)
#define INIT_RF2	(0x50)

// 戦士のカートリッジ バンク分類
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

// 拡張カートリッジ
const std::vector<ROMINFO> EXBASIC00 = { { "EXBASIC.ROM",	0x04000,	0          } };
const std::vector<ROMINFO> EXKANJI00 = { { "EXKANJI.ROM",	0x020000,	0          } };



////////////////////////////////////////////////////////////////
// ROMセット情報
////////////////////////////////////////////////////////////////
const std::map<int, const std::vector<std::vector<ROMINFO>>> ROMSET {
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
// 共通									 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM6::IEMPTROM   = { NOROM,		0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEMPTRAM   = { NOROM,		0x002000,	0xff,	0 };
const MEM6::MEMINFO MEM6::IEXTROM16  = { NOROM,		0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM128 = { NOROM,		0x020000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM512 = { NOROM,		0x080000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM8M  = { NOROM,		0x800000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTRAM16  = { NOROM,		0x004000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM32  = { NOROM,		0x008000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM64  = { NOROM,		0x010000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM128 = { NOROM,		0x020000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM512 = { NOROM,		0x080000,	0x00,	0 };

// 拡張カートリッジ
const MEM6::MEMINFO MEM6::IEXBASIC   = { EXBASIC00,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXKANJI   = { EXKANJI00,	0x020000,	0xff,	0 };

// PC-6001								 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM60::ISYSROM1  = { SYSROM160,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM60::ICGROM1   = { CGROM160,	0x001000,	0xff,	1 };
const MEM6::MEMINFO MEM60::IINTRAM   = { NOROM,		0x004000,	0x00,	0 };

// PC-6001A								 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM61::ISYSROM1  = { SYSROM161,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM61::ICGROM1   = { CGROM161,	0x001000,	0xff,	1 };
const MEM6::MEMINFO MEM61::IINTRAM   = { NOROM,		0x004000,	0x00,	0 };

// PC-6001mk2							 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM62::ISYSROM1  = { SYSROM162,	0x008000,	0xff,	1 };
const MEM6::MEMINFO MEM62::ICGROM1   = { CGROM162,	0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM62::ICGROM2   = { CGROM262,	0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IKANJI    = { KANJI62,	0x008000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IVOICE    = { VOICE62,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM62::IINTRAM   = { NOROM,		0x010000,	0x00,	0 };

// PC-6601								 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM66::ISYSROM1  = { SYSROM166,	0x008000,	0xff,	1 };
const MEM6::MEMINFO MEM66::ICGROM1   = { CGROM166,	0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM66::ICGROM2   = { CGROM266,	0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IKANJI    = { KANJI66,	0x008000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IVOICE    = { VOICE66,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM66::IINTRAM   = { NOROM,		0x010000,	0x00,	0 };

// PC-6001mk2SR / PC-6601SR				 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM64::ISYSROM1  = { SYSROM164,	0x010000,	0xff,	1 };
const MEM6::MEMINFO MEM64::ISYSROM2  = { SYSROM264,	0x010000,	0xff,	1 };
const MEM6::MEMINFO MEM64::ICGROM1   = { CGROM164,	0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM64::IINTRAM   = { NOROM,		0x010000,	0x00,	0 };



// ダミーメモリセル
MemCell EmptyCell( 0xff, true );


//--------------------------------------------------------------
// メモリセルクラス(最小単位)
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
// リサイズ
////////////////////////////////////////////////////////////////
void MemCell::Resize( size_t size, BYTE idata )
{
	Data.assign( size, idata );
}


////////////////////////////////////////////////////////////////
// ROMデータをファイルから読込み
//
// 引数:	fs		ファイルストリームへの参照
// 返値:	なし
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
//
// 引数:	なし
// 返値:	size_t	データサイズ(bytes)
////////////////////////////////////////////////////////////////
size_t MemCell::Size( void ) const
{
	return Data.size();
}


////////////////////////////////////////////////////////////////
// メモリリード
//
// 引数:	addr	アドレス
// 返値:	BYTE	データ
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
//
// 引数:	addr	アドレス
//			data	データ
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemCell::Write( WORD addr, BYTE data )
{
	if( WPt ) return;
	
	try{
		Data.at( addr & PAGEMASK ) = data;
	}
	catch( std::out_of_range& ){}
}




//--------------------------------------------------------------
// メモリセル集合体クラス(ROM/RAMチップ相当)
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
// リサイズ
//
// 引数:	size	データサイズ
//			idata	初期値
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemCells::Resize( size_t size, BYTE idata )
{
	// RAMとして再確保する
	Cells.assign( (size + MemCell::PAGEBITS) >> MemCell::PAGEBITS, MemCell( idata ) );
	// 端数がある場合は末尾のメモリセルをリサイズ
	size_t sz = size & MemCell::PAGEBITS;
	if( sz ){
		Cells.rbegin()->Resize( sz );
	}
}


////////////////////////////////////////////////////////////////
// ROMデータをファイルから読込み
//
// 引数:	filepath	ファイル名へのパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MemCells::SetData( const P6VPATH& filepath )
{
	try{
		// ファイルサイズに合わせてメモリセル再設定(端数考慮)
		Cells.resize( (OSD_GetFileSize( filepath ) + MemCell::PAGEMASK) >> MemCell::PAGEBITS );
		
		std::fstream fs;
		if( !OSD_FSopen( fs, filepath, std::ios_base::in|std::ios_base::binary ) ) throw Error::NoRom;
		
		for( auto &mc : Cells ){
			mc.SetData( fs );
		}
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
//
// 引数:	なし
// 返値:	size_t	メモリセル数
////////////////////////////////////////////////////////////////
size_t MemCells::Size( void ) const
{
	return Cells.size();
}


////////////////////////////////////////////////////////////////
// メモリリード
//
// 引数:	addr	アドレス
// 返値:	BYTE	データ
////////////////////////////////////////////////////////////////
BYTE MemCells::Read( WORD addr ) const
{
	try{
		return Cells.at( (addr >> MemCell::PAGEBITS) & (Cells.size() - 1) ).Read( addr );
	}
	catch( std::out_of_range& ){}
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
//
// 引数:	addr	アドレス
//			data	データ
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemCells::Write( WORD addr, BYTE data )
{
	try{
		Cells.at( (addr >> MemCell::PAGEBITS) & (Cells.size() - 1) ).Write( addr, data );
	}
	catch( std::out_of_range& ){}
}


////////////////////////////////////////////////////////////////
// メモリセル取得 - operator ()
//
// 引数:	num			インデックス(0-)
// 返値:	MemCell&	メモリセルへの参照
////////////////////////////////////////////////////////////////
MemCell& MemCells::operator()( const int num )
{
	try{
		return Cells.at( num );
	}
	catch( std::out_of_range& ){}
	
	return EmptyCell;
}




//--------------------------------------------------------------
// メモリブロッククラス
//--------------------------------------------------------------

////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MemBlock::MemBlock( void ) : Name( "" ), PMem( &EmptyCell ), FRead( nullptr ), FWrite( nullptr ), Wait( 0 )
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
//			data	メモリセルへの参照
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetMemory( const std::string& name, MemCell& data, int wait )
{
	Name   = name;
	PMem   = &data;
	FRead  = nullptr;
	FWrite = nullptr;
	Wait   = wait < 0 ? Wait : wait;
}


////////////////////////////////////////////////////////////////
// 関数割当て
//
// 引数:	name	メモリブロック名への参照
//			rd		読込み関数ポインタ(bind)
//			wr		書込み関数ポインタ(bind)
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::SetFunc( const std::string& name, RFunc rd, WFunc wr, int wait )
{
	Name   = name;
	PMem   = &EmptyCell;
	FRead  = rd;
	FWrite = wr;
	Wait   = wait < 0 ? Wait : wait;
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
// 返値:	int		ウェイト
////////////////////////////////////////////////////////////////
int MemBlock::GetWait( void ) const
{
	return Wait;
}


////////////////////////////////////////////////////////////////
// メモリリード
//
// 引数:	addr	アドレス
//			wcnt	ウェイトカウンタへのポインタ
// 返値:	BYTE	データ
////////////////////////////////////////////////////////////////
BYTE MemBlock::Read( WORD addr, int* wcnt ) const
{
	if( wcnt ){ *wcnt += Wait; }
	
	if( FRead ){
		return FRead( PMem, addr );
	}else if( PMem->Size() ){
		return PMem->Read( addr );
	}
	
	return 0xff;
}


////////////////////////////////////////////////////////////////
// メモリライト
//
// 引数:	addr	アドレス
//			data	データ
//			wcnt	ウェイトカウンタへのポインタ
// 返値:	なし
////////////////////////////////////////////////////////////////
void MemBlock::Write( WORD addr, BYTE data, int* wcnt ) const
{
	if( wcnt ){	*wcnt += Wait; }
	
	if( FWrite ){
		FWrite( PMem, addr, data );
	}else if( PMem->Size() ){
		PMem->Write( addr, data );
	}
}










////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MEM6::MEM6( VM6* vm, const ID& id ) : Device( vm, id ),
	CGBank( false ), ReadyExRom( false ),
	FilePath( "" ), M1Wait( 1 ), EnableChkCRC( true ),
	cgrom( true ), kj_rom( true ), kj_LR( true ), cgenable( true ), cgaden( 0 ), cgaddr( 3 ), c2acc( 0xff ),
	ExCart( 0 ), Sol60Mode( false ), CS01R( false ), CS01W( false ), SolBankSet( 0 ), Kaddr( 0 ), Kenable( false )
{
	InRomB.assign( MAXRIMB, MemBlock() );
	InRamB.assign( MAXWIMB, MemBlock() );
	ExRomB.assign( MAXREMB, MemBlock() );
	ExRamB.assign( MAXWEMB, MemBlock() );
	RD_Blk.fill( nullptr );
	WR_Blk.fill( nullptr );
	RD_BlkSR.fill( nullptr );
	WR_BlkSR.fill( nullptr );
	
	Rf = { INIT_RF0, INIT_RF1, INIT_RF2 };
	RfSR.fill( 0 );
	
	SolBank.fill( NONBANK );
}

MEM60::MEM60( VM6* vm, const ID& id ) : MEM6( vm, id )
{
	MemTable.IntRam  = &MEM60::IINTRAM;
	MemTable.System1 = &MEM60::ISYSROM1;
	MemTable.CGRom1  = &MEM60::ICGROM1;
	
	// Dvice Description (Out) ※enumと同じ順番で追加する
	descs.outdef.clear();
	// 戦士のカートリッジ --------------------------------------------------------------------
	descs.outdef.emplace( out06H,  STATIC_CAST( Device::OutFuncPtr, &MEM60::Out06H  ) );
	descs.outdef.emplace( out3xH,  STATIC_CAST( Device::OutFuncPtr, &MEM60::Out3xH  ) );
	descs.outdef.emplace( out7FH,  STATIC_CAST( Device::OutFuncPtr, &MEM60::Out7FH  ) );
	descs.outdef.emplace( outF0Hs, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutF0Hs ) );	// 戦士のカートリッジ対応
	descs.outdef.emplace( outF2Hs, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutF2Hs ) );	// 戦士のカートリッジ対応
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.outdef.emplace( outFCH, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutFCH ) );
	descs.outdef.emplace( outFFH, STATIC_CAST( Device::OutFuncPtr, &MEM60::OutFFH ) );
	// ---------------------------------------------------------------------------------------
	
	// Dvice Description (In) ※enumと同じ順番で追加する
	descs.indef.clear();
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.indef.emplace ( inFDH,  STATIC_CAST( Device::InFuncPtr,  &MEM60::InFDH  ) );
	descs.indef.emplace ( inFEH,  STATIC_CAST( Device::InFuncPtr,  &MEM60::InFEH  ) );
	// ---------------------------------------------------------------------------------------
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
	
	// Dvice Description (Out) ※enumと同じ順番で追加する
	descs.outdef.clear();
	// 戦士のカートリッジ --------------------------------------------------------------------
	descs.outdef.emplace( out06H,  STATIC_CAST( Device::OutFuncPtr, &MEM62::Out06H  ) );
	descs.outdef.emplace( out3xH,  STATIC_CAST( Device::OutFuncPtr, &MEM62::Out3xH  ) );
	descs.outdef.emplace( out7FH,  STATIC_CAST( Device::OutFuncPtr, &MEM62::Out7FH  ) );
	descs.outdef.emplace( outF0Hs, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF0Hs ) );	// 戦士のカートリッジ対応
	descs.outdef.emplace( outF2Hs, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF2Hs ) );	// 戦士のカートリッジ対応
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.outdef.emplace( outFCH, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutFCH ) );
	descs.outdef.emplace( outFFH, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutFFH ) );
	// ---------------------------------------------------------------------------------------
	descs.outdef.emplace( outC1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC1H ) );
	descs.outdef.emplace( outC2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC2H ) );
	descs.outdef.emplace( outC3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC3H ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF0H ) );
	descs.outdef.emplace( outF1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF1H ) );
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF2H ) );
	descs.outdef.emplace( outF3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF3H ) );
	descs.outdef.emplace( outF8H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF8H ) );
	
	// Dvice Description (In) ※enumと同じ順番で追加する
	descs.indef.clear();
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.indef.emplace ( inFDH,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InFDH  ) );
	descs.indef.emplace ( inFEH,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InFEH  ) );
	// ---------------------------------------------------------------------------------------
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
	
	// Dvice Description (Out) ※enumと同じ順番で追加する
	descs.outdef.clear();
	// 戦士のカートリッジ --------------------------------------------------------------------
	descs.outdef.emplace( out06H,  STATIC_CAST( Device::OutFuncPtr, &MEM64::Out06H  ) );
	descs.outdef.emplace( out3xH,  STATIC_CAST( Device::OutFuncPtr, &MEM64::Out3xH  ) );
	descs.outdef.emplace( out7FH,  STATIC_CAST( Device::OutFuncPtr, &MEM64::Out7FH  ) );
	descs.outdef.emplace( outF0Hs, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF0Hs ) );	// 戦士のカートリッジ対応
	descs.outdef.emplace( outF2Hs, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF2Hs ) );	// 戦士のカートリッジ対応
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.outdef.emplace( outFCH, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutFCH ) );
	descs.outdef.emplace( outFFH, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutFFH ) );
	// ---------------------------------------------------------------------------------------
	descs.outdef.emplace( out6xH, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out6xH ) );
	descs.outdef.emplace( outC1H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC1H ) );
	descs.outdef.emplace( outC2H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC2H ) );
	descs.outdef.emplace( outC3H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutC3H ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF0H ) );
	descs.outdef.emplace( outF1H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF1H ) );
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF2H ) );
	descs.outdef.emplace( outF3H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF3H ) );
	descs.outdef.emplace( outF8H, STATIC_CAST( Device::OutFuncPtr, &MEM64::OutF8H ) );
	
	// Dvice Description (In) ※enumと同じ順番で追加する
	descs.indef.clear();
	// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
	descs.indef.emplace ( inFDH,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InFDH  ) );
	descs.indef.emplace ( inFEH,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InFEH  ) );
	// ---------------------------------------------------------------------------------------
	descs.indef.emplace ( in6xH,  STATIC_CAST( Device::InFuncPtr,  &MEM64::In6xH  ) );
	descs.indef.emplace ( inB2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InB2H  ) );
	descs.indef.emplace ( inC2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InC2H  ) );
	descs.indef.emplace ( inF0H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF0H  ) );
	descs.indef.emplace ( inF1H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF1H  ) );
	descs.indef.emplace ( inF2H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF2H  ) );
	descs.indef.emplace ( inF3H,  STATIC_CAST( Device::InFuncPtr,  &MEM64::InF3H  ) );
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
// 戦士のカートリッジ読込み(外部ROM領域)
//   mkII以降の場合,外部ROMは4000-7FFFH以外に割当てることができるので
//   どこに割当てられてもアクセスできるように一段階かませる
////////////////////////////////////////////////////////////////
BYTE MEM6::SolReadEx( MemCell* ptr, WORD addr )
{
	return ExRamB[addr>>MemBlock::PAGEBITS].Read( addr );
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジAreaA,B読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::Sol60ReadA( MemCell* ptr, WORD addr )
{
	if( Sol60Mode && CS01R ){
		// EXTRAM0(ExRamB[0]),EXTRAM1(ExRamB[1])
		return ExRamB[addr>>MemBlock::PAGEBITS].Read( addr );
	}else{
		// MAINROM0(SysRom1(0)),MAINROM1(SysRom1(1))
		return SysRom1( addr>>MemBlock::PAGEBITS ).Read( addr );
	}
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジAreaA,B書込み
////////////////////////////////////////////////////////////////
void MEM6::Sol60WriteA( MemCell* ptr, WORD addr, BYTE data )
{
	if( Sol60Mode && CS01W ){
		// EXTRAM0(ExRamB[0]),EXTRAM1(ExRamB[1])
		ExRamB[addr>>MemBlock::PAGEBITS].Write( addr, data );
	}else{
		// MAINROM0(InRomB[2]),MAINROM1(InRomB[3])
		// 何もしない
	}
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::SolSccRead( MemCell* ptr, WORD addr )
{
	// 後で書く
	return 0xff;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC書込み
////////////////////////////////////////////////////////////////
void MEM6::SolSccWrite( MemCell* ptr, WORD addr, BYTE data )
{
	// 後で書く
}


////////////////////////////////////////////////////////////////
// PC-6001 CGROM読込み
////////////////////////////////////////////////////////////////
BYTE MEM60::CGromRead( MemCell* ptr, WORD addr )
{
	// 前半4KBはそのままCGROMデータを返す
	if( !(addr & 0x1000) )	return CGRom1.Read( addr & 0x0fff );
	
	// 後半4KBを読込んだ時の挙動を記述
	// 外部ROMが無い場合はCGROMのイメージとなるらしい
	// 戦士のカートリッジの場合はホントはコレジャナイんだけど後回し
	return (ReadyExRom && (ExCart & EXCBUS)) ? ExtRom.Read( addr ) : CGRom1.Read( addr & 0x0fff );
}


////////////////////////////////////////////////////////////////
// PC-6001mk2以降 内部/外部RAM書込み
////////////////////////////////////////////////////////////////
void MEM62::IERamWrite( MemCell* ptr, WORD addr, BYTE data )
{
	InRamB[addr>>MemBlock::PAGEBITS].Write( addr, data );
	ExRamB[addr>>MemBlock::PAGEBITS].Write( addr, data );
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
	if( ReadyExRom ) UnmountExtRom();
	
	try{
		if( !ExtRom.SetData( filepath ) ) return false;
		
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
	
	ReadyExRom = true;
	
	PRINTD( MEM_LOG, "OK %s\n", P6VPATH2STR( FilePath ).c_str() );
	
	return true;
}


////////////////////////////////////////////////////////////////
// 拡張ROM アンマウント
////////////////////////////////////////////////////////////////
void MEM6::UnmountExtRom( void )
{
	PRINTD( MEM_LOG, "[MEM][UnmountExtRom]\n" );
	
	ExtRom.Resize( MemTable.ExtRom->Size, MemTable.ExtRom->Init );
	FilePath.clear();
	
	ReadyExRom = false;
}


////////////////////////////////////////////////////////////////
// 拡張ROMファイルパス取得
////////////////////////////////////////////////////////////////
const P6VPATH& MEM6::GetFile( void ) const
{
	return FilePath;
}


////////////////////////////////////////////////////////////////
// 拡張カートリッジの種類取得
////////////////////////////////////////////////////////////////
WORD MEM6::GetCartridge() const
{
	return ExCart;
}


////////////////////////////////////////////////////////////////
// CRC32計算
//
// 引数:	buf				データバッファへのポインタ
//			num				データ数(バイト)
// 返値:	DWORD			CRC32値
////////////////////////////////////////////////////////////////
DWORD MEM6::CalcCrc32( MemCells& buf, int num )
{
	DWORD crc = 0xffffffff;
	
	for( int i=0; i < num; i++ ){
		crc ^= buf.Read( i );
		for( int j=0; j < 8; j++ )
			if( crc & 1 ) crc   = (crc >> 1) ^ 0xedb88320;
			else		  crc >>= 1;
	}
	return crc ^ 0xffffffff;
}


////////////////////////////////////////////////////////////////
// メモリ確保とROMファイル読込み
////////////////////////////////////////////////////////////////
bool MEM6::AllocMemory( MemCells& buf, const MEMINFO* info, const P6VPATH& path )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemory] " );
	
	int i = 0;
	bool ErrSize = false;
	bool ErrCrc = false;
	
	try{
		// メモリリサイズ
		buf.Resize( info->Size, info->Init );
		
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
			
			// ROMデータをファイルから読込み
			if( buf.SetData( fpath ) ){
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
	catch( Error::Errno i ){	// 例外発生
		PRINTD( MEM_LOG, "-> Failed\n" );
		
		Error::SetError( i );
		
		switch( i ){
		case Error::NoRom:		// ファイルのオープンに失敗した場合
		case Error::RomSizeNG:	// サイズが合わない場合
		case Error::RomCrcNG:	// CRCが合わない場合
		default:				// メモリを開放
			buf.Resize( 0 );
		}
		return false;
	}
	
	return false;
}


////////////////////////////////////////////////////////////////
// 全メモリ確保とROMファイル読込み
////////////////////////////////////////////////////////////////
bool MEM6::AllocAllMemory( const P6VPATH& path, WORD cart, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocAllMemory]\n" );
	
	EnableChkCRC = crc;		// CRCチェック有効
	
	// 共通
	if( !AllocMemory( IntRam,  MemTable.IntRam,  ""   ) ) return false;
	
	if( !AllocMemory( SysRom1, MemTable.System1, path ) ) return false;
	if( !AllocMemory( CGRom1,  MemTable.CGRom1,  path ) ) return false;
	
	 // 全メモリ確保とROMファイル読込み(機種別)
	if( !AllocMemorySpecific( path ) ) return false;
	
	// 内部RAMの初期値を設定
	SetRamValue();
	
	
	
	// 拡張カートリッジ ----------------------------------------
	ExCart = cart;			// 拡張カートリッジ
	
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
	case EXC6005:	// ROMカートリッジ
		// ROM:16KB RAM:-
		MemTable.ExtRom = &MEM6::IEXTROM16;
		MemTable.ExtRam = &MEM6::IEMPTRAM;
		break;
		
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		// ROM:16KB RAM:16KB
		MemTable.ExtRom = &MEM6::IEXTROM16;
		MemTable.ExtRam = &MEM6::IEXTRAM16;
		break;
		
	case EXC660101:	// 拡張漢字ROMカートリッジ
		// ROM:128KB RAM:-
		MemTable.ExtRom = &MEM6::IEXTROM128;
		MemTable.ExtRam = &MEM6::IEMPTRAM;
		break;
		
	case EXC6006SR:	// 拡張64KRAMカートリッジ
		// ROM:- RAM:64KB
		MemTable.ExtRom = &MEM6::IEMPTROM;
		MemTable.ExtRam = &MEM6::IEXTRAM64;
		break;
		
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		// ROM:128KB RAM:64KB
		MemTable.ExtRom = &MEM6::IEXTROM128;
		MemTable.ExtRam = &MEM6::IEXTRAM64;
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
		// ROM:128KB RAM:32KB
		MemTable.ExtRom = &MEM6::IEXTROM128;
		MemTable.ExtRam = &MEM6::IEXTRAM32;
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
		// ROM:512KB RAM:128KB
		MemTable.ExtRom = &MEM6::IEXTROM512;
		MemTable.ExtRam = &MEM6::IEXTRAM128;
		break;
		
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		// ROM:8MB RAM:512KB
		MemTable.ExtRom = &MEM6::IEXTROM8M;
		MemTable.ExtRam = &MEM6::IEXTRAM512;
		break;
	}
	
	if( !AllocMemory( ExtRam, MemTable.ExtRam,  ""   ) ) return false;
	
	
	// ROMファイル読込み
	switch( ExCart ){
	case EXC6001:
		if( !AllocMemory( ExtRom, &IEXBASIC, path ) ) return false;
		break;
		
	case EXC660101:
	case EXC6007SR:
		if( !AllocMemory( ExtRom, &IEXKANJI, path ) ) return false;
		break;
		
	default:
		if( !AllocMemory( ExtRom, MemTable.ExtRom,  ""   ) ) return false;
	}
	// ---------------------------------------------------------
	
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
// 内部RAMの初期値を設定
////////////////////////////////////////////////////////////////
void MEM60::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam.Write( i, i&0x40 ? 0x00 : 0xff );
	}
}

void MEM62::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam.Write( i, ((i>>7)^i)&1 ? 0xff : 0x00 );
	}
}

void MEM64::SetRamValue( void )
{
	// 0000-FF9FH
	for( int i=0; i<0xffa0; i++ ){
		IntRam.Write( i, ((i>>9)^(i>>1))&1 ? 0xff : 0x00 );
	}
	// FFA0-FFFFH
	for( int i=0xffa0; i<0x10000; i++ ){
		IntRam.Write( i, 0x00 );
	}
}

void MEM68::SetRamValue( void )
{
	for( int i=0; i<(int)MemTable.IntRam->Size; i++ ){
		IntRam.Write( i, i&0x100 ? 0xff : 0x00 );
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
	for( auto &mb : InRomB ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 1 ); }
	for( auto &mb : ExRomB ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 1 ); }
	for( auto &mb : InRamB ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 0 ); }
	for( auto &mb : ExRamB ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 0 ); }
	InExRamB.SetFunc( "EMPTY", nullptr, nullptr, 0 );
	
	// 拡張カートリッジ ----------------------------------------
	// ROM
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
	case EXC6005:	// ROMカートリッジ
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		EXTROM0.SetMemory( "EXROM0", ExtRom( 0 ), MemTable.ExtRom->Wait );
		EXTROM1.SetMemory( "EXROM1", ExtRom( 1 ), MemTable.ExtRom->Wait );
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
		EXTROM0.SetFunc  ( "EXROM0", FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		EXTROM1.SetMemory( "EXRAM3", ExtRam( 3 ), MemTable.ExtRom->Wait );
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		EXTROM0.SetFunc  ( "EXROM0", FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		EXTROM1.SetFunc  ( "EXROM1", FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		break;
	}
	
	// RAM
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
	case EXCSOL1:	// 戦士のカートリッジ
		{	int i = 0;
			for( auto &mb : ExRamB ){
				mb.SetMemory( Stringf( "EXRAM%d", i & (ExtRam.Size()-1) ), ExtRam( i & (ExtRam.Size()-1) ), MemTable.ExtRam->Wait );
				i++;
			}
		}
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		SetSolBank( 3, RAMBANK | 3 );
		SetSolBank( 4, RAMBANK | 4 );
		SetSolBank( 5, RAMBANK | 5 );
		break;
	}
	// ---------------------------------------------------------
	
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
	MAINROM0.SetMemory( "BASIC0", SysRom1( 0 ), MemTable.System1->Wait );
	MAINROM1.SetMemory( "BASIC1", SysRom1( 1 ), MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetFunc    ( "CGROM1", FR( STATIC_CAST( RFuncPtr, &MEM60::CGromRead ) ), nullptr, MemTable.CGRom1->Wait );
	
	// 内部RAM
	INTRAM0.SetMemory ( "INRAM0", IntRam( 0 ),  MemTable.IntRam->Wait );
	INTRAM1.SetMemory ( "INRAM1", IntRam( 1 ),  MemTable.IntRam->Wait );
	
	// 拡張カートリッジ ----------------------------------------
	switch( ExCart ){
	case EXCSOL2:		// 戦士のカートリッジmkⅡ
	case EXCSOL3:		// 戦士のカートリッジmkⅢ
		MAINROM0.SetFunc( "BASOL0", FR( STATIC_CAST( RFuncPtr, &MEM60::Sol60ReadA ) ), FW( STATIC_CAST( WFuncPtr, &MEM60::Sol60WriteA ) ), MemTable.System1->Wait );
		MAINROM1.SetFunc( "BASOL1", FR( STATIC_CAST( RFuncPtr, &MEM60::Sol60ReadA ) ), FW( STATIC_CAST( WFuncPtr, &MEM60::Sol60WriteA ) ), MemTable.System1->Wait );
		
		EXTRAM0.SetWait( MemTable.System1->Wait );
		EXTRAM1.SetWait( MemTable.System1->Wait );
		EXTRAM2.SetWait( MemTable.ExtRom->Wait  );
		EXTRAM3.SetWait( MemTable.ExtRom->Wait  );
	}
	// ---------------------------------------------------------
	
	return true;
}

bool MEM62::InitSpecific( void )
{
	PRINTD( MEM_LOG, "[MEM][InitSpecific]\n" );
	
	// メモリブロック設定
	// BASIC ROM
	MAINROM0.SetMemory ( "BASIC0", SysRom1( 0 ),  MemTable.System1->Wait );
	MAINROM1.SetMemory ( "BASIC1", SysRom1( 1 ),  MemTable.System1->Wait );
	MAINROM2.SetMemory ( "BASIC2", SysRom1( 2 ),  MemTable.System1->Wait );
	MAINROM3.SetMemory ( "BASIC3", SysRom1( 3 ),  MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetMemory   ( "CGROM1", CGRom1( 0 ),   MemTable.CGRom1->Wait );
	CGROM2.SetMemory   ( "CGROM2", CGRom2( 0 ),   MemTable.CGRom2->Wait );
	
	// 漢字ROM
	KANJIROM0.SetMemory( "KJROM0", KanjiRom( 0 ), MemTable.Kanji->Wait );
	KANJIROM1.SetMemory( "KJROM1", KanjiRom( 1 ), MemTable.Kanji->Wait );
	KANJIROM2.SetMemory( "KJROM2", KanjiRom( 2 ), MemTable.Kanji->Wait );
	KANJIROM3.SetMemory( "KJROM3", KanjiRom( 3 ), MemTable.Kanji->Wait );
	
	// 音声合成ROM
	VOICEROM0.SetMemory( "VOROM0", VoiceRom( 0 ), MemTable.Voice->Wait );
	VOICEROM1.SetMemory( "VOROM1", VoiceRom( 1 ), MemTable.Voice->Wait );
	
	// 内部RAM
	int i = 0;
	for( auto &mb : InRamB ){
		mb.SetMemory( Stringf( "INRAM%d", i ), IntRam( i ), MemTable.IntRam->Wait );
		i++;
	}
	
	// 内部/外部RAM書込み
	INEXRAM.SetFunc    ( "IERAM", nullptr, FW( STATIC_CAST( WFuncPtr, &MEM62::IERamWrite ) ), MemTable.IntRam->Wait );
	
	// 拡張カートリッジ ----------------------------------------
	switch( ExCart ){
	case EXCSOL1:		// 戦士のカートリッジ
		// ※mk2以降用の設定はこれ↓だけど多分正しくない
		// 設定は「内部RAMに書込み」だけどハード的に同時に外部RAMにも書き込まれる
		EXTROM1.SetMemory( "IERAM3", IntRam( 3 ), MemTable.ExtRom->Wait );
	}
	// ---------------------------------------------------------
	
	return true;
}

bool MEM64::InitSpecific( void )
{
	PRINTD( MEM_LOG, "[MEM][InitSpecific]\n" );
	
	// メモリブロック設定
	// N66-BASIC ROM
	MAINROM0.SetMemory ( "SYS1-0", SysRom1( 0 ), MemTable.System1->Wait );
	MAINROM1.SetMemory ( "SYS1-1", SysRom1( 1 ), MemTable.System1->Wait );
	MAINROM2.SetMemory ( "SYS1-2", SysRom1( 2 ), MemTable.System1->Wait );
	MAINROM3.SetMemory ( "SYS1-3", SysRom1( 3 ), MemTable.System1->Wait );
	
	// N66SR-BASIC ROM
	MAINROM4.SetMemory ( "SYS1-4", SysRom1( 4 ), MemTable.System1->Wait );
	MAINROM5.SetMemory ( "SYS1-5", SysRom1( 5 ), MemTable.System1->Wait );
	MAINROM6.SetMemory ( "SYS1-6", SysRom1( 6 ), MemTable.System1->Wait );
	MAINROM7.SetMemory ( "SYS1-7", SysRom1( 7 ), MemTable.System1->Wait );
	
	// SR メニューROM
	SRMENROM0.SetMemory( "SYS2-0", SysRom2( 0 ), MemTable.System1->Wait );
	SRMENROM1.SetMemory( "SYS2-1", SysRom2( 1 ), MemTable.System1->Wait );
	
	// 音声合成ROM
	VOICEROM0.SetMemory( "SYS2-2", SysRom2( 2 ), MemTable.System1->Wait );
	VOICEROM1.SetMemory( "SYS2-3", SysRom2( 3 ), MemTable.System1->Wait );
	
	// 漢字ROM
	KANJIROM0.SetMemory( "SYS2-4", SysRom2( 4 ), MemTable.System1->Wait );
	KANJIROM1.SetMemory( "SYS2-5", SysRom2( 5 ), MemTable.System1->Wait );
	KANJIROM2.SetMemory( "SYS2-6", SysRom2( 6 ), MemTable.System1->Wait );
	KANJIROM3.SetMemory( "SYS2-7", SysRom2( 7 ), MemTable.System1->Wait );
	
	// CG ROM
	CGROM1.SetMemory   ( "CGROM1", CGRom1( 0 ),  MemTable.CGRom1->Wait );
	CGROM2.SetMemory   ( "CGROM2", CGRom1( 1 ),  MemTable.CGRom1->Wait );
	
	// 内部RAM
	int i = 0;
	for( auto &mb : InRamB ){
		mb.SetMemory( Stringf( "INRAM%d", i ), IntRam( i ), MemTable.IntRam->Wait );
		i++;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// リセット
////////////////////////////////////////////////////////////////
void MEM6::Reset()
{
	PRINTD( MEM_LOG, "[MEM][Reset]\n" );
	
	CGBank = false;	// CG ROM BANK 無効
	
	// メモリコントローラ内部レジスタ初期値設定
	Rf = { INIT_RF0, INIT_RF1, INIT_RF2 };
	
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
	
	// 拡張カートリッジ ----------------------------------------
	switch( ExCart ){
	case EXC660101:		// 拡張漢字ROMカートリッジ
	case EXC6007SR:		// 拡張漢字ROM&RAMカートリッジ
		Kenable = false;
		break;
		
	case EXCSOL1:		// 戦士のカートリッジ
		// メモリバンク初期化
		SetSolBank( 2, ROMBANK );
		break;
		
	case EXCSOL2:		// 戦士のカートリッジmkⅡ
	case EXCSOL3:		// 戦士のカートリッジmkⅢ
		Sol60Mode = false;
		CS01R     = false;
		CS01W     = false;
		
		// メモリバンク初期化
		for( int i=0; i<8; i++ ){
			const BYTE bk2[] = { NONBANK,     NONBANK,     ROMBANK | 0, RAMBANK | 3,
								 RAMBANK | 4, RAMBANK | 5, NONBANK,     NONBANK      };
			SetSolBank( i, bk2[i] );
		}
		break;
	}
	// ---------------------------------------------------------
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
	BYTE data = vm->VdgIsSRmode() ? RD_BlkSR[addr>>MemBlock::PAGEBITS]->Read( addr )
								  : RD_Blk  [addr>>MemBlock::PAGEBITS]->Read( addr );
	
	PRINTD( MEM_LOG, "[MEM][Fetch] -> %04X:%02X\n", addr, data );
	
	// M1ウェイト追加
	if( m1wait ){ (*m1wait) += M1Wait; }
	
	// バスリクエスト区間実行時ウェイト追加
	if( vm->VdgIsBusReqExec() ){ (*m1wait)++; }
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリリード
////////////////////////////////////////////////////////////////
BYTE MEM6::Read( WORD addr, int* wcnt ) const
{
	BYTE data = 0xff;
	
	if( vm->VdgIsSRmode() ){
		if( vm->VdgIsSRBitmap( addr ) && (RfSR[addr>>MemBlock::PAGEBITS] == 0) ){	// ビットマップモード(内部RAMアクセス)
			WORD ad = vm->VdgSRGVramAddr( addr );
			data = ((addr & 1) ? (IntRam.Read( ad ) >> 4) : IntRam.Read( ad )) & 0x0f;
		}else{														// 直接アクセスモード
			data = RD_BlkSR[addr>>MemBlock::PAGEBITS]->Read( addr, wcnt );
		}
	}else{
		data = RD_Blk[addr>>MemBlock::PAGEBITS]->Read( addr, wcnt );
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){ (*wcnt)++; }
	
	PRINTD( MEM_LOG, "[MEM][Read]  -> %04X:%02X\n", addr, data );
	
	return data;
}


////////////////////////////////////////////////////////////////
// メモリライト
////////////////////////////////////////////////////////////////
void MEM6::Write( WORD addr, BYTE data, int* wcnt )
{
	PRINTD( MEM_LOG, "[MEM][Write] %04X:%02X -> %s[%04X]'%c'\n", addr, data, vm->VdgIsSRmode() ? WR_BlkSR[addr>>MemBlock::PAGEBITS]->GetName().c_str() : WR_Blk[addr>>MemBlock::PAGEBITS]->GetName().c_str(), addr&0x1fff, data );
	
	if( vm->VdgIsSRmode() ){
		if( vm->VdgIsSRBitmap( addr ) && (RfSR[(addr>>MemBlock::PAGEBITS)+8] == 0) ){	// ビットマップモード(内部RAMアクセス)
			WORD ad = vm->VdgSRGVramAddr( addr );
			IntRam.Write( ad, (addr & 1) ? ((IntRam.Read( ad ) & 0x0f) | ((data << 4) & 0xf0)) : ((IntRam.Read( ad ) & 0xf0) | ( data & 0x0f)) );
		}else{															// 直接アクセスモード
			WR_BlkSR[addr>>MemBlock::PAGEBITS]->Write( addr, data, wcnt );
		}
	}else{
		WR_Blk[addr>>MemBlock::PAGEBITS]->Write( addr, data, wcnt );
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){ (*wcnt)++; }
	
	// 内部/外部RAMとも書込みの場合はひとまず内部だけ
}


////////////////////////////////////////////////////////////////
// メモリアクセスウェイト設定
////////////////////////////////////////////////////////////////
void MEM6::SetWait( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetWait] -> M1:%d ROM:%d RAM:%d\n", (data>>7)&1, (data>>6)&1, (data>>5)&1 );
	
	// M1
	M1Wait = data & 0x80 ? 1 : 0;
	
	// ROM
	std::vector<MemBlock> mbrom = {
		EMPTYROM, EXTROM0, EXTROM1, MAINROM0, MAINROM1, MAINROM2, MAINROM3,
		KANJIROM0, KANJIROM1, KANJIROM2, KANJIROM3, VOICEROM0, VOICEROM1,
		MAINROM4, MAINROM5, MAINROM6, MAINROM7,	SRMENROM0, SRMENROM1	// SR
	};
	for( auto &mb : mbrom ){
		mb.SetWait ( data & 0x40 ? 1 : 0 );
	}
	
	// RAM
	std::vector<MemBlock> mbram = {
		EMPTYRAM,
		INTRAM0, INTRAM1, INTRAM2, INTRAM3, INTRAM4, INTRAM5, INTRAM6, INTRAM7,
		EXTRAM0, EXTRAM1, EXTRAM2, EXTRAM3, EXTRAM4, EXTRAM5, EXTRAM6, EXTRAM7
	};
	for( auto &mb : mbram ){
		mb.SetWait ( data & 0x20 ? 1 : 0 );
	}
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
	
	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = &MAINROM1;
	RD_Blk[2] = &EMPTYROM;	RD_Blk[3] = &EMPTYROM;
	RD_Blk[4] = &EMPTYROM;	RD_Blk[5] = &EMPTYROM;
	RD_Blk[6] = &INTRAM0;	RD_Blk[7] = &INTRAM1;
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	
	// 拡張カートリッジ ----------------------------------------
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM1;
		RD_Blk[4] = &EXTRAM0;	RD_Blk[5] = &EXTRAM1;
		break;
		
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		RD_Blk[4] = &EXTRAM4;	RD_Blk[5] = &EXTRAM5;
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
		RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM1;
		RD_Blk[4] = &EXTRAM4;	RD_Blk[5] = &EXTRAM5;
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		RD_Blk[2] = &EXTRAM2;	RD_Blk[3] = &EXTRAM3;
		RD_Blk[4] = &EXTRAM4;	RD_Blk[5] = &EXTRAM5;
		break;
	}
	// ---------------------------------------------------------
	
	// CG Rom
	if( CGBank ){
		RD_Blk[3] = &CGROM1;
	}
	
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName().c_str(), i+1, RD_Blk[i+1]->GetName().c_str() );
	}
	#endif
}

void MEM62::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR] -> %02X %02X\n", mem1, mem2 );
	
	// Port F0H
	switch( mem1 & 0x0f ){	// RF0下位 (0000 - 3FFF)
		case 0x00:	RD_Blk[0] = &EMPTYROM;	RD_Blk[1] = &EMPTYROM;	break;
		case 0x01:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = &MAINROM1;	break;
		case 0x02:	RD_Blk[0] = pKNJROM2;	RD_Blk[1] = pKNJROM3;	break;
		case 0x03:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &EXTROM1;	break;
		case 0x04:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = &EXTROM0;	break;
		case 0x05:	RD_Blk[0] = pKNJROM2;	RD_Blk[1] = &MAINROM1;	break;
		case 0x06:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = pKNJROM3;	break;
		case 0x07:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = &EXTROM1;	break;
		case 0x08:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &EXTROM0;	break;
		case 0x09:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &MAINROM1;	break;
		case 0x0a:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = &EXTROM1;	break;
		case 0x0b:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = pKNJROM3;	break;
		case 0x0c:	RD_Blk[0] = pKNJROM2;	RD_Blk[1] = &EXTROM0;	break;
		case 0x0d:	RD_Blk[0] = &INTRAM0;	RD_Blk[1] = &INTRAM1;	break;
		case 0x0e:	RD_Blk[0] = &EXTRAM0;	RD_Blk[1] = &EXTRAM1;	break;
		case 0x0f:	RD_Blk[0] = &EMPTYROM;	RD_Blk[1] = &EMPTYROM;	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	RD_Blk[2] = &EMPTYROM;	RD_Blk[3] = &EMPTYROM;	break;
		case 0x10:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = &MAINROM3;	break;
		case 0x20:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = pKNJROM3;	break;
		case 0x30:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &EXTROM1;	break;
		case 0x40:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM0;	break;
		case 0x50:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = &MAINROM3;	break;
		case 0x60:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = pKNJROM3;	break;
		case 0x70:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM1;	break;
		case 0x80:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &EXTROM0;	break;
		case 0x90:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &MAINROM3;	break;
		case 0xa0:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = &EXTROM1;	break;
		case 0xb0:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = pKNJROM3;	break;
		case 0xc0:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = &EXTROM0;	break;
		case 0xd0:	RD_Blk[2] = &INTRAM2;	RD_Blk[3] = &INTRAM3;	break;
		case 0xe0:	RD_Blk[2] = &EXTRAM2;	RD_Blk[3] = &EXTRAM3;	break;
		case 0xf0:	RD_Blk[2] = &EMPTYROM;	RD_Blk[3] = &EMPTYROM;	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	RD_Blk[4] = &EMPTYROM;	RD_Blk[5] = &EMPTYROM;	break;
		case 0x01:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = &MAINROM1;	break;
		case 0x02:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = pKNJROM3;	break;
		case 0x03:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &EXTROM1;	break;
		case 0x04:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = &EXTROM0;	break;
		case 0x05:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = &MAINROM1;	break;
		case 0x06:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = pKNJROM3;	break;
		case 0x07:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = &EXTROM1;	break;
		case 0x08:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &EXTROM0;	break;
		case 0x09:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &MAINROM1;	break;
		case 0x0a:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = &EXTROM1;	break;
		case 0x0b:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = pKNJROM3;	break;
		case 0x0c:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = &EXTROM0;	break;
		case 0x0d:	RD_Blk[4] = &INTRAM4;	RD_Blk[5] = &INTRAM5;	break;
		case 0x0e:	RD_Blk[4] = &EXTRAM4;	RD_Blk[5] = &EXTRAM5;	break;
		case 0x0f:	RD_Blk[4] = &EMPTYROM;	RD_Blk[5] = &EMPTYROM;	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	RD_Blk[6] = &EMPTYROM;	RD_Blk[7] = &EMPTYROM;	break;
		case 0x10:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = &MAINROM3;	break;
		case 0x20:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = pKNJROM3;	break;
		case 0x30:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &EXTROM1;	break;
		case 0x40:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = &EXTROM0;	break;
		case 0x50:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = &MAINROM3;	break;
		case 0x60:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = pKNJROM3;	break;
		case 0x70:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = &EXTROM1;	break;
		case 0x80:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &EXTROM0;	break;
		case 0x90:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &MAINROM3;	break;
		case 0xa0:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = &EXTROM1;	break;
		case 0xb0:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = pKNJROM3;	break;
		case 0xc0:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = &EXTROM0;	break;
		case 0xd0:	RD_Blk[6] = &INTRAM6;	RD_Blk[7] = &INTRAM7;	break;
		case 0xe0:	RD_Blk[6] = &EXTRAM6;	RD_Blk[7] = &EXTRAM7;	break;
		case 0xf0:	RD_Blk[6] = &EMPTYROM;	RD_Blk[7] = &EMPTYROM;	break;
	}
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	if( CGBank && cgenable ){
		for( int i=0; i<8; i++ ){
			if( ((cgaden & i) | (~cgaden & cgaddr)) == i )
				RD_Blk[i] = cgrom ? &CGROM1 : &CGROM2;
		}
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName().c_str(), i+1, RD_Blk[i+1]->GetName().c_str() );
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
		case 0x00:	RD_Blk[0] = &EMPTYROM;	RD_Blk[1] = &EMPTYROM;	break;
		case 0x01:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = &MAINROM1;	break;
		case 0x02:	RD_Blk[0] = pKNJROM0;	RD_Blk[1] = pKNJROM1;	break;	// ココ
		case 0x03:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &EXTROM1;	break;
		case 0x04:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = &EXTROM0;	break;
		case 0x05:	RD_Blk[0] = pKNJROM0;	RD_Blk[1] = &MAINROM1;	break;	// ココ
		case 0x06:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = pKNJROM1;	break;	// ココ
		case 0x07:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = &EXTROM1;	break;
		case 0x08:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &EXTROM0;	break;
		case 0x09:	RD_Blk[0] = &EXTROM1;	RD_Blk[1] = &MAINROM1;	break;
		case 0x0a:	RD_Blk[0] = &MAINROM0;	RD_Blk[1] = &EXTROM1;	break;
		case 0x0b:	RD_Blk[0] = &EXTROM0;	RD_Blk[1] = pKNJROM1;	break;	// ココ
		case 0x0c:	RD_Blk[0] = pKNJROM0;	RD_Blk[1] = &EXTROM0;	break;	// ココ
		case 0x0d:	RD_Blk[0] = &INTRAM0;	RD_Blk[1] = &INTRAM1;	break;
		case 0x0e:	RD_Blk[0] = &EXTRAM0;	RD_Blk[1] = &EXTRAM1;	break;
		case 0x0f:	RD_Blk[0] = &EMPTYROM;	RD_Blk[1] = &EMPTYROM;	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	RD_Blk[2] = &EMPTYROM;	RD_Blk[3] = &EMPTYROM;	break;
		case 0x10:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = &MAINROM3;	break;
		case 0x20:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = pKNJROM3;	break;
		case 0x30:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &EXTROM1;	break;
		case 0x40:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM0;	break;
		case 0x50:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = &MAINROM3;	break;
		case 0x60:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = pKNJROM3;	break;
		case 0x70:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = &EXTROM1;	break;
		case 0x80:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &EXTROM0;	break;
		case 0x90:	RD_Blk[2] = &EXTROM1;	RD_Blk[3] = &MAINROM3;	break;
		case 0xa0:	RD_Blk[2] = &MAINROM2;	RD_Blk[3] = &EXTROM1;	break;
		case 0xb0:	RD_Blk[2] = &EXTROM0;	RD_Blk[3] = pKNJROM3;	break;
		case 0xc0:	RD_Blk[2] = pKNJROM2;	RD_Blk[3] = &EXTROM0;	break;
		case 0xd0:	RD_Blk[2] = &INTRAM2;	RD_Blk[3] = &INTRAM3;	break;
		case 0xe0:	RD_Blk[2] = &EXTRAM2;	RD_Blk[3] = &EXTRAM3;	break;
		case 0xf0:	RD_Blk[2] = &EMPTYROM;	RD_Blk[3] = &EMPTYROM;	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	RD_Blk[4] = &EMPTYROM;	RD_Blk[5] = &EMPTYROM;	break;
		case 0x01:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = &MAINROM1;	break;
		case 0x02:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = pKNJROM3;	break;
		case 0x03:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &EXTROM1;	break;
		case 0x04:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = &EXTROM0;	break;
		case 0x05:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = &MAINROM1;	break;
		case 0x06:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = pKNJROM3;	break;
		case 0x07:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = &EXTROM1;	break;
		case 0x08:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &EXTROM0;	break;
		case 0x09:	RD_Blk[4] = &EXTROM1;	RD_Blk[5] = &MAINROM1;	break;
		case 0x0a:	RD_Blk[4] = &MAINROM0;	RD_Blk[5] = &EXTROM1;	break;
		case 0x0b:	RD_Blk[4] = &EXTROM0;	RD_Blk[5] = pKNJROM3;	break;
		case 0x0c:	RD_Blk[4] = pKNJROM2;	RD_Blk[5] = &EXTROM0;	break;
		case 0x0d:	RD_Blk[4] = &INTRAM4;	RD_Blk[5] = &INTRAM5;	break;
		case 0x0e:	RD_Blk[4] = &EXTRAM4;	RD_Blk[5] = &EXTRAM5;	break;
		case 0x0f:	RD_Blk[4] = &EMPTYROM;	RD_Blk[5] = &EMPTYROM;	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	RD_Blk[6] = &EMPTYROM;	RD_Blk[7] = &EMPTYROM;	break;
		case 0x10:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = &MAINROM3;	break;
		case 0x20:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = pKNJROM3;	break;
		case 0x30:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &EXTROM1;	break;
		case 0x40:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = &EXTROM0;	break;
		case 0x50:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = &MAINROM3;	break;
		case 0x60:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = pKNJROM3;	break;
		case 0x70:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = &EXTROM1;	break;
		case 0x80:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &EXTROM0;	break;
		case 0x90:	RD_Blk[6] = &EXTROM1;	RD_Blk[7] = &MAINROM3;	break;
		case 0xa0:	RD_Blk[6] = &MAINROM2;	RD_Blk[7] = &EXTROM1;	break;
		case 0xb0:	RD_Blk[6] = &EXTROM0;	RD_Blk[7] = pKNJROM3;	break;
		case 0xc0:	RD_Blk[6] = pKNJROM2;	RD_Blk[7] = &EXTROM0;	break;
		case 0xd0:	RD_Blk[6] = &INTRAM6;	RD_Blk[7] = &INTRAM7;	break;
		case 0xe0:	RD_Blk[6] = &EXTRAM6;	RD_Blk[7] = &EXTRAM7;	break;
		case 0xf0:	RD_Blk[6] = &EMPTYROM;	RD_Blk[7] = &EMPTYROM;	break;
	}
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	// (推定)SRはA13を無視して16KB単位でCG ROMが現れるようだ
	if( CGBank && cgenable ){
		for( int i=0; i<8; i++ ){
//			if( ((cgaden & i) | (~cgaden & cgaddr)) == i )
			if( (((cgaden|1) & i) | (~cgaden & cgaddr)) == i )
				RD_Blk[i] = cgrom ? &CGROM1 : &CGROM2;
		}
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName().c_str(), i+1, RD_Blk[i+1]->GetName().c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// メモリライト時のメモリブロック指定
////////////////////////////////////////////////////////////////
void MEM60::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW]\n" );
	
	WR_Blk[0] = &EMPTYROM;	WR_Blk[1] = &EMPTYROM;
	WR_Blk[2] = &EMPTYROM;	WR_Blk[3] = &EMPTYROM;
	WR_Blk[4] = &EMPTYROM;	WR_Blk[5] = &EMPTYROM;
	WR_Blk[6] = &INTRAM0;	WR_Blk[7] = &INTRAM1;
	
	// 内部レジスタ保存
	Rf[2] = data;
	
	
	// 拡張カートリッジ ----------------------------------------
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		WR_Blk[4] = &EXTRAM0;	WR_Blk[5] = &EXTRAM1;
		break;
		
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		WR_Blk[4] = &EXTRAM4;	WR_Blk[5] = &EXTRAM5;
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		WR_Blk[0] = &MAINROM0;	WR_Blk[1] = &MAINROM1;
		WR_Blk[2] = &EXTRAM2;	WR_Blk[3] = &EXTRAM3;
		WR_Blk[4] = &EXTRAM4;	WR_Blk[5] = &EXTRAM5;
		break;
	}
	// ---------------------------------------------------------
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, WR_Blk[i]->GetName().c_str(), i+1, WR_Blk[i+1]->GetName().c_str() );
	}
	#endif
}

void MEM62::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW] -> %02X\n", data );
	
	switch( data & 3 ){			// 0000 - 3FFF
		case 0: WR_Blk[0] = &EMPTYRAM;	WR_Blk[1] = &EMPTYRAM;	break;
		case 1: WR_Blk[0] = &INTRAM0;	WR_Blk[1] = &INTRAM1;	break;
		case 2: WR_Blk[0] = &EXTRAM0;	WR_Blk[1] = &EXTRAM1;	break;
		case 3: WR_Blk[0] = &INEXRAM;	WR_Blk[1] = &INEXRAM;	break;
	}
	switch( (data>>2) & 3 ){	// 4000 - 7FFF
		case 0: WR_Blk[2] = &EMPTYRAM;	WR_Blk[3] = &EMPTYRAM;	break;
		case 1: WR_Blk[2] = &INTRAM2;	WR_Blk[3] = &INTRAM3;	break;
		case 2: WR_Blk[2] = &EXTRAM2;	WR_Blk[3] = &EXTRAM3;	break;
		case 3: WR_Blk[2] = &INEXRAM;	WR_Blk[3] = &INEXRAM;	break;
	}
	switch( (data>>4) & 3 ){	// 8000 - BFFF
		case 0: WR_Blk[4] = &EMPTYRAM;	WR_Blk[5] = &EMPTYRAM;	break;
		case 1: WR_Blk[4] = &INTRAM4;	WR_Blk[5] = &INTRAM5;	break;
		case 2: WR_Blk[4] = &EXTRAM4;	WR_Blk[5] = &EXTRAM5;	break;
		case 3: WR_Blk[4] = &INEXRAM;	WR_Blk[5] = &INEXRAM;	break;
	}
	switch( (data>>6) & 3 ){	// C000 - FFFF
		case 0: WR_Blk[6] = &EMPTYRAM;	WR_Blk[7] = &EMPTYRAM;	break;
		case 1: WR_Blk[6] = &INTRAM6;	WR_Blk[7] = &INTRAM7;	break;
		case 2: WR_Blk[6] = &EXTRAM6;	WR_Blk[7] = &EXTRAM7;	break;
		case 3: WR_Blk[6] = &INEXRAM;	WR_Blk[7] = &INEXRAM;	break;
	}
	
	// 内部レジスタ保存
	Rf[2] = data;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, WR_Blk[i]->GetName().c_str(), i+1, WR_Blk[i+1]->GetName().c_str() );
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
	if( port&0x08 ) mb = &WR_BlkSR[port&0x07];	// 8-F : Write
	else			mb = &RD_BlkSR[port&0x07];	// 0-7 : Read
	
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
		PRINTD( MEM_LOG, "               %d:%8s\t%8s\n", i, RD_BlkSR[i]->GetName().c_str(), WR_BlkSR[i]->GetName().c_str() );
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
	SetMemBlockR( Rf[0], Rf[1] );
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
	int area = port & 7;
	
	SolBank[area] = data;
	
	switch( data & 0xc0 ){
	case ROMBANK:	// ROM
		SolBankSet = data & 0x30;
		ExRamB[area].SetMemory( Stringf( "EROM%02d", data ), ExtRom( data ) );
		
		// Bank Setが変わった場合の修正
		for( int i=0; i<8; i++ ){
			if( ((SolBank[i] & 0xc0) == ROMBANK) && ((SolBank[i] & 0x30) != SolBankSet) ){
				SolBank[i] = (SolBank[i] & 0x0f) | SolBankSet | ROMBANK;
				ExRamB[i].SetMemory( nullptr, ExtRom( SolBank[i] & 0x3f ) );
			}
		}
		break;
		
	case RAMBANK:	// RAM
		ExRamB[area].SetMemory( Stringf( "ERAM%02d", data & (ExtRam.Size()-1) ), ExtRam( data & (ExtRam.Size()-1) ) );
		break;
		
	case SCCBANK:	// SCC
		ExRamB[area].SetFunc( "SCC", FR( STATIC_CAST( RFuncPtr, &MEM6::SolSccRead ) ), FW( STATIC_CAST( WFuncPtr, &MEM6::SolSccWrite ) ) );
		break;
		
	case NONBANK:	// 無効
	default:
		ExRamB[area].SetFunc( "EMPTY", nullptr, nullptr );
	}
}


////////////////////////////////////////////////////////////////
// 直接アクセス関数
////////////////////////////////////////////////////////////////
BYTE MEM6::ReadMainRom  ( WORD addr ) const { return SysRom1.Read ( addr ); }
BYTE MEM6::ReadIntRam   ( WORD addr ) const { return IntRam.Read  ( addr ); }
BYTE MEM6::ReadExtRom   ( WORD addr ) const { return ExtRom.Read  ( addr ); }
BYTE MEM6::ReadExtRam   ( WORD addr ) const { return ExtRam.Read  ( addr ); }

BYTE MEM6::ReadCGrom1   ( WORD addr ) const { return CGRom1.Read  ( addr & 0x1fff ); }
BYTE MEM6::ReadCGrom2   ( WORD )      const { return 0xff; }
BYTE MEM6::ReadCGrom3   ( WORD )      const { return 0xff; }

BYTE MEM62::ReadCGrom2  ( WORD addr ) const { return CGRom2.Read  ( addr ); }
BYTE MEM62::ReadKanjiRom( WORD addr ) const { return KanjiRom.Read( addr ); }
BYTE MEM62::ReadVoiceRom( WORD addr ) const { return VoiceRom.Read( addr ); }

BYTE MEM64::ReadCGrom1  ( WORD addr ) const { return CGRom1.Read  (  addr & 0x0fff           ); }
BYTE MEM64::ReadCGrom2  ( WORD addr ) const { return CGRom1.Read  ( (addr & 0x1fff) + 0x2000 ); }
BYTE MEM64::ReadCGrom3  ( WORD addr ) const { return CGRom1.Read  ( (addr & 0x0fff) + 0x1000 ); }
BYTE MEM64::ReadKanjiRom( WORD addr ) const { return SysRom2.Read ( (addr & 0x7fff) + 0x8000 ); }
BYTE MEM64::ReadVoiceRom( WORD addr ) const { return SysRom2.Read ( (addr & 0x3fff) + 0x4000 ); }


////////////////////////////////////////////////////////////////
// I/Oアクセス関数
////////////////////////////////////////////////////////////////
// 拡張カートリッジ --------------------------------------------
// 戦士のカートリッジ --------------------------------------------------------------------
void MEM6::Out06H( int, BYTE data ){ Sol60Mode = (data == 0x66) ? true : false; }
void MEM6::Out3xH( int port, BYTE data ){ SetSolBank( port, data ); }
void MEM6::Out7FH( int, BYTE data ){ SetSolBank( 2, ROMBANK | (data & 0xf) ); }

void MEM6::OutF0Hs( int, BYTE data )
{
	switch( data & 0x0f ){	// RF0下位 (0000 - 3FFF)
	case 0x01:
	case 0x02:
	case 0x05:
	case 0x06:
		CS01R = false;
		break;
		
	default:
		CS01R = true;
	}
}

void MEM6::OutF2Hs( int, BYTE data )
{
	switch( data & 3 ){	// 0000 - 3FFF
	case 0:
		CS01W = false;
		break;
		
	default:
		CS01W = true;
	}
}

// 拡張漢字ROMカートリッジ ---------------------------------------------------------------
void MEM6::OutFCH( int port, BYTE data )
{
	Kaddr   = (data << 8) | ((port >> 8) & 0xff);
	Kenable = false;
}

void MEM6::OutFFH( int, BYTE ){ Kenable = !Kenable; }
BYTE MEM6::InFDH( int ){ return Kenable ? ExtRom.Read( (Kaddr << 1)     ) : 0xff; }
BYTE MEM6::InFEH( int ){ return Kenable ? ExtRom.Read( (Kaddr << 1) | 1 ) : 0xff; }
// -------------------------------------------------------------

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
	Ini->SetVal( "MEMORY", "M1Wait",		"", M1Wait );
	Ini->SetVal( "MEMORY", "ExCart",		"", ExCart );
	
	// 62,66,64,68
	Ini->SetVal( "MEMORY", "cgrom",			"", cgrom    );
	Ini->SetVal( "MEMORY", "kj_rom",		"", kj_rom   );
	Ini->SetVal( "MEMORY", "kj_LR",			"", kj_LR    );
	Ini->SetVal( "MEMORY", "cgenable",		"", cgenable );
	Ini->SetVal( "MEMORY", "cgaden",		"", cgaden );
	Ini->SetVal( "MEMORY", "cgaddr",		"", cgaddr );
	for( int i=0; i<3; i++ ){
		Ini->SetVal( "MEMORY", Stringf( "Rf%d", i ), "", "0x%02X", Rf[i] );
	}
	
	// メモリウェイト
	Ini->SetVal( "MEMORY", "Wait",		"", GetWait() );
	// CGRomウェイト
	Ini->SetVal( "MEMORY", "CgRomWait",	"", CGROM1.GetWait() );
	
	// 内部RAM
	for( int i=0; i<(int)MemTable.IntRam->Size; i+=64 ){
		std::string strva;
		for( int j=0; j<64; j++ ){
			strva += Stringf( "%02X", IntRam.Read( i+j ) );
		}
		Ini->SetEntry( "MEMORY", Stringf( "IntRam_%04X", i ), "", strva.c_str() );
	}
	
	// 拡張カートリッジ ----------------------------------------
	
	// 外部ROM
	if( ReadyExRom ){
		P6VPATH tpath = FilePath;
		OSD_RelativePath( tpath );
		Ini->SetVal( "MEMORY", "FilePath",	"", tpath );
	}
	
	// 外部RAM
	if( ExCart & EXCRAM ){
		for( int i=0; i<(int)MemTable.ExtRam->Size; i+=64 ){
			std::string strva;
			for( int j=0; j<64; j++ ){
				strva += Stringf( "%02X", ExtRam.Read( i+j ) );
			}
			Ini->SetEntry( "MEMORY", Stringf( "ExtRam_%06X", i ), "", strva.c_str() );
		}
	}
	
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		Ini->SetVal( "MEMORY", "Kaddr",			"", Kaddr      );
		Ini->SetVal( "MEMORY", "Kenable",		"", Kenable    );
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		Ini->SetVal( "MEMORY", "Soldier60",		"", Sol60Mode  );
		Ini->SetVal( "MEMORY", "CS01R",			"", CS01R      );
		Ini->SetVal( "MEMORY", "CS01W",			"", CS01W      );
		Ini->SetVal( "MEMORY", "SoldierBank",	"", SolBankSet );
		// メモリバンクレジスタ
		for( int i=0; i<8; i++ ){
			Ini->SetVal( "MEMORY", Stringf( "SolBank%d", i ), "", "0x%02X", SolBank[i] );
		}
		break;
	}
	// ---------------------------------------------------------
	
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
	
	Ini->GetVal( "MEMORY", "CGBank",		CGBank   );
	Ini->GetVal( "MEMORY", "M1Wait",		M1Wait   );
	Ini->GetVal( "MEMORY", "ExCart",		ExCart   );
	
	// 62,66,64,68
	Ini->GetVal( "MEMORY", "cgrom",			cgrom    );
	Ini->GetVal( "MEMORY", "kj_rom",		kj_rom   );
	Ini->GetVal( "MEMORY", "kj_LR",			kj_LR    );
	Ini->GetVal( "MEMORY", "cgenable",		cgenable );
	Ini->GetVal( "MEMORY", "cgaden",		cgaden   );
	Ini->GetVal( "MEMORY", "cgaddr",		cgaddr   );
	for( int i=0; i<3; i++ ){
		Ini->GetVal( "MEMORY", Stringf( "Rf%d", i ), Rf[i] );
	}
	
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
			strva.resize( 64 * 2, '0' );
			for( int j=0; j<64; j++ ){
				IntRam.Write( i+j, std::stoul( strva.substr( j * 2, 2 ), nullptr, 16 ) );
			}
		}
	}
	
	// 拡張カートリッジ ----------------------------------------
	
	// 拡張ROM
	if( Ini->GetVal( "MEMORY", "FilePath", tpath ) )
		MountExtRom( tpath );
	
	// 外部RAM
	if( ExCart & EXCRAM ){
		for( int i=0; i<(int)MemTable.ExtRam->Size; i+=64 ){
			std::string strva;
			if( Ini->GetEntry( "MEMORY", Stringf( "ExtRam_%06X", i ), strva ) ){
				strva.resize( 64 * 2, '0' );
				for( int j=0; j<64; j++ ){
					ExtRam.Write( i+j, std::stoul( strva.substr( j * 2, 2 ), nullptr, 16 ) );
				}
			}
		}
	}
	
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		Ini->GetVal( "MEMORY", "Kaddr",			Kaddr      );
		Ini->GetVal( "MEMORY", "Kenable",		Kenable    );
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		Ini->GetVal( "MEMORY", "Soldier60",		Sol60Mode  );
		Ini->GetVal( "MEMORY", "CS01R",			CS01R      );
		Ini->GetVal( "MEMORY", "CS01W",			CS01W      );
		Ini->GetVal( "MEMORY", "SoldierBank",	SolBankSet );
		// メモリバンクレジスタ
		for( int i=0; i<8; i++ ){
			Ini->GetVal( "MEMORY", Stringf( "SolBank%d", i ), SolBank[i] );
			SetSolBank( i, SolBank[i] );	// メモリバンク設定
		}
		break;
	}
	// ---------------------------------------------------------
	
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
	return vm->VdgIsSRmode() ? RD_BlkSR[blk]->GetName()
							 : RD_Blk  [blk]->GetName();
}

const std::string& MEM6::GetWriteMemBlk( int blk ) const 
{
	return vm->VdgIsSRmode() ? WR_BlkSR[blk]->GetName()
							 : WR_Blk  [blk]->GetName();
}
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

