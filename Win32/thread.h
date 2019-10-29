// Based on SLibrary.
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class is produced by AST. Check homepage when you need any help.
// Mail Address.    ast@qt-space.com
// Official HP URL. http://ast.qt-space.com/

#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include "typedef.h"
#include "semaphore.h"


class cThread : public cMutex {
private:
	bool m_bCancel;				// for Cancel().
	HTHREAD m_hThread;			// for Thread Handle.
	
	void* m_BeginTheadParam;
	static int ThreadProc( void* );			// デフォルトスレッド関数
	
protected:
	virtual void OnThread( void* ) = 0;		// Virtual func. You need overwrite.
	
public:
	cThread();								// Constructor
	virtual ~cThread();						// Destructor
	
	bool BeginThread( void* = nullptr );	// スレッド開始
	bool Waiting();							// スレッド終了を待つ
	
	void Cancel();							// スレッド終了要求
	bool IsCancel();						// スレッド終了要求された？
};

#endif // THREAD_H_INCLUDED

