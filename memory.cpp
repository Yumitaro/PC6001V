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


// メモリブロック割り当て
// 内部ROM
enum {
	EMPTYROM = 0,	// EmptyRom
	EMPTYRAM,		// EmptyRam
	MAINROM,		// SysRom1
	MAINROM1,		// SysRom1+0x2000
	MAINROM2,		// SysRom1+0x4000
	MAINROM3,		// SysRom1+0x6000
	MAINROM4,		// SysRom1+0x8000
	MAINROM5,		// SysRom1+0xa000
	MAINROM6,		// SysRom1+0xc000
	MAINROM7,		// SysRom1+0xe000
	CGROM1,			// CGRom1
	CGROM2,			// CGRom2			CGRom1+0x2000
	SRMENROM,		// MenuRom			SysRom2
	SRMENROM1,		// MenuRom+0x2000	SysRom2+0x2000
	VOICEROM,		// VoiceRom			SysRom2+0x4000
	VOICEROM1,		// VoiceRom+0x2000	SysRom2+0x6000
	KANJIROM,		// KanjiRom			SysRom2+0x8000
	KANJIROM1,		// KanjiRom+0x2000	SysRom2+0xa000
	KANJIROM2,		// KanjiRom+0x4000	SysRom2+0xc000
	KANJIROM3,		// KanjiRom+0x6000	SysRom2+0xe000
	
	RWCOMMON,		// 汎用読込み/書込み
	RWAREAA60,		// AreaA,B読込み/書込み(60)
	RWAREAC60,		// AreaC読込み/書込み(60)
	RWAREAD60,		// AreaD読込み/書込み(60)
	KNJROM,			// 漢字ROM選択用
	
	EndofIROM
};

// 内部RAM
enum {
	INTRAM = 0,		// IntRam
	INTRAM1,		// IntRam+0x2000
	INTRAM2,		// IntRam+0x4000
	INTRAM3,		// IntRam+0x6000
	INTRAM4,		// IntRam+0x8000
	INTRAM5,		// IntRam+0xa000
	INTRAM6,		// IntRam+0xc000
	INTRAM7,		// IntRam+0xe000
	
	INEXRAM,		// IntRam+ExtRam
	
	EndofIRAM
};

// 外部ROM
enum {
	EXTROM = 0,		// ExtRom
	EXTROM1,		// ExtRom+0x2000
	
	EndofEROM
};

// 外部RAM
enum {
	EXTRAM = 0,		// ExtRam
	EXTRAM1,		// ExtRam+0x2000
	EXTRAM2,		// ExtRam+0x4000
	EXTRAM3,		// ExtRam+0x6000
	EXTRAM4,		// ExtRam+0x8000
	EXTRAM5,		// ExtRam+0xa000
	EXTRAM6,		// ExtRam+0xc000
	EXTRAM7,		// ExtRam+0xe000
	
	EndofERAM
};


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
const std::vector<ROMINFO> EXBASIC00 = { { "EXBASIC.ROM",	0x02000,	0          } };	// CRCは後で調べる
const std::vector<ROMINFO> EXKANJI00 = { { "EXKANJI.ROM",	0x020000,	0          } };	// 自作フォントを想定してCRCなし？ただし存在チェックで0x2c9(16bit)のデータを0x4141に固定する必要あり
const std::vector<ROMINFO> EXVOICE00 = { { "EXVOICE.ROM",	0x04000,	0          } };	// CRCは後で調べる



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
// メモリ情報
////////////////////////////////////////////////////////////////
// 共通									 ROM情報	サイズ		初期値	Wait
const MEM6::MEMINFO MEM6::IEMPTROM   = { NOROM,		0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEMPTRAM   = { NOROM,		0x002000,	0xff,	0 };
const MEM6::MEMINFO MEM6::IEXTROM8   = { NOROM,		0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM16  = { NOROM,		0x004000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM128 = { NOROM,		0x020000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM512 = { NOROM,		0x080000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTROM8M  = { NOROM,		0x800000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXTRAM16  = { NOROM,		0x004000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM32  = { NOROM,		0x008000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM64  = { NOROM,		0x010000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM128 = { NOROM,		0x020000,	0x00,	0 };
const MEM6::MEMINFO MEM6::IEXTRAM512 = { NOROM,		0x080000,	0x00,	0 };

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

// 拡張カートリッジ
const MEM6::MEMINFO MEM6::IEXBASIC   = { EXBASIC00,	0x002000,	0xff,	1 };
const MEM6::MEMINFO MEM6::IEXKANJI   = { EXKANJI00,	0x020000,	0xff,	0 };
const MEM6::MEMINFO MEM6::IEXVOICE   = { EXVOICE00,	0x004000,	0xff,	1 };



// ダミーメモリセル
MemCell EmptyCell( 0xff, true );






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
// CRC32計算
//
// 引数:	buf				データバッファへのポインタ
//			num				データ数(バイト)
// 返値:	DWORD			CRC32値
////////////////////////////////////////////////////////////////
DWORD CalcCrc32( MemCells& buf, int num )
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
// 引数:	filepath	ファイルパス名への参照
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
MemBlock::MemBlock( void ) : Name( "" ), PMem( &EmptyCell ), FName( nullptr ), FRead( nullptr ), FWrite( nullptr ), Wait( 0 )
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
	FName  = nullptr;
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
	FName  = nullptr;
	FRead  = rd;
	FWrite = wr;
	Wait   = wait < 0 ? Wait : wait;
}

// 引数:	name	メモリブロック名取得関数ポインタ(bind)
//			rd		読込み関数ポインタ(bind)
//			wr		書込み関数ポインタ(bind)
//			wait	アクセスウェイト(-1:変更しない)
// 返値:	なし
void MemBlock::SetFunc( NFunc nm, RFunc rd, WFunc wr, int wait )
{
	Name   = "";
	PMem   = &EmptyCell;
	FName  = nm;
	FRead  = rd;
	FWrite = wr;
	Wait   = wait < 0 ? Wait : wait;
}


////////////////////////////////////////////////////////////////
// メモリブロック名取得
//
// 引数:	addr			割当先のアドレス(0000-FFFF)
//			type			true:Read false:Write
// 返値:	std::string& 	メモリブロック名への参照
////////////////////////////////////////////////////////////////
const std::string& MemBlock::GetName( WORD addr, bool type ) const
{
	return FName ? FName( addr, type ) : Name;
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
	if( FRead ){
		return FRead( PMem, addr, wcnt );
	}else if( PMem->Size() ){
		if( wcnt ){ *wcnt += Wait; }
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
	if( FWrite ){
		FWrite( PMem, addr, data, wcnt );
	}else if( PMem->Size() ){
		if( wcnt ){	*wcnt += Wait; }
		PMem->Write( addr, data );
	}
}







////////////////////////////////////////////////////////////////
// 拡張カートリッジ デバイスディスクリプタ追加
////////////////////////////////////////////////////////////////
void MEM6::AddDeviceDescriptorExt( void )
{
	switch( ExCart ){
	case EXC660101:	// 拡張漢字ROMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		descs.outdef.emplace( outFCH,  STATIC_CAST( Device::OutFuncPtr, &MEM6::OutFCH  ) );
		descs.outdef.emplace( outFFH,  STATIC_CAST( Device::OutFuncPtr, &MEM6::OutFFH  ) );
		
		descs.indef.emplace ( inFDH,   STATIC_CAST( Device::InFuncPtr,  &MEM6::InFDH   ) );
		descs.indef.emplace ( inFEH,   STATIC_CAST( Device::InFuncPtr,  &MEM6::InFEH   ) );
		break;
		
	case EXC6053:	// ボイスシンセサイザー
		descs.outdef.emplace( out70H,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out70H  ) );
		descs.outdef.emplace( out72H,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out72H  ) );
		descs.outdef.emplace( out73H,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out73H  ) );
		descs.outdef.emplace( out74H,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out74H  ) );
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		descs.outdef.emplace( out06H,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out06H  ) );
		descs.outdef.emplace( out3xH,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out3xH  ) );
		descs.outdef.emplace( outF0Hs, STATIC_CAST( Device::OutFuncPtr, &MEM6::OutF0Hs ) );	// 戦士のカートリッジ 60対応
		descs.outdef.emplace( outF2Hs, STATIC_CAST( Device::OutFuncPtr, &MEM6::OutF2Hs ) );	// 戦士のカートリッジ 60対応
		[[fallthrough]];
		
	case EXCSOL1:	// 戦士のカートリッジ
		descs.outdef.emplace( out7FH,  STATIC_CAST( Device::OutFuncPtr, &MEM6::Out7FH  ) );
		break;
	}
}


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
MEM6::MEM6( VM6* vm, const ID& id ) : Device( vm, id ),
	CGBank( false ), M1Wait( 1 ),
	// 拡張カートリッジ ======================================================================
	ExCart( 0 ), FilePath( "" ),
	Kaddr( 0 ), Kenable( false ),
	Sol60Mode( false ), CS01R( false ), CS01W( false ), CS02W( false ), SolBankSet( 0 )
	// =======================================================================================
{
	IRom.assign( EndofIROM, MemBlock() );
	IRam.assign( EndofIRAM, MemBlock() );
	
	RBLK.fill  ( &IRom[EMPTYROM] );
	WBLK.fill  ( &IRom[EMPTYROM] );
	RD_Blk.fill( &IRom[EMPTYROM] );
	WR_Blk.fill( &IRom[EMPTYROM] );
	
	Rf = { INIT_RF0, INIT_RF1, INIT_RF2 };
	
	descs.outdef.clear();
	descs.indef.clear();
	
	
	// 拡張カートリッジ ======================================================================
	ERom.assign( EndofEROM, MemBlock() );
	ERam.assign( EndofERAM, MemBlock() );
	// =======================================================================================
}

MEM60::MEM60( VM6* vm, const ID& id ) : MEM6( vm, id )
{
	MemTable.System1 = &MEM60::ISYSROM1;
	MemTable.CGRom1  = &MEM60::ICGROM1;
	MemTable.IntRam  = &MEM60::IINTRAM;
}

MEM61::MEM61( VM6* vm, const ID& id ) : MEM60( vm, id )
{
	MemTable.System1 = &MEM61::ISYSROM1;
	MemTable.CGRom1  = &MEM61::ICGROM1;
	MemTable.IntRam  = &MEM61::IINTRAM;
}

MEM62::MEM62( VM6* vm, const ID& id ) : MEM6( vm, id ),
	cgrom( true ), kj_rom( true ), kj_LR( true ), cgenable( true ), cgaden( 7 ), cgaddr( 3 ), c2acc( 0xff )
{
	MemTable.System1 = &MEM62::ISYSROM1;
	MemTable.CGRom1  = &MEM62::ICGROM1;
	MemTable.CGRom2  = &MEM62::ICGROM2;
	MemTable.Kanji   = &MEM62::IKANJI;
	MemTable.Voice   = &MEM62::IVOICE;
	MemTable.IntRam  = &MEM62::IINTRAM;
	
	// Dvice Description (Out)
	descs.outdef.emplace( outC1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC1H ) );
	descs.outdef.emplace( outC2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC2H ) );
	descs.outdef.emplace( outC3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutC3H ) );
	descs.outdef.emplace( outF0H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF0H ) );
	descs.outdef.emplace( outF1H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF1H ) );
	descs.outdef.emplace( outF2H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF2H ) );
	descs.outdef.emplace( outF3H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF3H ) );
	descs.outdef.emplace( outF8H, STATIC_CAST( Device::OutFuncPtr, &MEM62::OutF8H ) );
	
	// Dvice Description (In)
	descs.indef.emplace ( inC2H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InC2H  ) );
	descs.indef.emplace ( inF0H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF0H  ) );
	descs.indef.emplace ( inF1H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF1H  ) );
	descs.indef.emplace ( inF2H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF2H  ) );
	descs.indef.emplace ( inF3H,  STATIC_CAST( Device::InFuncPtr,  &MEM62::InF3H  ) );
}

