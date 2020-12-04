#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "typedef.h"
#include "device.h"
#include "ini.h"


// 各種フラグ
#define MCRCCHK		0b00010000	// CRCチェック有効
#define MUSEEXRAM	0b00100000	// 拡張RAM使う
#define MUSESOL		0b00001111	// 戦士のカートリッジ バージョン(bit0-3)




////////////////////////////////////////////////////////////////

// ROM情報構造体
struct ROMINFO {
	const std::string FileName;	// ファイル名
	const DWORD Size;			// サイズ
	const DWORD Crc;			// CRC32
};


////////////////////////////////////////////////////////////////

// ROMセット情報取得
const std::vector<std::vector<ROMINFO>>& GetRomSetList( const int );

////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////

// メモリセルクラス(最小単位)
class MemCell {
public:
	enum {
		PAGEBITS = 13,			// 8KB
		PAGESIZE = 1 << PAGEBITS,
		PAGEMASK = PAGESIZE - 1
	};

protected:
	std::vector<BYTE> Data;		// データ
	bool WPt;					// ライトプロテクトフラグ

public:
	MemCell( BYTE = 0xff, bool = false );
	~MemCell();
	
	void Resize( size_t, BYTE = 0xff );				// リサイズ
	void SetData( std::fstream& );					// ROMデータをファイルから読込み
	size_t Size() const;							// サイズ取得
	
	BYTE Read( WORD ) const;						// メモリリード
	void Write( WORD, BYTE );						// メモリライト
};


// メモリセル集合体クラス(ROM/RAMチップ相当)
class MemCells {
protected:
	std::vector<MemCell> Cells;	// データ

public:
	MemCells( size_t = 1, BYTE = 0xff, bool = false );
	~MemCells();
	
	void Resize( size_t, BYTE = 0xff );				// リサイズ
	bool SetData( const P6VPATH& );					// ROMデータをファイルから読込み
	size_t Size() const;							// サイズ(メモリセル数)取得
	
	BYTE Read( WORD ) const;						// メモリリード
	void Write( WORD, BYTE );						// メモリライト
	
	MemCell& operator()( const int );				// メモリセル取得 - operator ()
};


// メモリブロッククラス
class MemBlock {
public:
	enum {
		PAGEBITS = 13,			// 8KB
		PAGESIZE = 1 << PAGEBITS,
		PAGEMASK = PAGESIZE - 1
	};
	
	using RFuncPtr = IDevice::RFuncPtr;
	using WFuncPtr = IDevice::WFuncPtr;
	using RFunc    = IDevice::RFunc;
	using WFunc    = IDevice::WFunc;

protected:
	std::string Name;			// メモリブロック名
	MemCell* PMem;				// メモリポインタ
	RFunc FRead;				// 関数ポインタ(読込み)
	WFunc FWrite;				// 関数ポインタ(書込み)
	int Wait;					// アクセスウェイト

public:
	MemBlock();
	~MemBlock();
	
	void SetMemory( const std::string&, MemCell&, int = -1 );		// メモリ割当て
	void SetFunc  ( const std::string&, RFunc, WFunc, int = -1 );	// 関数割当て
	const std::string& GetName() const;						// メモリブロック名取得
	void SetWait( int );									// アクセスウェイト設定
	int GetWait() const;									// アクセスウェイト取得
	
	BYTE Read( WORD, int* = nullptr ) const;				// メモリリード
	void Write( WORD, BYTE, int* = nullptr ) const;			// メモリライト
};


class MEM6 : public Device, public IDoko {
protected:
	// メモリ情報構造体
	struct MEMINFO {
		const std::vector<ROMINFO>& Rinfo;	// ROM情報へのポインタ
		const DWORD Size;					// サイズ
		const BYTE Init;					// 初期化データ
		const int Wait;						// アクセスウェイト
	};
	
	// メモリ情報
	static const MEMINFO IEMPTROM;
	static const MEMINFO IEMPTRAM;
	static const MEMINFO IEXTROM16;
	static const MEMINFO IEXTROM128;
	static const MEMINFO IEXTROM512;
	static const MEMINFO IEXTRAM16;
	static const MEMINFO IEXTRAM64;
	static const MEMINFO IEXTRAM128;
	
	// メモリ情報テーブル構造体
	struct MEMINFOTABLE {
		const MEMINFO* ExtRom;
		const MEMINFO* IntRam;
		const MEMINFO* ExtRam;
		
		const MEMINFO* System1;
		const MEMINFO* System2;
		const MEMINFO* CGRom1;
		const MEMINFO* CGRom2;
		const MEMINFO* Kanji;
		const MEMINFO* Voice;
		
