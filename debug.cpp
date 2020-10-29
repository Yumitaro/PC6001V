#include <algorithm>
#include <cctype>
#include <fstream>

#include "breakpoint.h"
#include "common.h"
#include "debug.h"
#include "osd.h"
#include "pc6001v.h"
#include "p6el.h"
#include "p6vm.h"
#include "schedule.h"


#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#define	SCRWINW		(380)
#define	SCRWINH		(250)

#define	REGWINW		(40+32)
#define	REGWINH		( 8+10)
#define	MEMWINW		(72)
#define	MEMWINH		(31)
#define	MONWINW		(60)
#define	MONWINH		(30)

#define	PROMPT		"P6V>"

#define	MAX_CHRS	(256)	// キーバッファ最大値
#define	MAX_HIS		(256)	// ヒストリバッファ最大値


//------------------------------------------------------
//  モニタモードウィンドウ インターフェース(?)クラス
//------------------------------------------------------
////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
iMon::iMon( const std::shared_ptr<VM6>& vm ) : vm( vm ), x( 0 ), y( 0 )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
iMon::~iMon( void )
{
}


////////////////////////////////////////////////////////////////
// X座標取得
////////////////////////////////////////////////////////////////
int iMon::X( void )
{
	return x;
}


////////////////////////////////////////////////////////////////
// Y座標取得
////////////////////////////////////////////////////////////////
int iMon::Y( void )
{
	return y;
}


////////////////////////////////////////////////////////////////
// X座標設定
////////////////////////////////////////////////////////////////
void iMon::SetX( int xx )
{
	x = xx;
}


////////////////////////////////////////////////////////////////
// Y座標設定
////////////////////////////////////////////////////////////////
void iMon::SetY( int yy )
{
	x = yy;
}