MEM66::MEM66( VM6* vm, const ID& id ) : MEM62( vm, id )
{
	MemTable.System1 = &MEM66::ISYSROM1;
	MemTable.CGRom1  = &MEM66::ICGROM1;
	MemTable.CGRom2  = &MEM66::ICGROM2;
	MemTable.Kanji   = &MEM66::IKANJI;
	MemTable.Voice   = &MEM66::IVOICE;
	MemTable.IntRam  = &MEM66::IINTRAM;
}

MEM64::MEM64( VM6* vm, const ID& id ) : MEM62( vm, id )
{
	RD_BlkSR.fill( &IRom[EMPTYROM] );
	WR_BlkSR.fill( &IRom[EMPTYROM] );
	RfSR.fill( 0 );
	
	MemTable.System1 = &MEM64::ISYSROM1;
	MemTable.System2 = &MEM64::ISYSROM2;
	MemTable.CGRom1  = &MEM64::ICGROM1;
	MemTable.CGRom2  = nullptr;
	MemTable.Kanji   = nullptr;
	MemTable.Voice   = nullptr;
	MemTable.IntRam  = &MEM64::IINTRAM;
	
	// Dvice Description (Out)
	descs.outdef.emplace( out6xH, STATIC_CAST( Device::OutFuncPtr, &MEM64::Out6xH ) );
	
	// Dvice Description (In)
	descs.indef.emplace ( in6xH,  STATIC_CAST( Device::InFuncPtr,  &MEM64::In6xH  ) );
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
// 汎用 メモリブロック名取得
////////////////////////////////////////////////////////////////
const std::string& MEM6::GetNameCommon( WORD addr, bool rw )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	return rw ? WR_Blk[idx]->GetName( addr, rw )
			  : RD_Blk[idx]->GetName( addr, rw );
}

const std::string& MEM62::GetNameCommon( WORD addr, bool rw )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	if( CGBank && cgenable && ((idx & cgaden) == cgaddr) ){
		return IRom[cgrom ? CGROM1 : CGROM2].GetName( addr, rw );
	}
	return rw ? WR_Blk[idx]->GetName( addr, rw )
			  : RD_Blk[idx]->GetName( addr, rw );
}


////////////////////////////////////////////////////////////////
// 汎用 読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::CommonRead( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	return RD_Blk[idx]->Read( addr, wcnt );
}

BYTE MEM62::CommonRead( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// Port92HとF8H 両方とも設定されていたらCG ROM有効
	if( CGBank && cgenable && ((idx & cgaden) == cgaddr) ){
		return IRom[cgrom ? CGROM1 : CGROM2].Read( addr, wcnt );
	}
	return RD_Blk[idx]->Read( addr, wcnt );
}


////////////////////////////////////////////////////////////////
// 汎用 書込み
////////////////////////////////////////////////////////////////
void MEM6::CommonWrite( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	WR_Blk[idx]->Write( addr, data, wcnt );
}


////////////////////////////////////////////////////////////////
// AreaA,B メモリブロック名取得(60)
////////////////////////////////////////////////////////////////
const std::string& MEM60::GetNameAreaA60Read( WORD addr, bool rw )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	return rw ? GetWriteEnableExt( addr ) ? ERam[idx].GetName( addr, rw )
										  : WR_Blk[idx]->GetName( addr, rw )
			  : GetReadEnableExt ( addr ) ? ERam[idx].GetName( addr, rw )
										  : RD_Blk[idx]->GetName( addr, rw );
}


////////////////////////////////////////////////////////////////
// AreaA,B 読込み(60)
////////////////////////////////////////////////////////////////
BYTE MEM60::AreaA60Read( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	return GetReadEnableExt( addr ) ? ERam[idx].Read( addr )
									: RD_Blk[idx]->Read( addr );
}


////////////////////////////////////////////////////////////////
// AreaA,B 書込み(60)
////////////////////////////////////////////////////////////////
void MEM60::AreaA60Write( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	return GetWriteEnableExt( addr ) ? ERam[idx].Write( addr, data )
									 : WR_Blk[idx]->Write( addr, data );
}


////////////////////////////////////////////////////////////////
// AreaC メモリブロック名取得(60)
////////////////////////////////////////////////////////////////
const std::string& MEM60::GetNameAreaC60Read( WORD addr, bool rw )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// 結果的に汎用と同じ　後で統合する
	return rw ? WR_Blk[idx]->GetName( addr, rw )
			  : RD_Blk[idx]->GetName( addr, rw );
}


////////////////////////////////////////////////////////////////
// AreaC 読込み(60)
////////////////////////////////////////////////////////////////
BYTE MEM60::AreaC60Read( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	return RD_Blk[idx]->Read( addr );
}


////////////////////////////////////////////////////////////////
// AreaC 書込み(60)
////////////////////////////////////////////////////////////////
void MEM60::AreaC60Write( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	// 結果的にAreaAと同じ　後で統合する
	return GetWriteEnableExt( addr ) ? ERam[idx].Write( addr, data )
									 : WR_Blk[idx]->Write( addr, data );
}


////////////////////////////////////////////////////////////////
// AreaD メモリブロック名取得(60)
////////////////////////////////////////////////////////////////
const std::string& MEM60::GetNameAreaD60Read( WORD addr, bool rw )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	if( CGBank && (!(addr & 0x1000) || (RD_Blk[idx] == &IRom[EMPTYROM])) ){
		return IRom[CGROM1].GetName( addr, rw );
	}
	return rw ? WR_Blk[idx]->GetName( addr, rw )
			  : RD_Blk[idx]->GetName( addr, rw );
}


////////////////////////////////////////////////////////////////
// AreaD 読込み(60)
////////////////////////////////////////////////////////////////
BYTE MEM60::AreaD60Read( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	if( CGBank && (!(addr & 0x1000) || (RD_Blk[idx] == &IRom[EMPTYROM])) ){
		// 前半4KBはそのままCGROMデータを返す
		// 後半4KBは外部ROMがなければCGROMのイメージを返す ホント？
		// 外部ROMがあったら外部ROMを読む ホント？
		return IRom[CGROM1].Read( addr & 0x0fff );
	}
	return RD_Blk[idx]->Read( addr );
}


////////////////////////////////////////////////////////////////
// AreaD 書込み(60)
////////////////////////////////////////////////////////////////
void MEM60::AreaD60Write( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	// ウェイトは常にSystem1相当
	if( wcnt ){ *wcnt += MemTable.System1->Wait; }
	
	// 結果的にAreaAと同じ　後で統合する
	return GetWriteEnableExt( addr ) ? ERam[idx].Write( addr, data )
									 : WR_Blk[idx]->Write( addr, data );
}




////////////////////////////////////////////////////////////////
// 内部/外部RAM書込み
////////////////////////////////////////////////////////////////
void MEM62::IERamWrite( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	IRam[addr>>MemBlock::PAGEBITS].Write( addr, data, wcnt );
	ERam[addr>>MemBlock::PAGEBITS].Write( addr, data );		// ウェイト2重カウントしない
}


////////////////////////////////////////////////////////////////
// 漢字ROM メモリブロック名取得
////////////////////////////////////////////////////////////////
const std::string& MEM62::KanjiGetName( WORD addr, bool rw )
{
	int idx = (addr>>MemBlock::PAGEBITS) & 1;
	
	MemBlock& mb = kj_rom ? IRom[KANJIROM+(kj_LR ? 2 : 0) + idx] : IRom[VOICEROM+0+idx];
	return mb.GetName( addr );
}

const std::string& MEM64::KanjiGetName( WORD addr, bool rw )
{
	int idx =  (addr >> MemBlock::PAGEBITS) & 1;
	int mbi = ((addr >> MemBlock::PAGEBITS) & 6) ? VOICEROM+0 : SRMENROM+0;
	
	MemBlock& mb = kj_rom ? IRom[KANJIROM+(kj_LR ? 2 : 0) + idx] : IRom[mbi+idx];
	return mb.GetName( addr );
}


////////////////////////////////////////////////////////////////
// 漢字ROM読込み
////////////////////////////////////////////////////////////////
BYTE MEM62::ReadKanji( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx = (addr>>MemBlock::PAGEBITS) & 1;
	
	MemBlock& mb = kj_rom ? IRom[KANJIROM+(kj_LR ? 2 : 0) + idx] : IRom[VOICEROM+0+idx];
	return mb.Read( addr, wcnt );
}