		MEMINFOTABLE() :
			ExtRom( &IEMPTROM ), IntRam( &IEMPTRAM ), ExtRam( &IEMPTROM ),
			System1( &IEMPTROM ), System2( &IEMPTROM ), CGRom1( &IEMPTROM ), CGRom2( &IEMPTROM ),
			Kanji( &IEMPTROM ), Voice( &IEMPTROM ) {}
	};
	
	MEMINFOTABLE MemTable;		// メモリ情報テーブル
	
	bool CGBank;				// CG ROM BANK	true:有効 false:無効
	bool UseExtRom;				// 拡張ROM		true:有効 false:無効
	bool UseExtRam;				// 拡張RAM		true:有効 false:無効
	
	MemCells SysRom1;			// BASIC ROM	ALL (64,68の時はSystem ROM1)
	MemCells SysRom2;			// System ROM2	64,68
	MemCells ExtRom;			// 拡張 ROM		ALL
	MemCells CGRom1;			// CG ROM1		ALL
	MemCells CGRom2;			// CG ROM2		62,66
	MemCells KanjiRom;			// 漢字 ROM		62,66
	MemCells VoiceRom;			// 音声合成 ROM	62,66
	
	MemCells IntRam;			// 内部 RAM		ALL
	MemCells ExtRam;			// 外部 RAM		ALL
	
	std::vector<MemBlock> RomB;		// ROMブロック
	std::vector<MemBlock> InRamB;	// RAMブロック(内部)
	std::vector<MemBlock> ExRamB;	// RAMブロック(外部)
	MemBlock InExRamB;			// RAMブロック(内部/外部RAM書込み)
	
	std::array<MemBlock*,8> RD_Blk;	// リード時メモリブロックポインタ(8KB*8)
	std::array<MemBlock*,8> WR_Blk;	// ライト時メモリブロックポインタ(8KB*8)
	
	P6VPATH FilePath;			// 拡張ROMファイルフルパス
	int M1Wait;					// M1ウェイト
	bool EnableChkCRC;			// CRCチェック  true:有効 false:無効
	
	// for 62,66,64,68 -----------------------------------------------------------------------
	bool cgrom;					// CG ROM 選択用 true:N60モード false:N60mモード
	bool kj_rom;				// 漢字ROM,音声合成ROM 選択用 true:漢字ROM false:音声合成ROM
	bool kj_LR;					// 漢字ROM 左右選択用 true:右 false:左
	bool cgenable;				// CG ROMアクセスフラグ true:アクセス可 false:アクセス不可
	BYTE cgaden;				// CG ROMアドレスイネーブル
	BYTE cgaddr;				// CG ROMアドレス A13,14,15
	std::array<BYTE,3> Rf;		// メモリコントローラ内部レジスタ
	BYTE c2acc;					// PortC2Hアクセスフラグ
	// ---------------------------------------------------------------------------------------
	
	// for 64,68 -----------------------------------------------------------------------------
	std::array<MemBlock*,8> RD_BlkSR;	// リード時メモリブロックポインタ(8KB*8)	SRモード用
	std::array<MemBlock*,8> WR_BlkSR;	// ライト時メモリブロックポインタ(8KB*8)	SRモード用
	std::array<BYTE,16> RfSR;			// メモリコントローラ内部レジスタ			SRモード用
	// ---------------------------------------------------------------------------------------
	
	DWORD CalcCrc32( MemCells&, int );		// CRC32計算
	bool AllocMemory( MemCells&, const MEMINFO*, const P6VPATH& );	// メモリ確保とROMファイル読込み
	virtual bool AllocMemorySpecific( const P6VPATH& ) = 0;			// 全メモリ確保とROMファイル読込み(機種別)
	virtual void SetRamValue() = 0;			// RAMの初期値を設定
	virtual bool InitSpecific() = 0;		// 初期化(機種別)
	virtual void SetMemBlockR( BYTE, BYTE ) = 0;	// メモリリード時のメモリブロック指定(62,66)
	virtual void SetMemBlockW( BYTE ) = 0;	// メモリライト時のメモリブロック指定(62,66)
	
	// for 62,66,64,68 -----------------------------------------------------------------------
	void SetWait( BYTE );					// メモリアクセスウェイト設定
	BYTE GetWait() const;					// メモリアクセスウェイト取得
	void SetCGrom( BYTE );					// CG ROM アドレス等設定(62,66)
	void SelectCGrom( int );				// CG ROM 選択(62,66)
	void SetKanjiRom( BYTE );				// 漢字ROMおよび音声合成ROM設定(62,66)
	BYTE GetKanjiRom() const;				// 漢字ROMおよび音声合成ROM取得(62,66)
	void SetPortC2HAccess( BYTE );			// PortC2Hアクセス設定(62,66)
	// ---------------------------------------------------------------------------------------
	
