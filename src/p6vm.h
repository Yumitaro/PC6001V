#ifndef P6VM_H_INCLUDED
#define P6VM_H_INCLUDED

#include <memory>
#include <vector>

#include "cpum.h"
#include "cpus.h"
#include "io.h"
#include "keyboard.h"
#include "pio.h"
#include "psgfm.h"
#include "schedule.h"
#include "tape.h"
#include "typedef.h"

#include "device/z80.h"

#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#include "breakpoint.h"
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

class EL6;
class CFG6;

class IO6;
class MEM6;
class IRQ6;
class VDG6;
class PSGb;
class VCE6;
class DSK6;


// 基本仮想マシンクラス
//class VM6 {
class VM6 : public EVSC, public CPU6, public PIO6, public CMTL, public CMTS,
			virtual public SUB6, virtual public KEY6
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	, public BPoint
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
{
	
	friend class EL6;
	friend class DSP6;
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	friend class cWndMon;
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
protected:
	// デバイスコネクタテーブル構造体
	struct DEVCONNTABLE {
		const std::vector<IOBus::Connector>* Intr;		// 割込み
		const std::vector<IOBus::Connector>* Memory;	// メモリ
		const std::vector<IOBus::Connector>* Vdg;		// VDG
		const std::vector<IOBus::Connector>* Psg;		// PSG/OPN
		const std::vector<IOBus::Connector>* M8255;		// I/O(Z80側)
		const std::vector<IOBus::Connector>* S8255;		// I/O(SUB CPU側)
		const std::vector<IOBus::Connector>* Voice;		// 音声合成
		const std::vector<IOBus::Connector>* Disk;		// DISK
		const std::vector<IOBus::Connector>* CmtL;		// CMT(LOAD)
		const std::vector<IOBus::Connector>* Soldier;	// 戦士のカートリッジ
		
		DEVCONNTABLE() : Intr(nullptr), Memory(nullptr), Vdg(nullptr), Psg(nullptr), M8255(nullptr), S8255(nullptr),
						 Voice(nullptr), Disk(nullptr), CmtL(nullptr), Soldier(nullptr) {}
	};
	
	int cclock;					// CPUクロック
	int pclock;					// PSG/OPNクロック
	
	// オブジェクトポインタ
	EL6* el;					// エミュレータレイヤ
	IO6* iom;					// I/O(Z80側)
	IO6* ios;					// I/O(SUB CPU側)
	IRQ6* intr;					// 割込み
	MEM6* mem;					// メモリ
	VDG6* vdg;					// VDG
	PSGb* psg;					// PSG/OPN
	VCE6* voice;				// 音声合成
	DSK6* disk;					// DISK
	
	DEVCONNTABLE DevTable;		// デバイスコネクタテーブル
	
	virtual void AllocObjSpecific() = 0;				// 機種別オブジェクト確保
	bool AllocObject( CFG6* );							// 全オブジェクト確保
	void DeleteAllObject();								// 全オブジェクト削除
	
public:
	VM6( EL6* );										// コンストラクタ
	virtual ~VM6();										// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_soldier;	// 戦士のカートリッジ
	
	bool Init( CFG6* );									// 初期化
	void Reset();										// リセット
	int Emu();											// 1命令実行
	int GetCPUClock() const;							// CPUクロック取得
	
	
	// P6デバイス用関数群
	// EL
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	bool ElIsMonitor() const;							// モニタモード?
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// IO6
	BYTE IomIn( int, int* = nullptr );					// IN関数
	BYTE IosIn( int, int* = nullptr );					// IN関数
	void IomOut( int, BYTE, int* = nullptr );			// OUT関数
	void IosOut( int, BYTE, int* = nullptr );			// OUT関数
	// IRQ6
	int	IntIntrCheck();									// 割込みチェック
	void IntReqIntr( DWORD );							// 割込み要求
	void IntCancelIntr( DWORD );						// 割込み撤回
	bool IntGetTimerIntr();								// タイマ割込みスイッチ取得
	// SUB6
	bool IsCmtIntrReady();								// CMT割込み発生可?
	// MEM6
	BYTE MemFetch( WORD, int* = nullptr );				// フェッチ(M1)
	BYTE MemRead( WORD, int* = nullptr );				// メモリリード
	void MemWrite( WORD, BYTE, int* = nullptr );		// メモリライト
	void MemSetCGBank( bool );							// CG ROM BANK を切り替える
	BYTE MemReadMainRom( WORD ) const;					// 直接読込み
	BYTE MemReadIntRam ( WORD ) const;					// 直接読込み
	BYTE MemReadExtRom ( WORD ) const;					// 直接読込み
	BYTE MemReadExtRam ( WORD ) const;					// 直接読込み
	BYTE MemReadCGrom1 ( WORD ) const;					// 直接読込み
	BYTE MemReadCGrom2 ( WORD ) const;					// 直接読込み
	BYTE MemReadCGrom3 ( WORD ) const;					// 直接読込み
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	const std::string& MemGetReadMemBlk( int ) const;	// メモリブロック取得(Read)
	const std::string& MemGetWriteMemBlk( int ) const;	// メモリブロック取得(Write)
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// VDG6
	bool VdgGetCrtDisp() const;							// CRT表示状態取得
	void VdgSetCrtDisp( bool );							// CRT表示状態設定
	bool VdgGetWinSize() const;							// ウィンドウサイズ取得
	bool VdgIsBusReqStop() const;						// バスリクエスト区間停止フラグ取得
	bool VdgIsBusReqExec() const;						// バスリクエスト区間実行フラグ取得
	WORD VdgGetVramAddr() const;						// VRAMアドレス取得
	WORD VdgGerAttrAddr() const;						// ATTRアドレス取得
	bool VdgIsSRmode() const;							// SRモード取得
	bool VdgIsSRBitmap( WORD ) const;					// SRビットマップモード取得
	WORD VdgSRGVramAddr( WORD ) const;					// SRのG-VRAMアドレス取得(ビットマップモード)
	// DSK6
	bool DskIsMount( int ) const;						// マウント済み?
	bool DskIsSystem( int ) const;						// システムディスク?
	bool DskIsProtect( int ) const;						// プロテクト?
	bool DskInAccess( int ) const;						// アクセス中?
	const std::filesystem::path& DskGetFile( int ) const;	// ファイルパス取得
	const std::string& DskGetName( int ) const;			// DISK名取得
};


