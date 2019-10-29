// Based on SLibrary.
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class is produced by AST. Check homepage when you need any help.
// Mail Address.    ast@qt-space.com
// Official HP URL. http://ast.qt-space.com/

#ifndef SEMAPHORE_H_INCLUDED
#define SEMAPHORE_H_INCLUDED

#include <condition_variable>
#include <mutex>

#include "typedef.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
// Mutex クラス
class cMutex {
private:
	std::mutex mtx;
	
public:
	cMutex();						// コンストラクタ
	~cMutex();						// デストラクタ;
	
	void Lock();					// Lock
	void UnLock();					// Unlock
};


// Semaphore クラス
class cSemaphore {
private:
	std::mutex mtx;
	std::condition_variable cv;
	int count;
	
public:
	cSemaphore();					// コンストラクタ
	~cSemaphore();					// デストラクタ;
	
	void Post();					// セマフォ加算
	void Wait();					// セマフォ待つ
};


#endif // SEMAPHORE_H_INCLUDED