BYTE MEM64::ReadKanji( MemCell* ptr, WORD addr, int* wcnt )
{
	int idx =  (addr >> MemBlock::PAGEBITS) & 1;
	int mbi = ((addr >> MemBlock::PAGEBITS) & 6) ? VOICEROM+0 : SRMENROM+0;
	
	MemBlock& mb = kj_rom ? IRom[KANJIROM+(kj_LR ? 2 : 0) + idx] : IRom[mbi+idx];
	return mb.Read( addr, wcnt );
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジ メモリブロック名取得
////////////////////////////////////////////////////////////////
const std::string& MEM6::GetNameSolReadEx( WORD addr, bool rw )
{
	return ERam[addr>>MemBlock::PAGEBITS].GetName( addr );
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジ読込み(外部ROM領域)
//   mkII以降の場合,外部ROMは4000-7FFFH以外に割当てることができるので
//   どこに割当てられてもアクセスできるように一段階かませる
////////////////////////////////////////////////////////////////
BYTE MEM6::SolReadEx( MemCell* ptr, WORD addr, int* wcnt )
{
	return ERam[addr>>MemBlock::PAGEBITS].Read( addr, wcnt );
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC読込み
////////////////////////////////////////////////////////////////
BYTE MEM6::SolSccRead( MemCell* ptr, WORD addr, int* wcnt )
{
	// 後で書く
	return 0xff;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジSCC書込み
////////////////////////////////////////////////////////////////
void MEM6::SolSccWrite( MemCell* ptr, WORD addr, BYTE data, int* wcnt )
{
	// 後で書く
}

////////////////////////////////////////////////////////////////
// メモリブロック用関数 ここまで
////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////
// Read Enable取得 (ROM KILL)
////////////////////////////////////////////////////////////////
bool MEM6::GetReadEnableExt( WORD addr )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	switch( ExCart ){
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		return Sol60Mode && (CS01R && (idx == 0 || idx == 1));
	}
	return false;
}


////////////////////////////////////////////////////////////////
// Write Enable取得
////////////////////////////////////////////////////////////////
bool MEM6::GetWriteEnableExt( WORD addr )
{
	int idx = addr>>MemBlock::PAGEBITS;
	
	switch( ExCart ){
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		return Sol60Mode && ((CS01W && (idx == 0 || idx == 1)) || (CS02W && (idx == 2 || idx == 3)));
	}
	return true;
}


////////////////////////////////////////////////////////////////
// 拡張ROM マウント
//
// 引数:	filepath	ファイルパス名への参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::MountExtRom( const P6VPATH& filepath )
{
	PRINTD( MEM_LOG, "[MEM][MountExtRom] -> %s -> ", P6VPATH2STR( filepath ).c_str() );
	
	// ファイル名が空またはROM固定カートリッジならエラー無しで戻る
	if( P6VPATH2STR( filepath ).empty() || (ExCart & EXCFIX) ){ return true; }
	
	// マウント済みなら一旦開放
	UnmountExtRom();
	
	try{
		if( !ExtRom.SetData( filepath ) ){ return false; }
		
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
	
	PRINTD( MEM_LOG, "OK %s\n", P6VPATH2STR( FilePath ).c_str() );
	
	return true;
}


////////////////////////////////////////////////////////////////
// 拡張ROM アンマウント
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void MEM6::UnmountExtRom( void )
{
	PRINTD( MEM_LOG, "[MEM][UnmountExtRom]\n" );
	
	ExtRom.Resize( MemTable.ExtRom->Size, MemTable.ExtRom->Init );
	FilePath.clear();
}


////////////////////////////////////////////////////////////////
// 拡張ROMファイルパス取得
//
// 引数:	なし
// 返値:	P6VPATH&	ファイルパス名への参照
////////////////////////////////////////////////////////////////
const P6VPATH& MEM6::GetFile( void ) const
{
	return FilePath;
}


////////////////////////////////////////////////////////////////
// 拡張カートリッジの種類取得
//
// 引数:	なし
// 返値:	WORD		カートリッジの種類
////////////////////////////////////////////////////////////////
WORD MEM6::GetCartridge( void ) const
{
	return ExCart;
}


////////////////////////////////////////////////////////////////
// 拡張カートリッジマウント
//
// 引数:	cart		カートリッジの種類
//			filepath	ROMフォルダパス名への参照
//			crc			CRCチェック true:する false:しない
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::MountExtCart( WORD cart, const P6VPATH& path, bool crc  )
{
	PRINTD( MEM_LOG, "[MEM][MountExtCart]\n" );
	
	ExCart = cart;			// 拡張カートリッジ
	
	// 外部メモリ確保とROMファイル読込み
	if( !AllocMemoryExt( path, crc ) ){ return false; }
	
	// 外部メモリ初期化
	if( !InitExt() ){ return false; }
	
	// 拡張カートリッジ デバイスディスクリプタ追加
	AddDeviceDescriptorExt();
	
	// リセット
	Reset();
	
	return true;
}


////////////////////////////////////////////////////////////////
// メモリ確保とROMファイル読込み
//
// 引数:	buf			対象メモリセルへの参照
//			info		メモリ情報ポインタ
//			path		ROMフォルダパス名への参照
//			crc			CRCチェック true:する false:しない
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::AllocMemory( MemCells& buf, const MEMINFO* info, const P6VPATH& path, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemory] " );
	
	int i = 0;
	bool ErrSize = false;
	bool ErrCrc  = false;
	
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
				// crc=false または CRC=0の時はチェックしない
				if( crc && (info->Rinfo[i].Crc != 0) &&
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
// 内部メモリ確保とROMファイル読込み
//
// 引数:	path		ROMフォルダパス名への参照
//			crc			CRCチェック true:する false:しない
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::AllocMemoryInt( const P6VPATH& path, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemoryInt]\n" );
	
	// 共通
	if( !AllocMemory( SysRom1, MemTable.System1, path, crc ) ) return false;
	if( !AllocMemory( CGRom1,  MemTable.CGRom1,  path, crc ) ) return false;
	if( !AllocMemory( IntRam,  MemTable.IntRam,  "",   crc ) ) return false;
	
	 // 内部メモリ確保とROMファイル読込み(機種別)
	if( !AllocMemorySpec( path, crc ) ) return false;
	
	// 内部RAMの初期値を設定
	SetRamValue();
	
	return true;
}


////////////////////////////////////////////////////////////////
// 外部メモリ確保とROMファイル読込み
//
// 引数:	path		ROMフォルダパス名への参照
//			crc			CRCチェック true:する false:しない
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::AllocMemoryExt( const P6VPATH& path, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemoryExt]\n" );
	
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
		// ROM:8KB RAM:-
		MemTable.ExtRom = &MEM6::IEXTROM8;
		MemTable.ExtRam = &MEM6::IEMPTRAM;
		break;
		
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
		
	case EXC6053:	// ボイスシンセサイザー
		// ROM:16KB RAM:-
		MemTable.ExtRom = &MEM6::IEXTROM16;
		MemTable.ExtRam = &MEM6::IEMPTRAM;
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
	
	
	// ROM確保&ファイル読込み
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
		if( !AllocMemory( ExtRom, &IEXBASIC, path, crc ) ) return false;
		break;
		
	case EXC660101:	// 拡張漢字ROMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		if( !AllocMemory( ExtRom, &IEXKANJI, path, crc ) ) return false;
		break;
		
	case EXC6053:	// ボイスシンセサイザー
		if( !AllocMemory( ExtRom, &IEXVOICE, path, crc ) ) return false;
		break;
		
	default:		// その他
		if( !AllocMemory( ExtRom, MemTable.ExtRom, "", crc ) ) return false;
	}
	
	// RAM確保
	if( !AllocMemory( ExtRam, MemTable.ExtRam, "", crc ) ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// 内部メモリ確保とROMファイル読込み(機種別)
//
// 引数:	path		ROMフォルダパス名への参照
//			crc			CRCチェック true:する false:しない
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM60::AllocMemorySpec( const P6VPATH& path, bool crc )
{
	return true;
}

bool MEM62::AllocMemorySpec( const P6VPATH& path, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemorySpec]\n" );
	
	if( !AllocMemory( CGRom2,   MemTable.CGRom2, path, crc ) ) return false;
	if( !AllocMemory( KanjiRom, MemTable.Kanji,  path, crc ) ) return false;
	if( !AllocMemory( VoiceRom, MemTable.Voice,  path, crc ) ) return false;
	
	return true;
}

bool MEM64::AllocMemorySpec( const P6VPATH& path, bool crc )
{
	PRINTD( MEM_LOG, "[MEM][AllocMemorySpec]\n" );
	
	if( !AllocMemory( SysRom2,  MemTable.System2, path, crc ) ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// 内部RAMの初期値を設定
//
// 引数:	なし
// 返値:	なし
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
// 内部メモリ初期化
//
// 引数:	なし
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::InitInt( void )
{
	PRINTD( MEM_LOG, "[MEM][InitInt]\n" );
	
	// とりあえず全メモリブロックをEmptyに設定(ROMはウェイトあり)
	for( auto &mb : IRom ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 1 ); }
	for( auto &mb : IRam ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 0 ); }
	
	// 汎用アクセス用
	IRom[RWCOMMON].SetFunc( FN( STATIC_CAST( NFuncPtr, &MEM6::GetNameCommon ) ), FR( STATIC_CAST( RFuncPtr, &MEM6::CommonRead ) ), FW( STATIC_CAST( WFuncPtr, &MEM6::CommonWrite ) ), 0 );
	
	// CGROMアクセス用に一段噛ませる
	RBLK[0] = WBLK[0] = &IRom[RWCOMMON];
	RBLK[1] = WBLK[1] = &IRom[RWCOMMON];
	RBLK[2] = WBLK[2] = &IRom[RWCOMMON];
	RBLK[3] = WBLK[3] = &IRom[RWCOMMON];
	RBLK[4] = WBLK[4] = &IRom[RWCOMMON];
	RBLK[5] = WBLK[5] = &IRom[RWCOMMON];
	RBLK[6] = WBLK[6] = &IRom[RWCOMMON];
	RBLK[7] = WBLK[7] = &IRom[RWCOMMON];
	
	
	// 内部メモリ初期化(機種別)
	if( !InitIntSpec() ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// 外部メモリ初期化
//
// 引数:	なし
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM6::InitExt( void )
{
	PRINTD( MEM_LOG, "[MEM][InitExt]\n" );
	
	// とりあえず全メモリブロックをEmptyに設定(ROMはウェイトあり)
	for( auto &mb : ERom ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 1 ); }
	for( auto &mb : ERam ){ mb.SetFunc( "EMPTY", nullptr, nullptr, 0 ); }
	
	// ROM
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
		ERom[EXTROM+0].SetMemory( "EROM00", ExtRom( 0 ), MemTable.ExtRom->Wait );
		break;
		
	case EXC6005:	// ROMカートリッジ
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		ERom[EXTROM+0].SetMemory( "EROM00", ExtRom( 0 ), MemTable.ExtRom->Wait );
		ERom[EXTROM+1].SetMemory( "EROM01", ExtRom( 1 ), MemTable.ExtRom->Wait );
		break;
		
	case EXC6053:	// ボイスシンセサイザー
		ERom[EXTROM+1].SetMemory( "EROM01", ExtRom( 0 ), MemTable.ExtRom->Wait );
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
		ERom[EXTROM+0].SetFunc  ( FN( STATIC_CAST( NFuncPtr, &MEM6::GetNameSolReadEx ) ), FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		if( IntRam.Size() < 3 ){	// 60
			ERom[EXTROM+1].SetMemory( "ERAM03", ExtRam( 3 ), MemTable.ExtRom->Wait );
		}else{						// 62
			// ※mk2以降用の設定はこれ↓で動くけど本当は正しくない
			// 設定は「内部RAMに書込み」で，ハード的には外部RAMにも同時に書き込まれる
			ERom[EXTROM+1].SetMemory( "IERAM3", IntRam( 3 ), MemTable.ExtRom->Wait );
		}
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		ERom[EXTROM+0].SetFunc  ( FN( STATIC_CAST( NFuncPtr, &MEM6::GetNameSolReadEx ) ), FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		ERom[EXTROM+1].SetFunc  ( FN( STATIC_CAST( NFuncPtr, &MEM6::GetNameSolReadEx ) ), FR( STATIC_CAST( RFuncPtr, &MEM6::SolReadEx ) ), nullptr, MemTable.ExtRom->Wait );
		break;
	}
	
	// RAM
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
	case EXCSOL1:	// 戦士のカートリッジ
		{	int i = 0;
			for( auto &mb : ERam ){
				mb.SetMemory( Stringf( "ERAM%02d", i & (ExtRam.Size()-1) ), ExtRam( i & (ExtRam.Size()-1) ), MemTable.ExtRam->Wait );
				i++;
			}
		}
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		SolBank.fill( NONBANK );
		break;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// 内部メモリ初期化(機種別)
//
// 引数:	なし
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool MEM60::InitIntSpec( void )
{
	PRINTD( MEM_LOG, "[MEM][InitIntSpec]\n" );
	
	// 60専用アクセス用
	IRom[RWAREAA60].SetFunc( FN( STATIC_CAST( NFuncPtr, &MEM60::GetNameAreaA60Read ) ), FR( STATIC_CAST( RFuncPtr, &MEM60::AreaA60Read ) ), FW( STATIC_CAST( WFuncPtr, &MEM60::AreaA60Write ) ), MemTable.System1->Wait );
	IRom[RWAREAC60].SetFunc( FN( STATIC_CAST( NFuncPtr, &MEM60::GetNameAreaC60Read ) ), FR( STATIC_CAST( RFuncPtr, &MEM60::AreaC60Read ) ), FW( STATIC_CAST( WFuncPtr, &MEM60::AreaC60Write ) ), MemTable.System1->Wait );
	IRom[RWAREAD60].SetFunc( FN( STATIC_CAST( NFuncPtr, &MEM60::GetNameAreaD60Read ) ), FR( STATIC_CAST( RFuncPtr, &MEM60::AreaD60Read ) ), FW( STATIC_CAST( WFuncPtr, &MEM60::AreaD60Write ) ), MemTable.CGRom1->Wait );
	
	// CGROMアクセス用に一段噛ませる
	RBLK[0] = WBLK[0] = &IRom[RWAREAA60];
	RBLK[1] = WBLK[1] = &IRom[RWAREAA60];
	RBLK[2] = WBLK[2] = &IRom[RWAREAC60];
	RBLK[3] = WBLK[3] = &IRom[RWAREAD60];
	
	
	// BASIC ROM
	IRom[MAINROM+0].SetMemory( "BASIC0", SysRom1( 0 ), MemTable.System1->Wait );
	IRom[MAINROM+1].SetMemory( "BASIC1", SysRom1( 1 ), MemTable.System1->Wait );
	
	// CG ROM
	IRom[CGROM1].SetMemory   ( "CGROM1", CGRom1( 0 ),  MemTable.CGRom1->Wait );
	
	// 内部RAM
	IRam[INTRAM+0].SetMemory ( "IRAM00", IntRam( 0 ),  MemTable.IntRam->Wait );
	IRam[INTRAM+1].SetMemory ( "IRAM01", IntRam( 1 ),  MemTable.IntRam->Wait );
	
	return true;
}

bool MEM62::InitIntSpec( void )
{
	PRINTD( MEM_LOG, "[MEM][InitIntSpec]\n" );
	
	// BASIC ROM
	IRom[MAINROM+0].SetMemory ( "BASIC0", SysRom1( 0 ),  MemTable.System1->Wait );
	IRom[MAINROM+1].SetMemory ( "BASIC1", SysRom1( 1 ),  MemTable.System1->Wait );
	IRom[MAINROM+2].SetMemory ( "BASIC2", SysRom1( 2 ),  MemTable.System1->Wait );
	IRom[MAINROM+3].SetMemory ( "BASIC3", SysRom1( 3 ),  MemTable.System1->Wait );
	
	// CG ROM
	IRom[CGROM1].SetMemory    ( "CGROM1", CGRom1( 0 ),   MemTable.CGRom1->Wait );
	IRom[CGROM2].SetMemory    ( "CGROM2", CGRom2( 0 ),   MemTable.CGRom2->Wait );
	
	// 漢字ROM
	IRom[KANJIROM+0].SetMemory( "KJROM0", KanjiRom( 0 ), MemTable.Kanji->Wait );
	IRom[KANJIROM+1].SetMemory( "KJROM1", KanjiRom( 1 ), MemTable.Kanji->Wait );
	IRom[KANJIROM+2].SetMemory( "KJROM2", KanjiRom( 2 ), MemTable.Kanji->Wait );
	IRom[KANJIROM+3].SetMemory( "KJROM3", KanjiRom( 3 ), MemTable.Kanji->Wait );
	IRom[KNJROM].SetFunc      ( FN( STATIC_CAST( NFuncPtr, &MEM62::KanjiGetName ) ), FR( STATIC_CAST( RFuncPtr, &MEM62::ReadKanji ) ), nullptr, MemTable.Kanji->Wait );
	
	// 音声合成ROM
	IRom[VOICEROM+0].SetMemory( "VOROM0", VoiceRom( 0 ), MemTable.Voice->Wait );
	IRom[VOICEROM+1].SetMemory( "VOROM1", VoiceRom( 1 ), MemTable.Voice->Wait );
	
	// 内部RAM
	int i = 0;
	for( auto &mb : IRam ){
		mb.SetMemory( Stringf( "IRAM%02d", i ), IntRam( i ), MemTable.IntRam->Wait );
		i++;
	}
	
	// 内部/外部RAM書込み
	IRam[INEXRAM].SetFunc     ( "IERAM", nullptr, FW( STATIC_CAST( WFuncPtr, &MEM62::IERamWrite ) ), MemTable.IntRam->Wait );
	
	return true;
}

bool MEM64::InitIntSpec( void )
{
	PRINTD( MEM_LOG, "[MEM][InitIntSpec]\n" );
	
	// N66-BASIC ROM
	IRom[MAINROM+0].SetMemory ( "SYS1-0", SysRom1( 0 ), MemTable.System1->Wait );
	IRom[MAINROM+1].SetMemory ( "SYS1-1", SysRom1( 1 ), MemTable.System1->Wait );
	IRom[MAINROM+2].SetMemory ( "SYS1-2", SysRom1( 2 ), MemTable.System1->Wait );
	IRom[MAINROM+3].SetMemory ( "SYS1-3", SysRom1( 3 ), MemTable.System1->Wait );
	
	// N66SR-BASIC ROM
	IRom[MAINROM+4].SetMemory ( "SYS1-4", SysRom1( 4 ), MemTable.System1->Wait );
	IRom[MAINROM+5].SetMemory ( "SYS1-5", SysRom1( 5 ), MemTable.System1->Wait );
	IRom[MAINROM+6].SetMemory ( "SYS1-6", SysRom1( 6 ), MemTable.System1->Wait );
	IRom[MAINROM+7].SetMemory ( "SYS1-7", SysRom1( 7 ), MemTable.System1->Wait );
	
	// SR メニューROM
	IRom[SRMENROM+0].SetMemory( "SYS2-0", SysRom2( 0 ), MemTable.System1->Wait );
	IRom[SRMENROM+1].SetMemory( "SYS2-1", SysRom2( 1 ), MemTable.System1->Wait );
	
	// 音声合成ROM
	IRom[VOICEROM+0].SetMemory( "SYS2-2", SysRom2( 2 ), MemTable.System1->Wait );
	IRom[VOICEROM+1].SetMemory( "SYS2-3", SysRom2( 3 ), MemTable.System1->Wait );
	
	// 漢字ROM
	IRom[KANJIROM+0].SetMemory( "SYS2-4", SysRom2( 4 ), MemTable.System1->Wait );
	IRom[KANJIROM+1].SetMemory( "SYS2-5", SysRom2( 5 ), MemTable.System1->Wait );
	IRom[KANJIROM+2].SetMemory( "SYS2-6", SysRom2( 6 ), MemTable.System1->Wait );
	IRom[KANJIROM+3].SetMemory( "SYS2-7", SysRom2( 7 ), MemTable.System1->Wait );
	IRom[KNJROM].SetFunc      ( FN( STATIC_CAST( NFuncPtr, &MEM64::KanjiGetName ) ), FR( STATIC_CAST( RFuncPtr, &MEM64::ReadKanji ) ), nullptr, MemTable.System1->Wait );
	
	// CG ROM
	IRom[CGROM1].SetMemory    ( "CGROM1", CGRom1( 0 ),  MemTable.CGRom1->Wait );
	IRom[CGROM2].SetMemory    ( "CGROM2", CGRom1( 1 ),  MemTable.CGRom1->Wait );
	
	// 内部RAM
	int i = 0;
	for( auto &mb : IRam ){
		mb.SetMemory( Stringf( "IRAM%02d", i ), IntRam( i ), MemTable.IntRam->Wait );
		i++;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// リセット
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void MEM6::Reset()
{
	PRINTD( MEM_LOG, "[MEM][Reset]\n" );
	
	CGBank = false;	// CG ROM BANK 無効
	
	// 外部メモリリセット
	ResetExt();
	
	// メモリコントローラ内部レジスタ初期値設定
	Rf = { INIT_RF0, INIT_RF1, INIT_RF2 };
	
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
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
// 外部メモリリセット
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void MEM6::ResetExt()
{
	PRINTD( MEM_LOG, "[MEM][ResetExt]\n" );
	
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
		CS02W     = false;
		
		// メモリバンク初期化
		{	int i = 0;
			const std::vector<BYTE> bk2 = { NONBANK,     NONBANK,     ROMBANK | 0, RAMBANK | 3,
											RAMBANK | 4, RAMBANK | 5, NONBANK,     NONBANK      };
			for( auto bk : bk2 ){
				SetSolBank( i, bk );
				i++;
			}
		}
		break;
	}
}


////////////////////////////////////////////////////////////////
// フェッチ(M1)
////////////////////////////////////////////////////////////////
BYTE MEM6::Fetch( WORD addr, int* m1wait ) const
{
	BYTE data = RBLK[addr>>MemBlock::PAGEBITS]->Read( addr );
	
	PRINTD( MEM_LOG, "[MEM][Fetch] -> %04X:%02X\n", addr, data );
	
	// M1ウェイト追加
	if( m1wait ){ (*m1wait) += M1Wait; }
	
	// バスリクエスト区間実行時ウェイト追加
	if( vm->VdgIsBusReqExec() ){ (*m1wait)++; }
	
	return data;
}

BYTE MEM64::Fetch( WORD addr, int* m1wait ) const
{
	BYTE data = vm->VdgIsSRmode() ? RD_BlkSR[addr>>MemBlock::PAGEBITS]->Read( addr )
								  : RBLK    [addr>>MemBlock::PAGEBITS]->Read( addr );
	
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
	BYTE data = RBLK[addr>>MemBlock::PAGEBITS]->Read( addr, wcnt );
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){ (*wcnt)++; }
	
	PRINTD( MEM_LOG, "[MEM][Read]  -> %04X:%02X\n", addr, data );
	
	return data;
}

BYTE MEM64::Read( WORD addr, int* wcnt ) const
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
		data = RBLK[addr>>MemBlock::PAGEBITS]->Read( addr, wcnt );
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
	PRINTD( MEM_LOG, "[MEM][Write] %04X:%02X -> %s[%04X]'%c'\n", addr, data, WBLK[addr>>MemBlock::PAGEBITS]->GetName( addr, true ).c_str(), addr&0x1fff, data );
	
	WBLK[addr>>MemBlock::PAGEBITS]->Write( addr, data, wcnt );
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){ (*wcnt)++; }
	
	// 内部/外部RAMとも書込みの場合はひとまず内部だけ
}

void MEM64::Write( WORD addr, BYTE data, int* wcnt )
{
	PRINTD( MEM_LOG, "[MEM][Write] %04X:%02X -> %s[%04X]'%c'\n", addr, data, vm->VdgIsSRmode() ? WR_BlkSR[addr>>MemBlock::PAGEBITS]->GetName( addr, true ).c_str() : WR_Blk[addr>>MemBlock::PAGEBITS]->GetName( addr, true ).c_str(), addr&0x1fff, data );
	
	if( vm->VdgIsSRmode() ){
		if( vm->VdgIsSRBitmap( addr ) && (RfSR[(addr>>MemBlock::PAGEBITS)+8] == 0) ){	// ビットマップモード(内部RAMアクセス)
			WORD ad = vm->VdgSRGVramAddr( addr );
			IntRam.Write( ad, (addr & 1) ? ((IntRam.Read( ad ) & 0x0f) | ((data << 4) & 0xf0)) : ((IntRam.Read( ad ) & 0xf0) | ( data & 0x0f)) );
		}else{															// 直接アクセスモード
			WR_BlkSR[addr>>MemBlock::PAGEBITS]->Write( addr, data, wcnt );
		}
	}else{
		WBLK[addr>>MemBlock::PAGEBITS]->Write( addr, data, wcnt );
	}
	
	// バスリクエスト区間実行時ウェイト追加
	if( wcnt && vm->VdgIsBusReqExec() ){ (*wcnt)++; }
	
	// 内部/外部RAMとも書込みの場合はひとまず内部だけ
}


////////////////////////////////////////////////////////////////
// メモリアクセスウェイト設定
////////////////////////////////////////////////////////////////
void MEM62::SetWait( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetWait] -> M1:%d ROM:%d RAM:%d\n", (data>>7)&1, (data>>6)&1, (data>>5)&1 );
	
	// M1
	M1Wait = data & 0x80 ? 1 : 0;
	
	// ROM
	std::vector<MemBlock> mbrom = {
		IRom[EMPTYROM],   ERom[EXTROM+0],   ERom[EXTROM+1],   IRom[MAINROM+0],  IRom[MAINROM+1],  IRom[MAINROM+2], IRom[MAINROM+3],
		IRom[KANJIROM+0], IRom[KANJIROM+1], IRom[KANJIROM+2], IRom[KANJIROM+3], IRom[VOICEROM+0], IRom[VOICEROM+1],
		IRom[MAINROM+4],  IRom[MAINROM+5],  IRom[MAINROM+6],  IRom[MAINROM+7],	IRom[SRMENROM+0], IRom[SRMENROM+1]	// SR
	};
	for( auto &mb : mbrom ){
		mb.SetWait ( data & 0x40 ? 1 : 0 );
	}
	
	// RAM
	std::vector<MemBlock> mbram = {
		IRom[EMPTYRAM],
		IRam[INTRAM+0], IRam[INTRAM+1], IRam[INTRAM+2], IRam[INTRAM+3], IRam[INTRAM+4], IRam[INTRAM+5], IRam[INTRAM+6], IRam[INTRAM+7],
		ERam[EXTRAM+0], ERam[EXTRAM+1], ERam[EXTRAM+2], ERam[EXTRAM+3], ERam[EXTRAM+4], ERam[EXTRAM+5], ERam[EXTRAM+6], ERam[EXTRAM+7]
	};
	for( auto &mb : mbram ){
		mb.SetWait ( data & 0x20 ? 1 : 0 );
	}
}


////////////////////////////////////////////////////////////////
// メモリアクセスウェイト取得
////////////////////////////////////////////////////////////////
BYTE MEM62::GetWait( void ) const
{
	return ( M1Wait ? 0x80 : 0 ) | ( IRom[EMPTYROM].GetWait() ? 0x40 : 0 ) | ( IRom[EMPTYRAM].GetWait() ? 0x20 : 0 );
}


////////////////////////////////////////////////////////////////
// メモリリード時のメモリブロック指定
////////////////////////////////////////////////////////////////
void MEM60::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR]\n" );
	
	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &IRom[MAINROM+1];
	RD_Blk[2] = &IRom[EMPTYROM];	RD_Blk[3] = &IRom[EMPTYROM];
	RD_Blk[4] = &IRom[EMPTYROM];	RD_Blk[5] = &IRom[EMPTYROM];
	RD_Blk[6] = &IRam[INTRAM+0];	RD_Blk[7] = &IRam[INTRAM+1];
	
	
	// 拡張カートリッジ ========================================
	switch( ExCart ){
	case EXC6001:	// 拡張BASIC
		RD_Blk[2] = &ERom[EXTROM+0];
		break;
		
	case EXC6005:	// ROMカートリッジ
		RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+1];
		break;
		
	case EXC6053:	// ボイスシンセサイザー
										RD_Blk[3] = &ERom[EXTROM+1];
		break;
		
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+1];
		RD_Blk[4] = &ERam[EXTRAM+0];	RD_Blk[5] = &ERam[EXTRAM+1];
		break;
		
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		RD_Blk[4] = &ERam[EXTRAM+4];	RD_Blk[5] = &ERam[EXTRAM+5];
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
		RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+1];
		RD_Blk[4] = &ERam[EXTRAM+0];	RD_Blk[5] = &ERam[EXTRAM+1];
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		RD_Blk[2] = &ERam[EXTRAM+2];	RD_Blk[3] = &ERam[EXTRAM+3];
		RD_Blk[4] = &ERam[EXTRAM+4];	RD_Blk[5] = &ERam[EXTRAM+5];
		break;
	}
	// =========================================================
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName( i<<MemBlock::PAGEBITS ).c_str(), i+1, RD_Blk[i+1]->GetName( (i+1)<<MemBlock::PAGEBITS ).c_str() );
	}
	#endif
}

