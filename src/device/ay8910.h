#ifndef AY8910_H_INCLUDED
#define AY8910_H_INCLUDED

#include "../typedef.h"
#include "../ini.h"
#include "psgbase.h"

#ifdef USEFMGEN
#include "fmgen/types.h"
#include "fmgen/psg.h"
#endif


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class cAY8910 : public cPSG {
protected:
	#ifdef USEFMGEN
	PSG	psg;
	#else
	BYTE Regs[16];
	int UpdateStep;
	int PeriodA, PeriodB, PeriodC, PeriodN, PeriodE;
	int CountA, CountB, CountC, CountN, CountE;
	int VolA, VolB, VolC, VolE;
	BYTE EnvelopeA, EnvelopeB, EnvelopeC;
	BYTE OutputA, OutputB, OutputC, OutputN;
	int8_t CountEnv;
	BYTE Hold,Alternate,Attack,Holding;
	int RNG;
	int VolTable[32];
	#endif
	
	void _WriteReg( BYTE, BYTE );			// レジスタ書込みサブ
	void WriteReg( BYTE, BYTE ) override;	// レジスタ書込み
	BYTE ReadReg() override;				// レジスタ読込み
	
	void SetClock( int, int ) override;		// クロック設定
	void SetVolumeTable( int ) override;	// 音量設定(ボリュームテーブル設定)
	
	bool InitMod( int, int ) override;		// 初期化
	void Reset() override;					// リセット
	
public:
	cAY8910();								// コンストラクタ
	virtual ~cAY8910();						// デストラクタ
	
	int Update1Sample() override;			// ストリーム1Sample更新
};


#endif	// AY8910_H_INCLUDED