// PC-6001 仮想マシンクラス
class VM60 : public VM6, public SUB60, public KEY60 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM60( EL6* );								// コンストラクタ
	~VM60();									// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_intr;		// 割込み
	const static std::vector<IOBus::Connector> c_vdg;		// VDG
	const static std::vector<IOBus::Connector> c_psg;		// PSG
	const static std::vector<IOBus::Connector> c_8255m;		// I/O(Z80側)
	const static std::vector<IOBus::Connector> c_8255s;		// I/O(SUB CPU側)
	const static std::vector<IOBus::Connector> c_disk;		// DISK
	const static std::vector<IOBus::Connector> c_cmtl;		// CMT(LOAD)
	const static std::vector<IOBus::Connector> c_soldier;	// 戦士のカートリッジ
};


// PC-6001A 仮想マシンクラス
class VM61 : public VM6, public SUB60, public KEY60 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM61( EL6* );								// コンストラクタ
	~VM61();									// デストラクタ
};


// PC-6001mk2 仮想マシンクラス
class VM62 : public VM6, public SUB62, public KEY62 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM62( EL6* );								// コンストラクタ
	~VM62();									// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_intr;		// 割込み
	const static std::vector<IOBus::Connector> c_memory;	// メモリ
	const static std::vector<IOBus::Connector> c_vdg;		// VDG
	const static std::vector<IOBus::Connector> c_psg;		// PSG
	const static std::vector<IOBus::Connector> c_8255m;		// I/O(Z80側)
	const static std::vector<IOBus::Connector> c_8255s;		// I/O(SUB CPU側)
	const static std::vector<IOBus::Connector> c_voice;		// 音声合成
	const static std::vector<IOBus::Connector> c_disk;		// DISK
	const static std::vector<IOBus::Connector> c_cmtl;		// CMT(LOAD)
};


// PC-6601 仮想マシンクラス
class VM66 : public VM6, public SUB62, public KEY62 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM66( EL6* );								// コンストラクタ
	~VM66();									// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_disk;	// DISK
};


// PC-6001mk2SR 仮想マシンクラス
class VM64 : public VM6, public SUB62, public KEY62 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM64( EL6* );								// コンストラクタ
	~VM64();									// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_intr;		// 割込み
	const static std::vector<IOBus::Connector> c_memory;	// メモリ
	const static std::vector<IOBus::Connector> c_vdg;		// VDG
	const static std::vector<IOBus::Connector> c_psg;		// OPN
	const static std::vector<IOBus::Connector> c_8255m;		// I/O(Z80側)
	const static std::vector<IOBus::Connector> c_8255s;		// I/O(SUB CPU側)
	const static std::vector<IOBus::Connector> c_voice;		// 音声合成
	const static std::vector<IOBus::Connector> c_disk;		// DISK
	const static std::vector<IOBus::Connector> c_cmtl;		// CMT(LOAD)
};


// PC-6601SR 仮想マシンクラス
class VM68 : public VM6, public SUB68, public KEY62 {
private:
	void AllocObjSpecific() override;			// 機種別オブジェクト確保
	
public:
	VM68( EL6* );								// コンストラクタ
	~VM68();									// デストラクタ
	
	// デバイスコネクタ
	const static std::vector<IOBus::Connector> c_disk;	// DISK
};


#endif		// P6VM_H_INCLUDED