void MEM62::SetMemBlockR( BYTE mem1, BYTE mem2 )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockR] -> %02X %02X\n", mem1, mem2 );
	
	// Port F0H
	switch( mem1 & 0x0f ){	// RF0下位 (0000 - 3FFF)
		case 0x00:	RD_Blk[0] = &IRom[EMPTYROM];	RD_Blk[1] = &IRom[EMPTYROM];	break;
		case 0x01:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &IRom[MAINROM+1];	break;
		case 0x02:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &IRom[KNJROM];		break;
		case 0x03:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x04:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &ERom[EXTROM+0];	break;
		case 0x05:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &IRom[MAINROM+1];	break;
		case 0x06:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &IRom[KNJROM];		break;
		case 0x07:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x08:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &ERom[EXTROM+0];	break;
		case 0x09:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &IRom[MAINROM+1];	break;
		case 0x0a:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x0b:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &IRom[KNJROM];		break;
		case 0x0c:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &ERom[EXTROM+0];	break;
		case 0x0d:	RD_Blk[0] = &IRam[INTRAM+0];	RD_Blk[1] = &IRam[INTRAM+1];	break;
		case 0x0e:	RD_Blk[0] = &ERam[EXTRAM+0];	RD_Blk[1] = &ERam[EXTRAM+1];	break;
		case 0x0f:	RD_Blk[0] = &IRom[EMPTYROM];	RD_Blk[1] = &IRom[EMPTYROM];	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	RD_Blk[2] = &IRom[EMPTYROM];	RD_Blk[3] = &IRom[EMPTYROM];	break;
		case 0x10:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0x20:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &IRom[KNJROM];		break;
		case 0x30:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0x40:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0x50:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0x60:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &IRom[KNJROM];		break;
		case 0x70:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0x80:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0x90:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0xa0:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0xb0:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &IRom[KNJROM];		break;
		case 0xc0:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0xd0:	RD_Blk[2] = &IRam[INTRAM+2];	RD_Blk[3] = &IRam[INTRAM+3];	break;
		case 0xe0:	RD_Blk[2] = &ERam[EXTRAM+2];	RD_Blk[3] = &ERam[EXTRAM+3];	break;
		case 0xf0:	RD_Blk[2] = &IRom[EMPTYROM];	RD_Blk[3] = &IRom[EMPTYROM];	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	RD_Blk[4] = &IRom[EMPTYROM];	RD_Blk[5] = &IRom[EMPTYROM];	break;
		case 0x01:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x02:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x03:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x04:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x05:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x06:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x07:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x08:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x09:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x0a:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x0b:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x0c:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x0d:	RD_Blk[4] = &IRam[INTRAM+4];	RD_Blk[5] = &IRam[INTRAM+5];	break;
		case 0x0e:	RD_Blk[4] = &ERam[EXTRAM+4];	RD_Blk[5] = &ERam[EXTRAM+5];	break;
		case 0x0f:	RD_Blk[4] = &IRom[EMPTYROM];	RD_Blk[5] = &IRom[EMPTYROM];	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	RD_Blk[6] = &IRom[EMPTYROM];	RD_Blk[7] = &IRom[EMPTYROM];	break;
		case 0x10:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0x20:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &IRom[KNJROM];		break;
		case 0x30:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0x40:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0x50:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0x60:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &IRom[KNJROM];		break;
		case 0x70:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0x80:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0x90:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0xa0:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0xb0:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &IRom[KNJROM];		break;
		case 0xc0:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0xd0:	RD_Blk[6] = &IRam[INTRAM+6];	RD_Blk[7] = &IRam[INTRAM+7];	break;
		case 0xe0:	RD_Blk[6] = &ERam[EXTRAM+6];	RD_Blk[7] = &ERam[EXTRAM+7];	break;
		case 0xf0:	RD_Blk[6] = &IRom[EMPTYROM];	RD_Blk[7] = &IRom[EMPTYROM];	break;
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName( i<<MemBlock::PAGEBITS ).c_str(), i+1, RD_Blk[i+1]->GetName( (i+1)<<MemBlock::PAGEBITS ).c_str() );
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
		case 0x00:	RD_Blk[0] = &IRom[EMPTYROM];	RD_Blk[1] = &IRom[EMPTYROM];	break;
		case 0x01:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &IRom[MAINROM+1];	break;
		case 0x02:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &IRom[KNJROM];		break;	// ココ
		case 0x03:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x04:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &ERom[EXTROM+0];	break;
		case 0x05:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &IRom[MAINROM+1];	break;	// ココ
		case 0x06:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &IRom[KNJROM];		break;	// ココ
		case 0x07:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x08:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &ERom[EXTROM+0];	break;
		case 0x09:	RD_Blk[0] = &ERom[EXTROM+1];	RD_Blk[1] = &IRom[MAINROM+1];	break;
		case 0x0a:	RD_Blk[0] = &IRom[MAINROM+0];	RD_Blk[1] = &ERom[EXTROM+1];	break;
		case 0x0b:	RD_Blk[0] = &ERom[EXTROM+0];	RD_Blk[1] = &IRom[KNJROM];		break;	// ココ
		case 0x0c:	RD_Blk[0] = &IRom[KNJROM];		RD_Blk[1] = &ERom[EXTROM+0];	break;	// ココ
		case 0x0d:	RD_Blk[0] = &IRam[INTRAM+0];	RD_Blk[1] = &IRam[INTRAM+1];	break;
		case 0x0e:	RD_Blk[0] = &ERam[EXTRAM+0];	RD_Blk[1] = &ERam[EXTRAM+1];	break;
		case 0x0f:	RD_Blk[0] = &IRom[EMPTYROM];	RD_Blk[1] = &IRom[EMPTYROM];	break;
	}
	switch( mem1 & 0xf0 ){	// RF0上位 (4000 - 7FFF)
		case 0x00:	RD_Blk[2] = &IRom[EMPTYROM];	RD_Blk[3] = &IRom[EMPTYROM];	break;
		case 0x10:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0x20:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &IRom[KNJROM];		break;
		case 0x30:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0x40:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0x50:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0x60:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &IRom[KNJROM];		break;
		case 0x70:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0x80:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0x90:	RD_Blk[2] = &ERom[EXTROM+1];	RD_Blk[3] = &IRom[MAINROM+3];	break;
		case 0xa0:	RD_Blk[2] = &IRom[MAINROM+2];	RD_Blk[3] = &ERom[EXTROM+1];	break;
		case 0xb0:	RD_Blk[2] = &ERom[EXTROM+0];	RD_Blk[3] = &IRom[KNJROM];		break;
		case 0xc0:	RD_Blk[2] = &IRom[KNJROM];		RD_Blk[3] = &ERom[EXTROM+0];	break;
		case 0xd0:	RD_Blk[2] = &IRam[INTRAM+2];	RD_Blk[3] = &IRam[INTRAM+3];	break;
		case 0xe0:	RD_Blk[2] = &ERam[EXTRAM+2];	RD_Blk[3] = &ERam[EXTRAM+3];	break;
		case 0xf0:	RD_Blk[2] = &IRom[EMPTYROM];	RD_Blk[3] = &IRom[EMPTYROM];	break;
	}
	
	// Port F1H
	switch( mem2 & 0x0f ){	// RF1下位 (8000 - BFFF)
		case 0x00:	RD_Blk[4] = &IRom[EMPTYROM];	RD_Blk[5] = &IRom[EMPTYROM];	break;
		case 0x01:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x02:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x03:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x04:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x05:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x06:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x07:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x08:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x09:	RD_Blk[4] = &ERom[EXTROM+1];	RD_Blk[5] = &IRom[MAINROM+1];	break;
		case 0x0a:	RD_Blk[4] = &IRom[MAINROM+0];	RD_Blk[5] = &ERom[EXTROM+1];	break;
		case 0x0b:	RD_Blk[4] = &ERom[EXTROM+0];	RD_Blk[5] = &IRom[KNJROM];		break;
		case 0x0c:	RD_Blk[4] = &IRom[KNJROM];		RD_Blk[5] = &ERom[EXTROM+0];	break;
		case 0x0d:	RD_Blk[4] = &IRam[INTRAM+4];	RD_Blk[5] = &IRam[INTRAM+5];	break;
		case 0x0e:	RD_Blk[4] = &ERam[EXTRAM+4];	RD_Blk[5] = &ERam[EXTRAM+5];	break;
		case 0x0f:	RD_Blk[4] = &IRom[EMPTYROM];	RD_Blk[5] = &IRom[EMPTYROM];	break;
	}
	switch( mem2 & 0xf0 ){	// RF1上位 (C000 - FFFF)
		case 0x00:	RD_Blk[6] = &IRom[EMPTYROM];	RD_Blk[7] = &IRom[EMPTYROM];	break;
		case 0x10:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0x20:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &IRom[KNJROM];		break;
		case 0x30:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0x40:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0x50:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0x60:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &IRom[KNJROM];		break;
		case 0x70:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0x80:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0x90:	RD_Blk[6] = &ERom[EXTROM+1];	RD_Blk[7] = &IRom[MAINROM+3];	break;
		case 0xa0:	RD_Blk[6] = &IRom[MAINROM+2];	RD_Blk[7] = &ERom[EXTROM+1];	break;
		case 0xb0:	RD_Blk[6] = &ERom[EXTROM+0];	RD_Blk[7] = &IRom[KNJROM];		break;
		case 0xc0:	RD_Blk[6] = &IRom[KNJROM];		RD_Blk[7] = &ERom[EXTROM+0];	break;
		case 0xd0:	RD_Blk[6] = &IRam[INTRAM+6];	RD_Blk[7] = &IRam[INTRAM+7];	break;
		case 0xe0:	RD_Blk[6] = &ERam[EXTRAM+6];	RD_Blk[7] = &ERam[EXTRAM+7];	break;
		case 0xf0:	RD_Blk[6] = &IRom[EMPTYROM];	RD_Blk[7] = &IRom[EMPTYROM];	break;
	}
	
	// 内部レジスタ保存
	Rf[0] = mem1;
	Rf[1] = mem2;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, RD_Blk[i]->GetName( i<<MemBlock::PAGEBITS ).c_str(), i+1, RD_Blk[i+1]->GetName( (i+1)<<MemBlock::PAGEBITS ).c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// メモリライト時のメモリブロック指定
