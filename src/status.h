#ifndef STATUS_H_INCLUDED
#define STATUS_H_INCLUDED

#include <memory>

#include "console.h"
#include "p6el.h"


//------------------------------------------------------
//  ステータスバークラス
//------------------------------------------------------
class cWndStat : public ZCons {
private:
	int DrvNum;				// ドライブ数
	
public:
	cWndStat();
	~cWndStat();
	
	bool Init( int, int = -1 );			// 初期化
	void Update( EL6* );				// ウィンドウ更新
};

#endif	// STATUS_H_INCLUDED
