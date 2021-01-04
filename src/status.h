#ifndef STATUS_H_INCLUDED
#define STATUS_H_INCLUDED

#include <memory>

#include "console.h"
#include "p6vm.h"


//------------------------------------------------------
//  ステータスバークラス
//------------------------------------------------------
class cWndStat : public ZCons {
private:
	int 	DrvNum;				// ドライブ数
	DWORD	Indicator;			// リプレイステータス
	
public:
	cWndStat();
	~cWndStat();
	
	bool Init( int, int = -1 );					// 初期化
	void Update( const std::shared_ptr<VM6>& );	// ウィンドウ更新
	
	void SetIndicator( DWORD );					// インジケータセット
};

#endif	// STATUS_H_INCLUDED