////////////////////////////////////////////////////////////////
void MEM60::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW]\n" );
	
	WR_Blk[0] = &IRom[EMPTYROM];	WR_Blk[1] = &IRom[EMPTYROM];
	WR_Blk[2] = &IRom[EMPTYROM];	WR_Blk[3] = &IRom[EMPTYROM];
	WR_Blk[4] = &IRom[EMPTYROM];	WR_Blk[5] = &IRom[EMPTYROM];
	WR_Blk[6] = &IRam[INTRAM+0];	WR_Blk[7] = &IRam[INTRAM+1];
	
	
	// 拡張カートリッジ ========================================
	switch( ExCart ){
	case EXC6006:	// 拡張ROM/RAMカートリッジ
		WR_Blk[4] = &ERam[EXTRAM+0];	WR_Blk[5] = &ERam[EXTRAM+1];
		break;
		
	case EXC6006SR:	// 拡張64KRAMカートリッジ
	case EXC6007SR:	// 拡張漢字ROM&RAMカートリッジ
		WR_Blk[4] = &ERam[EXTRAM+4];	WR_Blk[5] = &ERam[EXTRAM+5];
		break;
		
	case EXCSOL1:	// 戦士のカートリッジ
										WR_Blk[3] = &ERam[EXTRAM+3];
		WR_Blk[4] = &ERam[EXTRAM+0];	WR_Blk[5] = &ERam[EXTRAM+1];
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		WR_Blk[0] = &ERam[EXTRAM+0];	WR_Blk[1] = &ERam[EXTRAM+1];
		WR_Blk[2] = &ERam[EXTRAM+2];	WR_Blk[3] = &ERam[EXTRAM+3];
		WR_Blk[4] = &ERam[EXTRAM+4];	WR_Blk[5] = &ERam[EXTRAM+5];
		break;
	}
	// =========================================================
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, WR_Blk[i]->GetName( i<<MemBlock::PAGEBITS, true ).c_str(), i+1, WR_Blk[i+1]->GetName( (i+1)<<MemBlock::PAGEBITS, true ).c_str() );
	}
	#endif
}

