#ifndef BREAKPOINTL_H_INCLUDED
#define BREAKPOINTL_H_INCLUDED

#include <vector>

#include "typedef.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

class BPoint {
public:
	enum BPtype{ BP_NONE, BP_PC, BP_READ, BP_WRITE, BP_IN, BP_OUT, EndofBPtype };
	
private:
	struct BreakPoint {
		BPtype Type;
		WORD Addr;
		bool Break;
		
		BreakPoint() : Type(BP_NONE), Addr(0), Break(false) {}
	};
	std::vector<BreakPoint> BP;	// ブレークポイント
	
public:
	BPoint();
	~BPoint();
	
	void SetBP( const BPtype, WORD );			// ブレークポイントを設定
	void DeleteBP( const int );					// ブレークポイントを削除
	BPtype GetBPType( const int ) const;		// ブレークポイントのタイプを取得
	WORD GetBPAddr( const int ) const;			// ブレークポイントのアドレスを取得
	int GetBPNum() const;						// ブレークポイント登録数取得
	
	bool CheckBP( const BPtype, const WORD );	// ブレークポイントをチェック
	int GetReqBPNum() const;					// ブレーク要求のあったブレークポイントNo.を取得
	void ResetBP();								// ブレーク要求キャンセル
};

#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif	// BREAKPOINTL_H_INCLUDED