	// 戦士のカートリッジ --------------------------------------------------------------------
	int SolVer;					// バージョン 0:なし 1:無印 2:mkⅡ 3:mkⅢ
	bool Sol60Mode;				// 初代機モード　true:有効 false:無効
	std::array<BYTE,8> SolBank;	// メモリバンクレジスタ
	int SolBankSet;				// ROMバンクセット
	
	void SetSolBank( BYTE, BYTE );			// メモリバンクレジスタ設定
	
	// メモリブロック用関数 ------------------------------------------------------------------
	BYTE SolReadEx( MemCell*, WORD );		// 戦士のカートリッジ読込み(拡張ROM領域)
	BYTE SolSccRead( MemCell*, WORD );		// 戦士のカートリッジSCC読込み
	void SolSccWrite( MemCell*, WORD, BYTE );	// 戦士のカートリッジSCC書込み
	// ---------------------------------------------------------------------------------------
	
	// I/Oアクセス関数 -----------------------------------------------------------------------
	virtual void Out06H( int, BYTE );
	void Out3xH( int, BYTE );
	void Out7FH( int, BYTE );
	
	// for 62,66,64,68 -----------------------------------------------------------------------
	void OutC1H( int, BYTE );
	void OutC2H( int, BYTE );
	void OutC3H( int, BYTE );
	void OutF0H( int, BYTE );
	void OutF1H( int, BYTE );
	void OutF2H( int, BYTE );
	void OutF3H( int, BYTE );
	void OutF8H( int, BYTE );
	BYTE InC2H( int );
	BYTE InF0H( int );
	BYTE InF1H( int );
	BYTE InF2H( int );
	BYTE InF3H( int );
	// ---------------------------------------------------------------------------------------

public:
	MEM6( VM6*, const ID& );
	virtual ~MEM6();
	
	bool AllocAllMemory( const P6VPATH& , BYTE );		// 全メモリ確保とROMファイル読込み
	bool Init();										// 初期化
	virtual void Reset();								// リセット
	
	BYTE Fetch( WORD, int* = nullptr ) const;			// フェッチ(M1)
	BYTE Read( WORD, int* = nullptr ) const;			// メモリリード
	void Write( WORD, BYTE, int* = nullptr );			// メモリライト
	
	bool MountExtRom( const P6VPATH& );					// 拡張ROM マウント
	void UnmountExtRom();								// 拡張ROM アンマウント
	const P6VPATH& GetFile() const;						// 拡張ROMファイルパス取得
	
	// 8255入出力関連関数
	virtual void SetCGBank( bool );						// CG ROM BANK を切り替える
	
	// 直接アクセス関数
	BYTE ReadMainRom( WORD ) const;
	BYTE ReadIntRam ( WORD ) const;
	BYTE ReadExtRom ( WORD ) const;
	BYTE ReadExtRam ( WORD ) const;
	virtual BYTE ReadCGrom1( WORD ) const;
	virtual BYTE ReadCGrom2( WORD ) const;
	virtual BYTE ReadCGrom3( WORD ) const;
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	const std::string& GetReadMemBlk( int ) const ;
	const std::string& GetWriteMemBlk( int ) const ;
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	// デバイスID
	enum IDOut{ out06H=0, out3xH, out7FH };
	enum IDIn {};
	
	// ---------------------------------------------------------
	bool DokoSave( cIni* ) override;	// どこでもSAVE
	bool DokoLoad( cIni* ) override;	// どこでもLOAD
	// ---------------------------------------------------------
};


class MEM60 : public MEM6 {
private:
	// メモリ情報
	static const MEMINFO ISYSROM1;
	static const MEMINFO ICGROM0;
	static const MEMINFO ICGROM1;
	static const MEMINFO IINTRAM;
	
	bool AllocMemorySpecific( const P6VPATH& ) override;	// 全メモリ確保とROMファイル読込み(機種別)
	void SetRamValue() override;						// RAMの初期値を設定
	bool InitSpecific() override;						// 初期化(機種別)
	void SetMemBlockR( BYTE, BYTE ) override;			// メモリリード時のメモリブロック指定
	void SetMemBlockW( BYTE ) override;					// メモリライト時のメモリブロック指定
	
	// メモリブロック用関数 ------------------------------------------------------------------
	BYTE CGromRead( MemCell*, WORD );		// PC-6001 CGROM読込み
	// ---------------------------------------------------------------------------------------
	
	// I/Oアクセス関数
	void Out06H( int, BYTE ) override;

public:
	MEM60( VM6*, const ID& );
	~MEM60();
	
	// デバイスID
	enum IDOut{ out06H=0, out3xH, out7FH, outF0H, outF2H };
	enum IDIn {};
};