void MEM62::SetMemBlockW( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockW] -> %02X\n", data );
	
	switch( data & 3 ){			// 0000 - 3FFF
		case 0: WR_Blk[0] = &IRom[EMPTYRAM];	WR_Blk[1] = &IRom[EMPTYRAM];	break;
		case 1: WR_Blk[0] = &IRam[INTRAM+0];	WR_Blk[1] = &IRam[INTRAM+1];	break;
		case 2: WR_Blk[0] = &ERam[EXTRAM+0];	WR_Blk[1] = &ERam[EXTRAM+1];	break;
		case 3: WR_Blk[0] = &IRam[INEXRAM];		WR_Blk[1] = &IRam[INEXRAM];		break;
	}
	switch( (data>>2) & 3 ){	// 4000 - 7FFF
		case 0: WR_Blk[2] = &IRom[EMPTYRAM];	WR_Blk[3] = &IRom[EMPTYRAM];	break;
		case 1: WR_Blk[2] = &IRam[INTRAM+2];	WR_Blk[3] = &IRam[INTRAM+3];	break;
		case 2: WR_Blk[2] = &ERam[EXTRAM+2];	WR_Blk[3] = &ERam[EXTRAM+3];	break;
		case 3: WR_Blk[2] = &IRam[INEXRAM];		WR_Blk[3] = &IRam[INEXRAM];		break;
	}
	switch( (data>>4) & 3 ){	// 8000 - BFFF
		case 0: WR_Blk[4] = &IRom[EMPTYRAM];	WR_Blk[5] = &IRom[EMPTYRAM];	break;
		case 1: WR_Blk[4] = &IRam[INTRAM+4];	WR_Blk[5] = &IRam[INTRAM+5];	break;
		case 2: WR_Blk[4] = &ERam[EXTRAM+4];	WR_Blk[5] = &ERam[EXTRAM+5];	break;
		case 3: WR_Blk[4] = &IRam[INEXRAM];		WR_Blk[5] = &IRam[INEXRAM];		break;
	}
	switch( (data>>6) & 3 ){	// C000 - FFFF
		case 0: WR_Blk[6] = &IRom[EMPTYRAM];	WR_Blk[7] = &IRom[EMPTYRAM];	break;
		case 1: WR_Blk[6] = &IRam[INTRAM+6];	WR_Blk[7] = &IRam[INTRAM+7];	break;
		case 2: WR_Blk[6] = &ERam[EXTRAM+6];	WR_Blk[7] = &ERam[EXTRAM+7];	break;
		case 3: WR_Blk[6] = &IRam[INEXRAM];		WR_Blk[7] = &IRam[INEXRAM];		break;
	}
	
	// 内部レジスタ保存
	Rf[2] = data;
	
	#if (MEM_LOG)
	for( int i=0; i<8; i+=2 ){
		PRINTD( MEM_LOG, "\t%d:%8s\t%d:%8s\n", i, WR_Blk[i]->GetName( i<<MemBlock::PAGEBITS, true ).c_str(), i+1, WR_Blk[i+1]->GetName( (i+1)<<MemBlock::PAGEBITS, true ).c_str() );
	}
	#endif
}


