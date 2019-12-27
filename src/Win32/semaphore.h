// Based on SLibrary.
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class is produced by AST. Check homepage when you need any help.
// Mail Address.    ast@qt-space.com
// Official HP URL. http://ast.qt-space.com/

#ifndef SEMAPHORE_H_INCLUDED
#define SEMAPHORE_H_INCLUDED

#include "typedef.h"



////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
// Mutex クラス
class cMutex {
private:
	HCRSECT mcs;
	
public:
	cMutex();
	~cMutex();
	
	void lock();		// Lock
	void unlock();		// Unlock
};


// Semaphore クラス
class cSemaphore {
private:
	HSEMAPHORE sem;
	long count;
	
public:
	cSemaphore();		// コンストラクタ
	~cSemaphore();		// デストラクタ;
	
	int Post();			// セマフォ加算
	int Wait();			// セマフォ待つ
};


#endif // SEMAPHORE_H_INCLUDED