class MEM61 : public MEM60 {
private:
	// メモリ情報
	static const MEMINFO ISYSROM1;
	static const MEMINFO ICGROM0;
	static const MEMINFO ICGROM1;
	static const MEMINFO IINTRAM;

public:
	MEM61( VM6*, const ID& );
	~MEM61();
};


class MEM62 : public MEM6 {
protected:
	// メモリ情報
	static const MEMINFO ISYSROM1;
	static const MEMINFO ICGROM1;
	static const MEMINFO ICGROM2;
	static const MEMINFO IKANJI;
	static const MEMINFO IVOICE;
	static const MEMINFO IINTRAM;
	
	virtual bool AllocMemorySpecific( const P6VPATH& ) override;	// 全メモリ確保とROMファイル読込み(機種別)
	virtual void SetRamValue() override;					// RAMの初期値を設定
	virtual bool InitSpecific() override;					// 初期化(機種別)
	virtual void SetMemBlockR( BYTE, BYTE ) override;		// メモリリード時のメモリブロック指定
	void SetMemBlockW( BYTE );								// メモリライト時のメモリブロック指定
	
	// メモリブロック用関数 ------------------------------------------------------------------
	void IERamWrite( MemCell*, WORD, BYTE );	// PC-6001mk2以降 内部/外部RAM書込み
	// ---------------------------------------------------------------------------------------

public:
	MEM62( VM6*, const ID& );
	virtual ~MEM62();
	
	// 8255入出力関連関数
	void SetCGBank( bool ) override;		// CG ROM BANK を切り替える
	
	// 直接アクセス関数
	virtual BYTE ReadCGrom2( WORD ) const override;
	virtual BYTE ReadKanjiRom( WORD ) const;
	virtual BYTE ReadVoiceRom( WORD ) const;
	
	// デバイスID
	enum IDOut{ out06H=0, out3xH, out7FH, outC1H, outC2H, outC3H, outF0H,  outF1H, outF2H, outF3H, outF8H };
	enum IDIn {                                    inC2H=0,        inF0H,   inF1H,  inF2H,  inF3H         };
};


class MEM66 : public MEM62 {
private:
	// メモリ情報
	static const MEMINFO ISYSROM1;
	static const MEMINFO ICGROM1;
	static const MEMINFO ICGROM2;
	static const MEMINFO IKANJI;
	static const MEMINFO IVOICE;
	static const MEMINFO IINTRAM;

public:
	MEM66( VM6*, const ID& );
	~MEM66();
};


class MEM64 : public MEM62 {
protected:
	// メモリ情報
	static const MEMINFO ISYSROM1;
	static const MEMINFO ISYSROM2;
	static const MEMINFO ICGROM1;
	static const MEMINFO IINTRAM;
	
	bool AllocMemorySpecific( const P6VPATH& ) override;	// 全メモリ確保とROMファイル読込み(機種別)
	virtual void SetRamValue() override;			// RAMの初期値を設定
	bool InitSpecific() override;					// 初期化(機種別)
	void SetMemBlockR( BYTE, BYTE ) override;		// メモリリード時のメモリブロック指定
	void SetMemBlockSR( BYTE, BYTE );				// メモリリード/ライト時のメモリブロック指定(64,68)
	
	// I/Oアクセス関数
	void Out6xH( int, BYTE );
	void OutC8H( int, BYTE );
	
	BYTE In6xH( int );
	virtual BYTE InB2H( int );

public:
	MEM64( VM6*, const ID& );
	virtual ~MEM64();
	
	void Reset() override;					// リセット
	
	// 直接アクセス関数
	BYTE ReadCGrom1( WORD ) const override;
	BYTE ReadCGrom2( WORD ) const override;
	BYTE ReadCGrom3( WORD ) const override;
	BYTE ReadKanjiRom( WORD ) const override;
	BYTE ReadVoiceRom( WORD ) const override;
	
	// デバイスID
	enum IDOut{ out06H=0, out3xH, out7FH, out6xH,   outC1H, outC2H, outC3H, outF0H, outF1H, outF2H, outF3H, outF8H };
	enum IDIn {                            in6xH=0,          inC2H,          inF0H,  inF1H,  inF2H,  inF3H,
				 inB2H };
	
	// ---------------------------------------------------------
	bool DokoSave( cIni* ) override;	// どこでもSAVE
	bool DokoLoad( cIni* ) override;	// どこでもLOAD
	// ---------------------------------------------------------
};


class MEM68 : public MEM64 {
protected:
	void SetRamValue() override;			// RAMの初期値を設定
	
	// I/Oアクセス関数
	BYTE InB2H( int ) override;

public:
	MEM68( VM6*, const ID& );
	virtual ~MEM68();
};

#endif	// MEMORY_H_INCLUDED