////////////////////////////////////////////////////////////////
// メモリリード/ライト時のメモリブロック指定(64,68)
////////////////////////////////////////////////////////////////
void MEM64::SetMemBlockSR( BYTE port, BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetMemBlockSR] -> Port:%02X Data:%02X\n", port, data );
	
	BYTE cs   = data >> 4;
	BYTE addr = (data >> 1) & 0x07;
	MemBlock** mb;
	
	RfSR[port & 0x0f] = data;
	if( port & 0x08 ) mb = &WR_BlkSR[port & 0x07];	// 8-F : Write
	else			  mb = &RD_BlkSR[port & 0x07];	// 0-7 : Read
	
	switch( cs ){
	case 0x00:	// System RAM (16KB毎)
		switch( addr ){
		case 0x00: *mb = port&1 ? &IRam[INTRAM+1] : &IRam[INTRAM+0]; break;
		case 0x01: *mb = port&1 ? &IRam[INTRAM+1] : &IRam[INTRAM+0]; break;
		case 0x02: *mb = port&1 ? &IRam[INTRAM+3] : &IRam[INTRAM+2]; break;
		case 0x03: *mb = port&1 ? &IRam[INTRAM+3] : &IRam[INTRAM+2]; break;
		case 0x04: *mb = port&1 ? &IRam[INTRAM+5] : &IRam[INTRAM+4]; break;
		case 0x05: *mb = port&1 ? &IRam[INTRAM+5] : &IRam[INTRAM+4]; break;
		case 0x06: *mb = port&1 ? &IRam[INTRAM+7] : &IRam[INTRAM+6]; break;
		case 0x07: *mb = port&1 ? &IRam[INTRAM+7] : &IRam[INTRAM+6]; break;
		}
		break;
		
	case 0x02:	// Ext RAM (こっちも16KB毎とすべき?)
		switch( addr ){
		case 0x00: *mb = &ERam[EXTRAM+0]; break;
		case 0x01: *mb = &ERam[EXTRAM+1]; break;
		case 0x02: *mb = &ERam[EXTRAM+2]; break;
		case 0x03: *mb = &ERam[EXTRAM+3]; break;
		case 0x04: *mb = &ERam[EXTRAM+4]; break;
		case 0x05: *mb = &ERam[EXTRAM+5]; break;
		case 0x06: *mb = &ERam[EXTRAM+6]; break;
		case 0x07: *mb = &ERam[EXTRAM+7]; break;
		}
		break;
		
	case 0x0b:	// Ext ROM1
		*mb = &ERom[EXTROM+1];
		break;
		
	case 0x0c:	// Ext ROM2
		*mb = &ERom[EXTROM+0];
		break;
		
	case 0x0d:	// CGROM
		switch( addr&0x01 ){
		case 0x00: *mb = &IRom[CGROM1]; break;
		case 0x01: *mb = &IRom[CGROM2]; break;
		}
		break;
		
	case 0x0e:	// System Rom2
		switch( addr ){
		case 0x00: *mb = &IRom[SRMENROM+0]; break;
		case 0x01: *mb = &IRom[SRMENROM+1]; break;
		case 0x02: *mb = &IRom[VOICEROM+0]; break;
		case 0x03: *mb = &IRom[VOICEROM+1]; break;
		case 0x04: *mb = &IRom[KANJIROM+0]; break;
		case 0x05: *mb = &IRom[KANJIROM+1]; break;
		case 0x06: *mb = &IRom[KANJIROM+2]; break;
		case 0x07: *mb = &IRom[KANJIROM+3]; break;
		}
		break;
		
	case 0x0f:	// System Rom1
		switch( addr ){
		case 0x00: *mb = &IRom[MAINROM+0]; break;
		case 0x01: *mb = &IRom[MAINROM+1]; break;
		case 0x02: *mb = &IRom[MAINROM+2]; break;
		case 0x03: *mb = &IRom[MAINROM+3]; break;
		case 0x04: *mb = &IRom[MAINROM+4]; break;
		case 0x05: *mb = &IRom[MAINROM+5]; break;
		case 0x06: *mb = &IRom[MAINROM+6]; break;
		case 0x07: *mb = &IRom[MAINROM+7]; break;
		}
		break;
		
	default:
		*mb = &IRom[EMPTYROM];
	}
	
	#if (MEM_LOG)
	PRINTD( MEM_LOG, "              [Read]\t\t[Write]\n" );
	{	int i = 0;
		for( auto &rb : RD_BlkSR ){
			PRINTD( MEM_LOG, "               %d:%8s\t%8s\n", i, rb->GetName( i<<MemBlock::PAGEBITS, true ).c_str(), rb->GetName( (i+1)<<MemBlock::PAGEBITS, true ).c_str() );
			i++;
		}
	}
	#endif
}


////////////////////////////////////////////////////////////////
// CG ROM アドレス設定
////////////////////////////////////////////////////////////////
void MEM62::SetCGrom( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetCGrom] -> %02x\n", data );
	
	// bit 7
	int	cgwait = data & 0x80 ? 0 : 1;
	IRom[CGROM1].SetWait( cgwait );
	IRom[CGROM2].SetWait( cgwait );
	
	// bit 6 CG ROMアクセスフラグ true:アクセス可 false:アクセス不可
	cgenable = data & 0x40 ? true : false;
	
	// bit 5,4,3 CG ROMアドレスイネーブル
	cgaden   = (~data >> 3) & 7;
	
	// bit 2,1,0 CG ROMアドレス A13,14,15
	// 予めcgadenでマスクしておく
	cgaddr   = data & cgaden;
}


////////////////////////////////////////////////////////////////
// CG ROM 選択
////////////////////////////////////////////////////////////////
void MEM62::SelectCGrom( bool mode )
{
	PRINTD( MEM_LOG, "[MEM][SelectCGrom] -> %s\n", mode ? "1:32*16(N60)" : "0:40*20(N60m)" );
	
	// mode 1:32*16(N60モード) 0:40*20(N60mモード)
	cgrom = mode;
}


////////////////////////////////////////////////////////////////
// CG ROM BANK 選択
////////////////////////////////////////////////////////////////
void MEM6::SetCGBank( bool data )
{
	PRINTD( MEM_LOG, "[MEM][SetCGBank] -> %s\n", data ? "Enable" : "Disable" );
	
	CGBank = data;
}


////////////////////////////////////////////////////////////////
// 漢字ROMおよび音声合成ROM設定
////////////////////////////////////////////////////////////////
void MEM62::SetKanjiRom( BYTE mode )
{
	PRINTD( MEM_LOG, "[MEM][SetKanjiRom] -> %02X\n", mode );
	
	// mode bit0 0:音声合成ROM選択 1:漢字ROM選択
	//      bit1 0:漢字ROM左側     1:漢字ROM右側
	if( c2acc & 2 ) kj_LR  = mode&2 ? true : false;	// 漢字 左？右？
	if( c2acc & 1 ) kj_rom = mode&1 ? true : false;	// 漢字ROM？音声合成ROM？
}


////////////////////////////////////////////////////////////////
// 漢字ROMおよび音声合成ROM取得
////////////////////////////////////////////////////////////////
BYTE MEM62::GetKanjiRom( void ) const
{
	return 0xfc | kj_LR ? 2 : 0 | kj_rom ? 1 : 0;
}


////////////////////////////////////////////////////////////////
// PortC2Hアクセス設定
////////////////////////////////////////////////////////////////
void MEM62::SetPortC2HAccess( BYTE data )
{
	PRINTD( MEM_LOG, "[MEM][SetPortC2HAccess] -> %02X\n", data );
	
	// 0:入力 1:出力
	c2acc = data;
}


////////////////////////////////////////////////////////////////
// 戦士のカートリッジmkⅡ メモリバンクレジスタ設定
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
		ERam[area].SetMemory( Stringf( "EROM%02d", data ), ExtRom( data ) );
		
		// Bank Setが変わった場合の修正
		{	int i = 0;
			for( auto &sb : SolBank ){
				if( ((sb & 0xc0) == ROMBANK) && ((sb & 0x30) != SolBankSet) ){
					sb = (sb & 0x0f) | SolBankSet | ROMBANK;
					ERam[i].SetMemory( nullptr, ExtRom( sb & 0x3f ) );
				}
				i++;
			}
		}
		break;
		
	case RAMBANK:	// RAM
		ERam[area].SetMemory( Stringf( "ERAM%02d", data & (ExtRam.Size()-1) ), ExtRam( data & (ExtRam.Size()-1) ) );
		break;
		
	case SCCBANK:	// SCC
		ERam[area].SetFunc( "SCC", FR( STATIC_CAST( RFuncPtr, &MEM6::SolSccRead ) ), FW( STATIC_CAST( WFuncPtr, &MEM6::SolSccWrite ) ) );
		break;
		
	case NONBANK:	// 無効
	default:
		ERam[area].SetFunc( "EMPTY", nullptr, nullptr );
	}
}