//------------------------------------------------------
//  メモリダンプウィンドウクラス
//------------------------------------------------------
////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
cWndMem::cWndMem( const std::shared_ptr<VM6>& vm ) : iMon( vm ), Addr( 0 )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
cWndMem::~cWndMem( void )
{
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool cWndMem::Init( void )
{
	// 表示アドレス初期化
	Addr = 0;
	
	return ZCons::Init( MEMWINW, MEMWINH, "MEMORY" );
}


////////////////////////////////////////////////////////////////
// ウィンドウ更新
////////////////////////////////////////////////////////////////
void cWndMem::Update( void )
{
	WORD addr = Addr;
	const DWORD TextCol[] = { FC_GRAY,  FC_BLUE,  FC_GREEN,  FC_CYAN,  FC_RED,  FC_MAGENTA,  FC_YELLOW,  FC_WHITE };
	const DWORD BackCol[] = { FC_BLACK, FC_DBLUE, FC_DGREEN, FC_DCYAN, FC_DRED, FC_DMAGENTA, FC_DYELLOW, FC_GRAY  };
	int i,j;
	
	ZCons::Locate( 0, 0 );
	ZCons::SetColor( FC_YELLOW, FC_DYELLOW );
	ZCons::SPrint( "MAP " );
	ZCons::SetColor( FC_WHITE, FC_DCYAN );
	ZCons::SPrintc( "  0000   2000   4000   6000   8000   A000   C000   E000   \n" );
	ZCons::SetColor( FC_WHITE, FC_BLACK );
	
	// Read メモリブロック名表示
	ZCons::SPrint( "READ  " );
	for( i=0; i<8; i++ ){
		ZCons::SetColor( TextCol[i] );
		ZCons::SPrint( Stringf( "%-7s", vm->MemGetReadMemBlk( i ).c_str() ) );
	}
	ZCons::SPrintc( "\nWRITE " );
	
	// Write メモリブロック名表示
	for( i=0; i<8; i++ ){
		ZCons::SetColor( TextCol[i] );
		ZCons::SPrint( Stringf( "%-7s", vm->MemGetWriteMemBlk( i ).c_str() ) );
	}
	ZCons::SPrintc( "\n" );
	
	// メモリダンプ表示
	ZCons::SetColor( FC_YELLOW, FC_DYELLOW );
	ZCons::SPrint( "ADDR" );
	ZCons::SetColor( FC_WHITE, FC_DCYAN );
	ZCons::SPrint( " 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F                 " );
	ZCons::SetColor( FC_WHITE, FC_BLACK );
	
	for( i=0; i<16; i++ ){
		ZCons::SPrintc( "\n" );
		ZCons::SetColor( FC_WHITE, BackCol[ addr>>13 ] );
		ZCons::SPrint( Stringf( "%04X", addr ) );
		ZCons::SetColor( FC_WHITE, FC_BLACK );
		ZCons::SPrint( " " );
		for( j=0; j<16; j++)
			ZCons::SPrint( Stringf( "%02X ", vm->MemRead(addr+j) ) );
		for( j=0; j<16; j++)
			ZCons::PutCharH( vm->MemRead(addr+j) );
		addr += 16;
	}
}


////////////////////////////////////////////////////////////////
// 表示アドレス設定
////////////////////////////////////////////////////////////////
void cWndMem::SetAddress( WORD addr )
{
	Addr = addr & 0xfff8;
}


////////////////////////////////////////////////////////////////
// 表示アドレス取得
////////////////////////////////////////////////////////////////
WORD cWndMem::GetAddress( void )
{
	return Addr;
}



//------------------------------------------------------
//  レジスタウィンドウクラス
//------------------------------------------------------
////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
cWndReg::cWndReg( const std::shared_ptr<VM6>& vm ) : iMon( vm )
{
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
cWndReg::~cWndReg( void )
{
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool cWndReg::Init( void )
{
	return ZCons::Init( REGWINW, REGWINH, "REGISTER" );
}


////////////////////////////////////////////////////////////////
// ウィンドウ更新
////////////////////////////////////////////////////////////////
void cWndReg::Update( void )
{
	const char flags[9] = "SZ.H.PNC";
	cZ80::Register reg;
	std::string DisCode;
	int i,j;
	
	// レジスタ値取得
	vm->CpumGetRegister( &reg );
	
	// 1ライン逆アセンブル
	vm->CpumDisasm( DisCode, reg.PC.W );
	
	ZCons::Locate( 0, 0 ); ZCons::SPrint( Stringf( "AF :%04X BC :%04X DE :%04X HL :%04X", reg.AF.W, reg.BC.W, reg.DE.W, reg.HL.W ) );
	ZCons::Locate( 0, 1 ); ZCons::SPrint( Stringf( "AF':%04X BC':%04X DE':%04X HL':%04X", reg.AF1.W, reg.BC1.W, reg.DE1.W, reg.HL1.W ) );
	ZCons::Locate( 0, 2 ); ZCons::SPrint( Stringf( "IX :%04X IY :%04X PC :%04X SP :%04X", reg.IX.W, reg.IY.W, reg.PC.W, reg.SP.W ) );
	
	ZCons::Locate( 0, 3 ); ZCons::SPrint( "FLAG:" );
	for( j=0,i=reg.AF.B.l; j<8; j++,i<<=1 ){
		ZCons::SetColor( i&0x80 ? FC_WHITE : FC_GRAY );
		ZCons::SPrint( Stringf( "%c", flags[j] ) );
	}
	ZCons::SetColor( FC_WHITE );
	ZCons::SPrint( Stringf( " I:%02X IFF:%d IM:%1d HALT:%1d", reg.I, reg.IFF, reg.IM, reg.Halt ) );
	
	ZCons::SetColor( FC_WHITE, FC_DCYAN );
	ZCons::Locate( 0, 4 ); ZCons::SPrint( Stringf( "%-36s", DisCode.c_str() ) );
	ZCons::SetColor( FC_WHITE, FC_BLACK );

	ZCons::Locate( 0, 5 );

// PRINTER STROBE 0/1
	ZCons::SPrintc( Stringf( "CRT  :%s\n", vm->VdgGetCrtDisp() ? "DISP" : "KILL" ) );
// CGROM ON/OFF
	ZCons::SPrintc( Stringf( "TIMER:%s\n", vm->IntGetTimerIntr() ? "ON" : "OFF" ) );
	ZCons::SPrintc( Stringf( "VRAM:%04X ATTR:%04X\n", vm->VdgGetVramAddr(), vm->VdgGerAttrAddr() ) );
// RELAY ON/OFF





}


//------------------------------------------------------
//  モニタウィンドウクラス
//------------------------------------------------------

////////////////////////////////////////////////////////////////
// 命令の種類判定テーブル
////////////////////////////////////////////////////////////////
enum MonitorJob{	// ジョブ
	MONITOR_NONE = 0,
	
	MONITOR_HELP,
	
	MONITOR_GO,
	MONITOR_TRACE,
	MONITOR_STEP,
	MONITOR_STEPALL,
	MONITOR_BREAK,
	
	MONITOR_READ,
	MONITOR_WRITE,
	MONITOR_FILL,
	MONITOR_MOVE,
	MONITOR_SEARCH,
	MONITOR_OUT,
	MONITOR_LOADMEM,
	MONITOR_SAVEMEM,
	
	MONITOR_RESET,
	MONITOR_REG,
	MONITOR_DISASM
};

struct MonCmd{
	MonitorJob Step;
	const std::string cmd;
	const std::string HelpMes;
};

const std::vector<MonCmd> MonitorCmd = {
	{ MONITOR_HELP,		"help",		"ヘルプを表示" },
	{ MONITOR_HELP,		"?",		"    〃" },
	{ MONITOR_GO,		"go",		"実行" },
	{ MONITOR_GO,		"g",		"    〃" },
	{ MONITOR_TRACE,	"trace",	"トレース実行" },
	{ MONITOR_TRACE,	"t",		"    〃" },
	{ MONITOR_STEP,		"step",		"ステップ実行" },
	{ MONITOR_STEP,		"s",		"    〃" },
	{ MONITOR_STEPALL,	"S",		"    〃" },
	{ MONITOR_BREAK,	"break",	"ブレークポイント設定" },
	{ MONITOR_BREAK,	"b",		"    〃" },
	{ MONITOR_READ,		"read",		"メモリを読込む" },
	{ MONITOR_WRITE,	"write",	"メモリに書込む" },
	{ MONITOR_FILL,		"fill",		"メモリを埋める" },
	{ MONITOR_MOVE,		"move",		"メモリを移動" },
	{ MONITOR_SEARCH,	"search",	"メモリを検索" },
	{ MONITOR_OUT,		"out",		"ポート出力" },
	{ MONITOR_LOADMEM,	"loadmem",	"ファイルからメモリに読込む" },
	{ MONITOR_SAVEMEM,	"savemem",	"メモリからファイルに書込む" },
	{ MONITOR_RESET,	"reset",	"PC6001Vをリセット" },
	{ MONITOR_REG,		"reg",		"CPUレジスタを参照/設定" },
	{ MONITOR_DISASM,	"disasm",	"逆アセンブル" }
};


////////////////////////////////////////////////////////////////
// 引数の種類判定テーブル
////////////////////////////////////////////////////////////////
enum ArgvType{
	ARGV_END	= 0x00000,
	ARGV_STR	= 0x00001,	// strings
	ARGV_PORT	= 0x00002,	// 0～0xff
	ARGV_ADDR	= 0x00004,	// 0～0xffff
	ARGV_NUM	= 0x00008,	// 0～0x7fffffff
	ARGV_INT	= 0x00010,	// -0x7fffffff～0x7fffffff
	ARGV_SIZE	= 0x00080,	// #1～#0x7fffffff
	ARGV_REG	= 0x00400,	// RegisterName
	ARGV_BREAK	= 0x00800,	// BreakAction
	ARGV_STEP	= 0x04000	// StepCommand
};


enum ArgvName{
	// <reg>
	ARG_AF,		ARG_BC,		ARG_DE,		ARG_HL,
	ARG_IX,		ARG_IY,		ARG_SP,		ARG_PC,
	ARG_AF1,	ARG_BC1,	ARG_DE1,	ARG_HL1,
	ARG_I,		ARG_R,
	ARG_IFF,	ARG_IM,		ARG_HALT,
	
	// <action>
	//ARG_PC,
	ARG_READ,	ARG_WRITE,	ARG_IN,
	ARG_OUT,	ARG_CLEAR,
	
	// step <cmd>
	//ARG_ALL
	ARG_CALL,	ARG_JP,		ARG_REP,
	
	// reg all
	ARG_ALL
};


struct MonArgv{
	const std::string Str;
	int Type;
	int Val;
};

const std::vector<MonArgv> MonitorArgv = {
	// <reg>
	{ "AF",		ARGV_REG,	ARG_AF,		},
	{ "BC",		ARGV_REG,	ARG_BC,		},
	{ "DE",		ARGV_REG,	ARG_DE,		},
	{ "HL",		ARGV_REG,	ARG_HL,		},
	{ "IX",		ARGV_REG,	ARG_IX,		},
	{ "IY",		ARGV_REG,	ARG_IY,		},
	{ "SP",		ARGV_REG,	ARG_SP,		},
	{ "PC",		ARGV_REG,	ARG_PC,		},
	{ "AF'",	ARGV_REG,	ARG_AF1,	},
	{ "BC'",	ARGV_REG,	ARG_BC1,	},
	{ "DE'",	ARGV_REG,	ARG_DE1,	},
	{ "HL'",	ARGV_REG,	ARG_HL1,	},
	{ "I",		ARGV_REG,	ARG_I,		},
	{ "R",		ARGV_REG,	ARG_R,		},
	{ "IFF",	ARGV_REG,	ARG_IFF,	},
	{ "IM",		ARGV_REG,	ARG_IM,		},
	{ "HALT",	ARGV_REG,	ARG_HALT,	},
	
	//<action>
	{ "PC",		ARGV_BREAK,	ARG_PC,		},
	{ "READ",	ARGV_BREAK,	ARG_READ,	},
	{ "WRITE",	ARGV_BREAK,	ARG_WRITE,	},
	{ "IN",		ARGV_BREAK,	ARG_IN,		},
	{ "OUT",	ARGV_BREAK,	ARG_OUT,	},
	{ "CLEAR",	ARGV_BREAK,	ARG_CLEAR,	},
	
	// step
	{ "CALL",	ARGV_STEP,	ARG_CALL,	},
	{ "JP",		ARGV_STEP,	ARG_JP,		},
	{ "REP",	ARGV_STEP,	ARG_REP,	},
	{ "ALL",	ARGV_STEP,	ARG_ALL,	}
};


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
cWndMon::cWndMon( const std::shared_ptr<VM6>& vm ) : iMon( vm ), ArgvCounter( 0 )
{
	KeyBuf.clear();
	HisBuf.clear();
	Argv.clear();
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
cWndMon::~cWndMon( void )
{
}


////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool cWndMon::Init( void )
{
	if( ZCons::Init( MONWINW, MONWINH, "" ) ){
		// 最初だけメッセージ表示
		ZCons::SPrintc( "***********************************************\n" );
		ZCons::SPrintc( "* PC6001V  - monitor mode -                   *\n" );
		ZCons::SPrintc( "*  help 又は ? と入力するとヘルプを表示します *\n" );
		ZCons::SPrintc( "***********************************************\n\n" );
		ZCons::SPrint( PROMPT );
		return true;
	}
	
	return false;
}


////////////////////////////////////////////////////////////////
// ウィンドウ更新
////////////////////////////////////////////////////////////////
void cWndMon::Update( void )
{
}


////////////////////////////////////////////////////////////////
// キー入力処理
////////////////////////////////////////////////////////////////
void cWndMon::KeyIn( int kcode, int ccode )
{
	static int LastKey  = KVC_ENTER;	// 前回のキー
	static int HisLevel = 0;			// ヒストリレベル
	
	switch( kcode ){		// キーコード
	case KVC_ENTER:			// Enter
	case KVC_P_ENTER:		// Enter(テンキー)
		ZCons::SPrintc( "\n" );
		
		if( !KeyBuf.empty() ){	// キーバッファに文字列あり
			// 末尾のスペースを削除
			while( KeyBuf.substr( KeyBuf.length()-1, 1) == " " )
				KeyBuf.pop_back();
			
			auto result = std::find( HisBuf.begin(), HisBuf.end(), KeyBuf );
			
			// ヒストリに含まれるなら一旦削除
			if( result != HisBuf.end() )
				HisBuf.erase( HisBuf.begin() + std::distance( HisBuf.begin(), result ) );
			
			// ヒストリバッファがいっぱいなら削除
			while( HisBuf.size() >= MAX_HIS )
				HisBuf.erase( HisBuf.begin() );
			
			// キーバッファをヒストリバッファにコピー
			if( HisBuf.size() < MAX_HIS )
				HisBuf.emplace_back( KeyBuf );
		}
		
		// ここで引数解析処理
		Exec( GetArg() );
		
		KeyBuf.clear();						// キーバッファクリア
		ZCons::SPrint( PROMPT );
		break;
		
	case KVC_UP:			// 上矢印
	case KVC_DOWN:			// 下矢印
		if( !HisBuf.size() ) break;
		
		if( LastKey == KVC_UP || LastKey == KVC_DOWN ){
			if( kcode == KVC_UP   && HisLevel < (int)HisBuf.size() ) HisLevel++;
			if( kcode == KVC_DOWN && HisLevel > 0 )                  HisLevel--;
		}else{
			if( kcode == KVC_UP ) HisLevel = 1;
			else                  HisLevel = 0;
		}
		
		// 今のコマンドラインを消去
		while( !KeyBuf.empty() ){
			ZCons::SPrintc( "\b" );
			KeyBuf.pop_back();
		}
		
		// ヒストリバッファからキーバッファにコピーして表示
		if( HisLevel > 0 ){
			KeyBuf = HisBuf[HisBuf.size() - HisLevel];
			ZCons::SPrint( KeyBuf );
		}
		break;
		
	case KVC_BACKSPACE:		// BackSpace
		if( !KeyBuf.empty() ){
			ZCons::SPrintc( "\b" );
			KeyBuf.pop_back();
		}
		break;
		
	default:
		// 有効な文字コードかつバッファがあふれていなければ
		if( ( ccode > 0x1f ) && ( ccode < 0x80 ) ){
			if( KeyBuf.length() < (MAX_CHRS-1) ){
				KeyBuf.push_back( ccode );
				ZCons::SPrint( Stringf( "%c", ccode ) );
			}
		}
	}
	
	LastKey = kcode;
}


////////////////////////////////////////////////////////////////
// ブレークポイント到達
////////////////////////////////////////////////////////////////
void cWndMon::BreakIn( WORD addr )
{
	ZCons::SetColor( FC_YELLOW );
	ZCons::SPrintc( Stringf( "\n << Break in %04XH >>", addr ) );
	switch( vm->BpGetType( vm->BpGetReqNum() ) ){
	case BPoint::BP_READ:	ZCons::SPrint( Stringf( " Read Memory %04XH",    vm->BpGetAddr( vm->BpGetReqNum() ) ) );	break;
	case BPoint::BP_WRITE:	ZCons::SPrint( Stringf( " Write Memory %04XH",   vm->BpGetAddr( vm->BpGetReqNum() ) ) );	break;
	case BPoint::BP_IN:		ZCons::SPrint( Stringf( " Read I/O Port %02XH",  vm->BpGetAddr( vm->BpGetReqNum() ) ) );	break;
	case BPoint::BP_OUT:	ZCons::SPrint( Stringf( " Write I/O Port %02XH", vm->BpGetAddr( vm->BpGetReqNum() ) ) );	break;
	default:				break;
	}
	ZCons::SetColor( FC_WHITE );
	ZCons::SPrintc( "\n" PROMPT );
}


////////////////////////////////////////////////////////////////
// 引数処理
////////////////////////////////////////////////////////////////
int cWndMon::GetArg( void )
{
	std::string::size_type ps = 0, pe = 0;
	
	ArgvCounter = 0;	// Shift()用カウンタ初期化
	
	// 引数分解
	Argv.clear();
	while( ps != std::string::npos ){
		pe = KeyBuf.find( ' ', ps );
		
		if( pe == std::string::npos ){
			if( KeyBuf.substr( ps ).length() )
				Argv.push_back( KeyBuf.substr( ps ) );
			break;
		}else{
			if( pe > ps ){
				Argv.push_back( KeyBuf.substr( ps, pe - ps ) );
			}
			ps = pe + 1;
		}
	}
	
	if( !Argv.size() )	// 空行?
		return MONITOR_NONE;
	
	// 有効命令探す
	for( auto &m : MonitorCmd ){
		if( !StriCmp( Argv.front(), m.cmd ) ){
			if( Argv.size() == 2 && (Argv[1] == "?") ){	// 引数が ? の場合
				Help( m.Step );
				return MONITOR_NONE;
			}else{										// 通常の命令の場合
				Shift();
				return m.Step;
			}
		}
	}
	
	// 無効命令の場合
	ZCons::SetColor( FC_RED );
	ZCons::SPrintc( Stringf( "無効なコマンドです : %s\n", Argv.front().c_str() ) );
	ZCons::SetColor( FC_WHITE );
	
	return MONITOR_NONE;
}


////////////////////////////////////////////////////////////////
// 引数配列シフト
////////////////////////////////////////////////////////////////
void cWndMon::Shift( void )
{
	bool size = false;
	size_t chk;
	
	// これ以上引数が無い
	if( Argv.size() <= 1 ){
		argv.Type = ARGV_END;
		Argv.clear();
	// まだ引数があるので解析
	}else{
		// 先頭要素(処理済み)を削除
		Argv.erase( Argv.begin() );
		
		argv.Type = ARGV_END;
		argv.Str  = Argv.front();
		
		// 大文字化
		std::transform( argv.Str.begin(), argv.Str.end(), argv.Str.begin(), ::toupper );
		
		if( argv.Str.front() == '#' ){
			size = true;
			argv.Str.erase( argv.Str.begin() );
		}
		
		try{
			argv.Val = std::stoi( argv.Str, &chk, 0 );
		}
		catch( std::logic_error& ){	// 変換失敗
			chk = 0;
		}
		
		// 数値の場合
		if( chk == argv.Str.length() ){
			if( size ){		// #で始まる
				if( argv.Val <= 0 ) argv.Type = ARGV_STR;
				else                argv.Type = ARGV_SIZE;
			}else{			// 数で始まる
				argv.Type |= ARGV_INT;
				if( argv.Val >= 0 )      argv.Type |= ARGV_NUM;
				if( argv.Val <= 0xff )   argv.Type |= ARGV_PORT;
				if( argv.Val <= 0xffff ) argv.Type |= ARGV_ADDR;
			}
		// 文字列の場合
		}else{
			if( size ){		// #で始まる
				argv.Type = ARGV_STR;
			}else{			// 字で始まる
				for( auto &m : MonitorArgv ){
					if( !StriCmp( argv.Str, m.Str ) ){
						argv.Type |= m.Type;
						argv.Val   = m.Val;
					}
				}
				if( argv.Type == ARGV_END ) argv.Type = ARGV_STR;
			}
		}
	}
	ArgvCounter++;
}


////////////////////////////////////////////////////////////////
// 引数エラーメッセージ処理
////////////////////////////////////////////////////////////////
#define ErrorMes()																\
	do{																			\
		ZCons::SetColor( FC_RED );												\
		ZCons::SPrintc( Stringf( "引数が無効です (arg %d)\n", ArgvCounter ) );	\
		ZCons::SetColor( FC_WHITE );											\
		return;																	\
	}while(0)


////////////////////////////////////////////////////////////////
// コマンド実行
////////////////////////////////////////////////////////////////
void cWndMon::Exec( int cmd )
{
// 処理された引数の種類をチェック
#define	ArgvIs( x )	(argv.Type & (x))

	switch( cmd ){
	case MONITOR_HELP:
	//--------------------------------------------------------------
	// help [<cmd>]
	//	ヘルプを表示する
	//--------------------------------------------------------------
	{
		std::string cmds = "";
		
		if( argv.Type != ARGV_END ){				// [cmd]
			cmds = argv.Str;
			Shift();
		}
		if( argv.Type != ARGV_END ) ErrorMes();		// 余計な引数があればエラー
		
		if( cmds.empty() ){	// 引数なし。全ヘルプ表示
			ZCons::SPrintc( "help\n" );
			for( auto &m : MonitorCmd )
				ZCons::SPrintc( Stringf( "  %-7s %s\n", m.cmd.c_str(), m.HelpMes.c_str() ) );
			ZCons::SPrintc( "     注: \"help <コマンド名>\" と入力すると\n         更に詳細なヘルプを表示します。\n" );
		}else{		// 引数のコマンドのヘルプ表示
			size_t i = 0;
			for( auto &m : MonitorCmd ){
				if( !StriCmp( cmds, m.cmd ) ){
					Help( m.Step );
					break;
				}
				i++;
			}
			if( i == MonitorCmd.size() ) ErrorMes();
		}
		
		break;
	}
	case MONITOR_GO:
	//--------------------------------------------------------------
	//  go
	//	 実行
	//--------------------------------------------------------------
	{
		if( argv.Type != ARGV_END ) ErrorMes();
		
		OSD_PushEvent( EV_DEBUGMODETOGGLE );
		
		break;
	}
	
	case MONITOR_TRACE:
	//--------------------------------------------------------------
	//  trace <step>
	//  trace #<step>
	//  指定したステップ分処理が変わるまで実行
	//--------------------------------------------------------------
	{
		int step = 1;
		
		if( argv.Type != ARGV_END ){
			if     ( ArgvIs( ARGV_SIZE ) ) step = argv.Val;	// [<step>]
			else if( ArgvIs( ARGV_NUM )  ) step = argv.Val;	// [#<step>]
			else                           ErrorMes();
			Shift();
		}
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		
		while( step-- ){
			int st = 0;
			while( st <= 0 ){	// バスリクエスト期間をスキップ
				st = vm->Emu();
				vm->EventUpdate( st <= 0 ? 1 : st );	// 1命令実行とイベント更新
			}
		}
		
//		if( CheckBreakPointPC() ) set_emumode( TRACE_BP );
//		else                      set_emumode( M_TRACE );
		
		break;
	}
	
	case MONITOR_STEP:
	//--------------------------------------------------------------
	//  step
	//  step [call] [jp] [rep] [all]
	//  1ステップ,実行
	//  CALL,DJNZ,LDIR etc のスキップも可
	//--------------------------------------------------------------
	{
		bool call = false, jp = false, rep = false;
		BYTE code;
		WORD addr;
		std::string DisCode;
		cZ80::Register reg;
		int st = 0;
		
		while( argv.Type != ARGV_END ){
			if( ArgvIs( ARGV_STEP ) ){
				if( argv.Val == ARG_CALL )	call = true;
				if( argv.Val == ARG_JP )	jp   = true;
				if( argv.Val == ARG_REP )	rep  = true;
				if( argv.Val == ARG_ALL )	call = jp = rep = true;
				Shift();
			}else
				ErrorMes();
		}
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->CpumGetRegister( &reg );
		
		addr = reg.PC.W;
		code = vm->MemRead( addr );
		
		if( call ){
			if( code		== 0xcd ||	// CALL nn    = 11001101B
			  ( code&0xc7 ) == 0xc4 ){	// CALL cc,nn = 11ccc100B
				addr += 3;
			}
		}
		
		if( jp ){
			if( code == 0x10 ){			// DJNZ e     = 00010000B
				addr += 2;
			}
		}
		
	    if( rep ){
			if( code == 0xed ){			// LDIR/LDDR/CPIR/CPDR etc
				code = vm->MemRead( addr+1 );
				if( (code&0xf4) == 0xb0 ){
					addr += 2;
				}
			}
		}
		
		vm->CpumDisasm( DisCode, addr );
		ZCons::SPrintc( Stringf( "%s\n", DisCode.c_str() ) );
		
		while( st <= 0 ){	// バスリクエスト期間をスキップ
			st = vm->Emu();
			vm->EventUpdate( st <= 0 ? 1 : st );	// 1命令実行とイベント更新
		}
		
		break;
	}	
	case MONITOR_STEPALL:
	//--------------------------------------------------------------
	//  S
	//  step all に同じ
	//--------------------------------------------------------------
	{
		BYTE code;
		WORD addr;
		std::string DisCode;
		cZ80::Register reg;
		int st = -1;
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->CpumGetRegister( &reg );
		
		addr = reg.PC.W;
		code = vm->MemRead( addr );
		
		if( code		== 0xcd ||	// CALL nn    = 11001101B
		  ( code&0xc7 ) == 0xc4 ){	// CALL cc,nn = 11ccc100B
			addr += 3;
		}
		
		if( code == 0x10 ){			// DJNZ e     = 00010000B
			addr += 2;
		}
		
		if( code == 0xed ){			// LDIR/LDDR/CPIR/CPDR etc
			code = vm->MemRead( addr+1 );
			if( (code&0xf4) == 0xb0 ){
				addr += 2;
			}
		}
		
		vm->CpumDisasm( DisCode, addr );
		ZCons::SPrintc( Stringf( "%s\n", DisCode.c_str() ) );
		
		while( st <= 0 ){	// バスリクエスト期間をスキップ
			st = vm->Emu();
			vm->EventUpdate( st <= 0 ? 1 : st );	// 1命令実行とイベント更新
		}
		
		break;
	}	
	case MONITOR_BREAK:
	//--------------------------------------------------------------
	//  break [PC|READ|WRITE|IN|OUT] <addr|port>
	//  break CLEAR [#<No>]
	//  break
	//  ブレークポイントの設定／解除／表示
	//--------------------------------------------------------------
	{
		bool show = false;
		int action = ARG_PC;
		WORD addr = 0;
		int number = 1;
		
		if( argv.Type != ARGV_END ){
			// <action>
			if( ArgvIs( ARGV_BREAK ) ){
				action = argv.Val;
				Shift();
			}
			
			// <addr|port>
			switch( action ){
			case ARG_IN:
			case ARG_OUT:
				if( !ArgvIs( ARGV_PORT ) ) ErrorMes();
				addr = argv.Val;
				Shift();
				break;
			case ARG_PC:
			case ARG_READ:
			case ARG_WRITE:
				if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
				addr = argv.Val;
				Shift();
				break;
			}
			
			// [#<No>]
			if( argv.Type != ARGV_END ){
				if( !ArgvIs( ARGV_SIZE ) ) ErrorMes();
				if( argv.Val < 1 || argv.Val > vm->BpGetNum() ) ErrorMes();
				number = argv.Val;
				Shift();
			}
		}else{
			show = true;
		}
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		
		if( show ){
			if( vm->BpGetNum() ){
				for( int i=1; i<=vm->BpGetNum(); i++ ){
					ZCons::SPrint( Stringf( "    #%02d  ", i ) );
					addr = vm->BpGetAddr( i );
					switch( vm->BpGetType( i ) ){
					case BPoint::BP_NONE:	ZCons::SPrintc( "-- なし --\n" );								break;
					case BPoint::BP_PC:		ZCons::SPrintc( Stringf( "PC   reach %04XH\n", addr&0xffff ) );	break;
					case BPoint::BP_READ:	ZCons::SPrintc( Stringf( "READ  from %04XH\n", addr&0xffff ) );	break;
					case BPoint::BP_WRITE:	ZCons::SPrintc( Stringf( "WRITE   to %04XH\n", addr&0xffff ) );	break;
					case BPoint::BP_IN:		ZCons::SPrintc( Stringf( "INPUT from %02XH\n", addr&0xff   ) );	break;
					case BPoint::BP_OUT:	ZCons::SPrintc( Stringf( "OUTPUT  to %04XH\n", addr&0xff   ) );	break;
					default:																				break;
					}
				}
			}else
				ZCons::SPrintc( "ブレークポイントは設定されていません。\n" );
		}else{
			if( action == ARG_CLEAR ){
				vm->BpDelete( number );
				ZCons::SPrintc( Stringf( "ブレークポイント #%02d を消去します。\n", number ) );
			}else{
				std::string s = "";
				
				switch( action ){
				case ARG_PC:	vm->BpSet( BPoint::BP_PC,    addr );	s = "PC : %04XH";		break;
				case ARG_READ:	vm->BpSet( BPoint::BP_READ,  addr );	s = "READ : %04XH";		break;
				case ARG_WRITE:	vm->BpSet( BPoint::BP_WRITE, addr );	s = "WRITE : %04XH";	break;
				case ARG_IN:	vm->BpSet( BPoint::BP_IN,    addr );	s = "IN : %02XH";		break;
				case ARG_OUT:	vm->BpSet( BPoint::BP_OUT,   addr );	s = "OUT : %02XH";		break;
				default:																				break;
				}
				ZCons::SPrint( Stringf( "ブレークポイント #%02d を設定します。[ ", vm->BpGetNum() ) );
				ZCons::SPrint( Stringf( s, addr ) );
				ZCons::SPrintc( " ]\n" );
			}
		}
		break;
	}
	case MONITOR_READ:
	//--------------------------------------------------------------
	//  read <addr>
	//  特定のアドレスをリード
	//--------------------------------------------------------------
		break;
		
	case MONITOR_WRITE:
	//--------------------------------------------------------------
	//  write <addr> <data>
	//  特定のアドレスにライト
	//--------------------------------------------------------------
	{
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR )) ErrorMes();
		WORD addr = argv.Val;
		Shift();
		
		// <data>
		if( !ArgvIs( ARGV_INT )) ErrorMes();
		BYTE data = argv.Val;
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->MemWrite( addr, data );
		
		ZCons::SPrint( Stringf( "WRITE memory [ %04XH ] <- %02X  (= %d | %+d | ", addr, (BYTE)data, (BYTE)data, (int8_t)data ) );
		int i,j;
		for( i=0, j=0x80; i<8; i++, j>>=1 ) ZCons::SPrint( Stringf( "%d", (data & j) ? 1 : 0 ) );
		ZCons::SPrintc( "B )\n");
		
		break;
	}
	
	case MONITOR_FILL:
	//--------------------------------------------------------------
	//  fill <start-addr> <end-addr> <value>
	//  fill <start-addr> #<size> <value>
	//  メモリを埋める
	//--------------------------------------------------------------
	{
		int start, size, value;
		
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
		start = argv.Val;
		Shift();
		
		// [<addr|#size>]
		if     ( ArgvIs( ARGV_SIZE ) ) size = argv.Val;
		else if( ArgvIs( ARGV_ADDR ) ) size = argv.Val - start +1;
		else                           ErrorMes();
		Shift();
		
		// <data>
		if( !ArgvIs( ARGV_INT )) ErrorMes();
		value = argv.Val;
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		for( int i=0; i<size; i++ ) vm->MemWrite( start+i, value );
		
		break;
	}
	
	case MONITOR_MOVE:
	//--------------------------------------------------------------
	//  move <src-addr> <end-addr> <dist-addr>
	//  move <src-addr> #size      <dist-addr>
	//  メモリ転送
	//--------------------------------------------------------------
	{
		int start, size, dist;
		
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
		start = argv.Val;
		Shift();
		
		// [<addr|#size>]
		if     ( ArgvIs( ARGV_SIZE ) ) size = argv.Val;
		else if( ArgvIs( ARGV_ADDR ) ) size = argv.Val - start +1;
		else                           ErrorMes();
		Shift();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
		dist = argv.Val;
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		// 転送元-転送先が 重ならない
		if( start+size <= dist ) for( int i=0; i<size; i++ )    vm->MemWrite( dist+i, vm->MemRead( start+i ) );
		// 転送元-転送先が 重なる
		else                     for( int i=size-1; i>=0; i-- ) vm->MemWrite( dist+i, vm->MemRead( start+i ) );
		
		break;
	}
	
	case MONITOR_SEARCH:
	//--------------------------------------------------------------
	//  search [<value> [<start-addr> <end-addr>]]
	//  search [<value> [<start-addr> #<size>]]
	//  特定の定数 (1バイト) をサーチ
	//--------------------------------------------------------------
		break;
		
	case MONITOR_OUT:
	//--------------------------------------------------------------
	//  out <port> <data>
	//  特定のポートに出力
	//--------------------------------------------------------------
	{
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <port>
		if( !ArgvIs( ARGV_PORT )) ErrorMes();
		WORD port = argv.Val;
		Shift();
		
		// <data>
		if( !ArgvIs( ARGV_INT )) ErrorMes();
		BYTE data = argv.Val;
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->IomOut( port, data );
		
		ZCons::SPrint( Stringf( "OUT port [ %02XH ] <- %02X  (= %d | %+d | ", port, (BYTE)data, (BYTE)data, (int8_t)data ) );
		int i,j;
		for( i=0, j=0x80; i<8; i++, j>>=1 ) ZCons::SPrint( Stringf( "%d", (data & j) ? 1 : 0 ) );
		ZCons::SPrintc( "B )\n");
		
		break;
	}
		
	case MONITOR_LOADMEM:
	//--------------------------------------------------------------
	//  loadmem <filename> <start-addr> <end-addr>
	//  loadmem <filename> <start-addr> #<size>
	// ファイルからメモリにロード
	//--------------------------------------------------------------
	{
		std::fstream fs;
		int start,size;
		
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <filename>
		if( !ArgvIs( ARGV_STR ) ) ErrorMes();
		P6VPATH fname = P6VSTR2PATH( argv.Str );
		Shift();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
		start = argv.Val;
		Shift();
		
		// [<addr|#size>]
		if     ( ArgvIs( ARGV_SIZE ) ) size = (argv.Val > 0xffff) ? 0xffff : argv.Val;
		else if( ArgvIs( ARGV_ADDR ) ) size = (argv.Val < start ) ? (0x10000 | argv.Val) - start + 1 : argv.Val - start + 1;
		else                           ErrorMes();
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		if( !OSD_FSopen( fs, fname, std::ios_base::in|std::ios_base::binary ) ){
			ZCons::SetColor( FC_RED );
			ZCons::SPrintc( "Failed : File open error\n" );
			ZCons::SetColor( FC_WHITE );
			break;
		}
		
		int addr = start;
		for( int i=0; i<size; i++ )
			vm->MemWrite( (addr++)&0xffff, FSGETBYTE( fs ) );
		fs.close();
		
		ZCons::SPrintc( Stringf( " Loaded [%s] -> %d bytes\n", P6VPATH2STR( fname ).c_str(), addr-start ) );
		break;
	}
		
	case MONITOR_SAVEMEM:
	//--------------------------------------------------------------
	//  savemem <filename> <start-addr> <end-addr>
	//  savemem <filename> <start-addr> #<size>
	//  メモリをファイルにセーブ
	//--------------------------------------------------------------
	{
		std::fstream fs;
		int start,size;
		
		if( argv.Type == ARGV_END ) ErrorMes();
		
		// <filename>
		if( !ArgvIs( ARGV_STR ) ) ErrorMes();
		P6VPATH fname = P6VSTR2PATH( argv.Str );
		Shift();
		
		// <addr>
		if( !ArgvIs( ARGV_ADDR ) ) ErrorMes();
		start = argv.Val;
		Shift();
		
		// [<addr|#size>]
		if     ( ArgvIs( ARGV_SIZE ) ) size = (argv.Val > 0xffff) ? 0xffff : argv.Val;
		else if( ArgvIs( ARGV_ADDR ) ) size = (argv.Val < start ) ? (0x10000 | argv.Val) - start + 1 : argv.Val - start + 1;
		else                           ErrorMes();
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		if( !OSD_FSopen( fs, fname, std::ios_base::out|std::ios_base::binary|std::ios_base::trunc ) ){
			ZCons::SetColor( FC_RED );
			ZCons::SPrintc( "Failed : File open error\n" );
			ZCons::SetColor( FC_WHITE );
			break;
		}
		
		int addr = start;
		for( int i=0; i<size; i++ )
			FSPUTBYTE( vm->MemRead( (addr++)&0xffff ), fs );
		fs.close();
		
		ZCons::SPrintc( Stringf( " Saved [%s] -> %d bytes\n", P6VPATH2STR( fname ).c_str(), addr-start ) );
		break;
	}
		
	case MONITOR_RESET:
	//--------------------------------------------------------------
	//  reset
	//	リセット
	//--------------------------------------------------------------
		if( argv.Type != ARGV_END ) ErrorMes();
		vm->Reset();
		
		break;
		
	case MONITOR_REG:
	//--------------------------------------------------------------
	//  reg <name> <value>
	//  レジスタの内容を変更
	//--------------------------------------------------------------
	{	int re = -1, val=0;
		std::string str = "XX";
		cZ80::Register reg;
		
		if( argv.Type == ARGV_END ) ErrorMes();
		
		if( !ArgvIs( ARGV_REG ) ) ErrorMes();		// <name>
		re = argv.Val;
		Shift();
		if( !ArgvIs( ARGV_INT ) ) ErrorMes();		// <value>
		val = argv.Val;
		Shift();
		
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->CpumGetRegister( &reg );
		
		switch( re ){
		case ARG_AF:	reg.AF.W  = val;		break;
		case ARG_BC:	reg.BC.W  = val;		break;
		case ARG_DE:	reg.DE.W  = val;		break;
		case ARG_HL:	reg.HL.W  = val;		break;
		case ARG_IX:	reg.IX.W  = val;		break;
		case ARG_IY:	reg.IY.W  = val;		break;
		case ARG_SP:	reg.SP.W  = val;		break;
		case ARG_PC:	reg.PC.W  = val;		break;
		case ARG_AF1:	reg.AF1.W = val;		break;
		case ARG_BC1:	reg.BC1.W = val;		break;
		case ARG_DE1:	reg.DE1.W = val;		break;
		case ARG_HL1:	reg.HL1.W = val;		break;
		case ARG_I:		val &= 0xff; reg.I = val;		break;
		case ARG_R:		val &= 0xff; reg.R = val;		break;
		case ARG_IFF:	if(val)   val=1; reg.IFF  = val;	break;
		case ARG_IM:	if(val>3) val=2; reg.IM   = val;	break;
		case ARG_HALT:	if(val)   val=1; reg.Halt = val;	break;
		}
		
		vm->CpumSetRegister( &reg );
		
		for( auto &m : MonitorArgv ){
			if( re == m.Val ){
				str = m.Str;
				break;
			}
		}
		ZCons::SPrintc( Stringf( "reg %s <- %04X\n", str.c_str(), val ) );
		
		break;
	}
		
	case MONITOR_DISASM:
	//--------------------------------------------------------------
	//  disasm [[<start-addr>][#<steps>]]
	//  逆アセンブル
	//--------------------------------------------------------------
	{	static int SaveDisasmAddr = -1;
		int i, pc;
		int addr = SaveDisasmAddr;
		int step = 16;
		std::string DisCode;
		cZ80::Register reg;
		
		if( argv.Type != ARGV_END ){
			if( ArgvIs( ARGV_ADDR )){		// [<addr>]
				addr = argv.Val;
				Shift();
			}
			if( ArgvIs( ARGV_SIZE )){		// [#<step>]
				step = argv.Val;
				Shift();
			}
		}
		if( argv.Type != ARGV_END ) ErrorMes();
		
		vm->CpumGetRegister( &reg );
		if( addr == -1 ) addr = reg.PC.W;	// ADDR 未指定時
		
		pc = 0;
		for( i=0; i<step; i++ ){
			pc += vm->CpumDisasm( DisCode, (WORD)(addr+pc) );
			ZCons::SPrintc( Stringf( "%s\n", DisCode.c_str() ) );
		}
		SaveDisasmAddr = ( addr + pc ) & 0xffff;
		break;
	}
	
	}
}


////////////////////////////////////////////////////////////////
// ヘルプ表示
////////////////////////////////////////////////////////////////
void cWndMon::Help( int cmd )
{
	switch( cmd ){
	case MONITOR_HELP:
		ZCons::SPrintc(
			"  help [<cmd>]\n"
			"    ヘルプを表示します\n"
			"    <cmd> ... ヘルプを表示したいコマンド\n"
			"              [omit]... 全コマンドの簡易ヘルプを表示\n"
		);
		break;
		
	case MONITOR_GO:
		ZCons::SPrintc(
			"  go\n"
			"    プログラムを実行します\n"
		);
		break;
		
	case MONITOR_TRACE:
		ZCons::SPrintc(
			"  trace [#<steps>|<steps>]\n"
			"    execute program specityes times\n"
			"    [all omit]        ... trace some steps (previous steps)\n"
			"    #<steps>, <steps> ... step counts of trace  ( you can omit '#' )\n"
		);
		break;
		
	case MONITOR_STEP:
		ZCons::SPrintc(
			"  step [call][jp][rep]\n"
			"    execute program 1 time\n"
			"    [all omit] ... execute 1 step\n"
			"    call       ... not trace CALL instruction\n"
			"    jp         ... not trace DJNZ instruction\n"
			"    rep        ... not trace LD*R/CP*R/IN*R/OT*R instruction\n"
			"    CAUTION)\n"
			"         call/jp/rep are use break-point #10.\n"
		);
		break;
		
	case MONITOR_STEPALL:
		ZCons::SPrintc(
			"  S\n"
			"    'step all' と同じ   (stepを参照)\n"
		);
		break;
		
	case MONITOR_BREAK:
		ZCons::SPrintc(
			"  break [<action>] <addr|port>\n"
			"  break CLEAR [#<No>]\n"
			"  break\n"
			"    ブレークポイントを設定します\n"
			"    [all omit]  ... 全てのブレークポイントを表示\n"
			"    <action>    ... set action of conditon PC|READ|WRITE|IN|OUT or CLEAR\n"
			"                    PC    ... break if PC reach addr\n"
			"                    READ  ... break if data is read\n"
			"                    WRITE ... break if data is written\n"
			"                    IN    ... break if data is input\n"
			"                    OUT   ... break if data is output\n"
			"                    CLEAR ... clear all break point\n"
			"                    [omit]... select PC\n"
			"    <addr|port> ... specify address or port\n"
			"                    if <action> is CLEAR, this argument is invalid\n"
			"    #<No>       ... number of break point.\n"
			"                    [omit]... select #1\n"
		);
		break;
		
	case MONITOR_READ:
		ZCons::SPrintc(
			"  read <addr>\n"
			"    メモリを読込みます\n"
			"    <addr> ... 指定アドレス\n"
		);
		break;
		
	case MONITOR_WRITE:
		ZCons::SPrintc(
			"  write <addr> <data>\n"
			"    メモリに書込みます\n"
			"    <addr> ... 指定アドレス\n"
			"    <data> ... 書込むデータ\n"
		);
		break;
		
	case MONITOR_FILL:
		ZCons::SPrintc(
			"  fill <start-addr> <end-addr> <value>\n"
			"  fill <start-addr> #<size>    <value>\n"
			"    メモリを指定値で埋めます\n"
			"    <start-addr> ... 開始アドレス\n"
			"    <end-addr>   ... 終了アドレス\n"
			"    #<size>      ... サイズ\n"
			"    <value>      ... 書込む値\n"
		);
		break;
		
	case MONITOR_MOVE:
		ZCons::SPrintc(
			"  move <src-addr> <end-addr> <dist-addr>\n"
			"  move <src-addr> #<size>    <dist-addr>\n"
			"    メモリを転送します\n"
			"    <src-addr>  ... 転送元開始アドレス\n"
			"    <end-addr>  ... 転送元終了アドレス\n"
			"    #<size>     ... 転送サイズ\n"
			"    <dist-addr> ... 転送先アドレス\n"
		);
		break;
		
	case MONITOR_SEARCH:
		ZCons::SPrintc(
			"  search [<value> [<start-addr> <end-addr>]]\n"
			"  search [<value> [<start-addr> #<size>]]\n"
			"    メモリを検索します\n"
			"    <value>      ... 検索値\n"
			"    <start-addr> ... 検索開始アドレス\n"
			"    <end-addr>   ... 検索終了アドレス\n"
			"    #<size>      ... 検索サイズ\n"
			"    [omit-all]   ... 前回の値または文字列を検索\n"
		);
		break;
		
	case MONITOR_OUT:
		ZCons::SPrintc(
			"  out <port> <data>\n"
			"    I/Oポートに出力します\n"
			"    <port> ... I/Oポートアドレス\n"
			"    <data> ... 出力データ\n"
		);
		break;
		
	case MONITOR_LOADMEM:
		ZCons::SPrintc(
			"  loadmem <filename> <start-addr> <end-addr>\n"
			"  loadmem <filename> <start-addr> #<size>\n"
			"    ファイルからメモリにロードします\n"
			"    <filename>   ... ファイル名\n"
			"    <start-addr> ... ロード開始アドレス\n"
			"    <end-addr>   ... ロード終了アドレス\n"
			"    #<size>      ... ロードサイズ\n"
		);
		break;
		
	case MONITOR_SAVEMEM:
		ZCons::SPrintc(
			"  savemem <filename> <start-addr> <end-addr>\n"
			"  savemem <filename> <start-addr> #<size>\n"
			"    メモリイメージをファイルにセーブします\n"
			"    <filename>   ... ファイル名\n"
			"    <start-addr> ... セーブ開始アドレス\n"
			"    <end-addr>   ... セーブ終了アドレス\n"
			"    #<size>      ... セーブサイズ\n"
		);
		break;
		
	case MONITOR_RESET:
		ZCons::SPrintc(
			"  reset\n"
			"    PC6001Vをリセットし，アドレス 0000H から実行します\n"
		);
		break;
		
	case MONITOR_REG:
		ZCons::SPrintc(
			"  reg <name> <value>\n"
			"    レジスタの値を参照，設定します\n"
			"    <name>     ... specity register name.\n"
			"                   AF|BC|DE|HL|AF'|BC'|DE'|HL'|IX|IY|SP|PC|I|R|IFF|IM\n"
			"    <value>    ... set value\n"
		);
		break;
		
	case MONITOR_DISASM:
		ZCons::SPrintc(
			"  disasm [[<start-addr>][#<steps>]]\n"
			"    逆アセンブルします\n"
			"    [all omit]   ... PCレジスタアドレスから16ステップ分を逆アセンブル\n"
			"    <start-addr> ... start-addr から逆アセンブルします\n"
			"                     [omit]... PCレジスタアドレス\n"
			"    #<steps>     ... 逆アセンブルするステップ数\n"
			"                     [omit]... 16ステップ\n"
		);
		break;
		
	}
}

#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