////////////////////////////////////////////////////////////////
// 直接アクセス関数
////////////////////////////////////////////////////////////////
BYTE MEM6::ReadSysRom   ( WORD addr ) const { return SysRom1.Read ( addr ); }
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
void MEM62::OutC1H( int, BYTE data ){ SelectCGrom( (data & 2) ); }
void MEM62::OutC2H( int, BYTE data ){ SetKanjiRom( data ); }
void MEM62::OutC3H( int, BYTE data ){ SetPortC2HAccess( data ); }
void MEM62::OutF0H( int, BYTE data ){ SetMemBlockR( data, Rf[1] ); }
void MEM62::OutF1H( int, BYTE data ){ SetMemBlockR( Rf[0], data ); }
void MEM62::OutF2H( int, BYTE data ){ SetMemBlockW( data ); }
void MEM62::OutF3H( int, BYTE data ){ SetWait( data ); }
void MEM62::OutF8H( int, BYTE data ){ SetCGrom( data ); }

void MEM64::Out6xH( int port, BYTE data ){ SetMemBlockSR( port, data ); }

BYTE MEM62::InC2H( int ){ return GetKanjiRom(); }
BYTE MEM62::InF0H( int ){ return Rf[0]; }
BYTE MEM62::InF1H( int ){ return Rf[1]; }
BYTE MEM62::InF2H( int ){ return Rf[2]; }
BYTE MEM62::InF3H( int ){ return GetWait() | 0x1f; }

BYTE MEM64::In6xH( int port ){ return RfSR[port & 0x0f]; }
BYTE MEM64::InB2H( int port ){ return 0xfd; }	// bit1 0:mk2SR 1:66SR
BYTE MEM68::InB2H( int port ){ return 0xff; }	// bit1 0:mk2SR 1:66SR


// 拡張カートリッジ ============================================
// 拡張漢字ROMカートリッジ -------------------------------------
void MEM6::OutFCH( int port, BYTE data )
{
	Kaddr   = (data << 8) | ((port >> 8) & 0xff);
	Kenable = false;
}

void MEM6::OutFFH( int, BYTE ){ Kenable = !Kenable; }
BYTE MEM6::InFDH( int ){ return Kenable ? ExtRom.Read( (Kaddr << 1)     ) : 0xff; }
BYTE MEM6::InFEH( int ){ return Kenable ? ExtRom.Read( (Kaddr << 1) | 1 ) : 0xff; }

// ボイスシンセサイザー ----------------------------------------
void MEM6::Out70H( int, BYTE ){}
void MEM6::Out72H( int, BYTE ){}
void MEM6::Out73H( int, BYTE ){}
void MEM6::Out74H( int, BYTE ){}
BYTE MEM6::In70H( int ){ return 0xff; }
BYTE MEM6::In72H( int ){ return 0xff; }
BYTE MEM6::In73H( int ){ return 0xff; }

// 戦士のカートリッジ(共通) ------------------------------------
void MEM6::Out7FH( int, BYTE data ){ SetSolBank( 2, ROMBANK | (data & 0xf) ); }

// 戦士のカートリッジ mkⅡ ------------------------------------
// 戦士のカートリッジ mkⅢ -------------------------------------
void MEM6::Out06H( int, BYTE data ){ Sol60Mode = (data == 0x66) ? true : false; }
void MEM6::Out3xH( int port, BYTE data ){ SetSolBank( port, data ); }

void MEM6::OutF0Hs( int, BYTE data )
{
	switch( data & 0x0f ){			// RF0下位 (0000 - 3FFF)
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
	switch( data & 0x03 ){			// 0000 - 3FFF
	case 0:
		CS01W = false;
		break;
		
	default:
		CS01W = true;
	}
	
	switch( (data >> 2) & 0x03 ){	// 4000 - 7FFF
	case 0:
		CS02W = false;
		break;
		
	default:
		CS02W = true;
	}
}
// =============================================================


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
	
	// 62,66,64,68
	for( int i=0; i<3; i++ ){
		Ini->SetVal( "MEMORY", Stringf( "Rf%d", i ), "", "0x%02X", Rf[i] );
	}
	
	// CGRomウェイト
	Ini->SetVal( "MEMORY", "CgRomWait",	"", IRom[CGROM1].GetWait() );
	
	// 内部RAM
	for( int i=0; i<(int)MemTable.IntRam->Size; i+=64 ){
		std::string strva;
		for( int j=0; j<64; j++ ){
			strva += Stringf( "%02X", IntRam.Read( i+j ) );
		}
		Ini->SetEntry( "MEMORY", Stringf( "IntRam_%04X", i ), "", strva.c_str() );
	}
	
	
	// 拡張カートリッジ ========================================
	Ini->SetVal( "MEMORY", "ExCart",	"", ExCart );
	
	// 外部ROM
	if( !FilePath.empty() ){
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
		Ini->SetVal( "MEMORY", "SolBank2",		"", "0x%02X", SolBank[2] );
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		Ini->SetVal( "MEMORY", "Soldier60",		"", Sol60Mode  );
		Ini->SetVal( "MEMORY", "CS01R",			"", CS01R      );
		Ini->SetVal( "MEMORY", "CS01W",			"", CS01W      );
		Ini->SetVal( "MEMORY", "CS02W",			"", CS02W      );
		Ini->SetVal( "MEMORY", "SoldierBank",	"", SolBankSet );
		// メモリバンクレジスタ
		{	int i = 0;
			for( auto &sb : SolBank ){
				Ini->SetVal( "MEMORY", Stringf( "SolBank%d", i ), "", "0x%02X", sb );
				i++;
			}
		}
		break;
	}
	// =========================================================
	
	return true;
}

bool MEM62::DokoSave( cIni* Ini )
{
	if( !MEM6::DokoSave( Ini ) ) return false;
	
	Ini->SetVal( "MEMORY", "cgrom",			"", cgrom    );
	Ini->SetVal( "MEMORY", "kj_rom",		"", kj_rom   );
	Ini->SetVal( "MEMORY", "kj_LR",			"", kj_LR    );
	Ini->SetVal( "MEMORY", "cgenable",		"", cgenable );
	Ini->SetVal( "MEMORY", "cgaden",		"", cgaden   );
	Ini->SetVal( "MEMORY", "cgaddr",		"", cgaddr   );
	Ini->SetVal( "MEMORY", "c2acc",			"", c2acc    );
	
	// メモリウェイト
	Ini->SetVal( "MEMORY", "Wait",		"", GetWait() );
	return true;
}

bool MEM64::DokoSave( cIni* Ini )
{
	if( !MEM62::DokoSave( Ini ) ) return false;
	
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
	
	// 62,66,64,68
	for( int i=0; i<3; i++ ){
		Ini->GetVal( "MEMORY", Stringf( "Rf%d", i ), Rf[i] );
	}
	
	// メモリブロック設定
	InitInt();
	SetMemBlockR( Rf[0], Rf[1] );
	SetMemBlockW( Rf[2] );
	SetCGBank( CGBank );
	
	// CGRomウェイト
	st = IRom[CGROM1].GetWait();
	Ini->GetVal( "MEMORY", "CgRomWait", st );
	st &= 0xff;
	IRom[CGROM1].SetWait( st );
	IRom[CGROM2].SetWait( st );
	
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
	
	
	// 拡張カートリッジ ========================================
	Ini->GetVal( "MEMORY", "ExCart",	ExCart   );
	
	// 外部メモリ初期化
	InitExt();
	
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
		Ini->GetVal( "MEMORY", "SolBank2",		SolBank[2] );
		break;
		
	case EXCSOL2:	// 戦士のカートリッジmkⅡ
	case EXCSOL3:	// 戦士のカートリッジmkⅢ
		Ini->GetVal( "MEMORY", "Soldier60",		Sol60Mode  );
		Ini->GetVal( "MEMORY", "CS01R",			CS01R      );
		Ini->GetVal( "MEMORY", "CS01W",			CS01W      );
		Ini->GetVal( "MEMORY", "CS02W",			CS02W      );
		Ini->GetVal( "MEMORY", "SoldierBank",	SolBankSet );
		// メモリバンクレジスタ
		{	int i = 0;
			for( auto &sb : SolBank ){
				Ini->GetVal( "MEMORY", Stringf( "SolBank%d", i ), sb );
				SetSolBank( i, sb );	// メモリバンク設定
				i++;
			}
		}
		break;
	}
	// =========================================================
	
	return true;
}

bool MEM62::DokoLoad( cIni* Ini )
{
	int st;
	
	if( !MEM6::DokoLoad( Ini ) ) return false;
	
	Ini->GetVal( "MEMORY", "cgrom",			cgrom    );
	Ini->GetVal( "MEMORY", "kj_rom",		kj_rom   );
	Ini->GetVal( "MEMORY", "kj_LR",			kj_LR    );
	Ini->GetVal( "MEMORY", "cgenable",		cgenable );
	Ini->GetVal( "MEMORY", "cgaden",		cgaden   );
	Ini->GetVal( "MEMORY", "cgaddr",		cgaddr   );
	Ini->GetVal( "MEMORY", "c2acc",			c2acc    );
	
	// メモリウェイト
	st = GetWait();
	Ini->GetVal( "MEMORY", "Wait",      st );
	SetWait( st );
	
	return true;
}

bool MEM64::DokoLoad( cIni* Ini )
{
	if( !MEM62::DokoLoad( Ini ) ) return false;
	
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
	 return RBLK[blk]->GetName( blk<<MemBlock::PAGEBITS );
}

const std::string& MEM64::GetReadMemBlk( int blk ) const
{
	return vm->VdgIsSRmode() ? RD_BlkSR[blk]->GetName( blk<<MemBlock::PAGEBITS )
							 : RBLK    [blk]->GetName( blk<<MemBlock::PAGEBITS );
}


const std::string& MEM6::GetWriteMemBlk( int blk ) const
{
	return WBLK[blk]->GetName( blk<<MemBlock::PAGEBITS, true );
}

const std::string& MEM64::GetWriteMemBlk( int blk ) const
{
	return vm->VdgIsSRmode() ? WR_BlkSR[blk]->GetName( blk<<MemBlock::PAGEBITS, true )
							 : WBLK    [blk]->GetName( blk<<MemBlock::PAGEBITS, true );
}
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

