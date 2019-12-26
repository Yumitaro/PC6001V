// Based on SLibrary.
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class is produced by AST. Check homepage when you need any help.
// Mail Address.    ast@qt-space.com
// Official HP URL. http://ast.qt-space.com/

#include "../thread.h"
#include <windows.h>
#include <process.h>


////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////
cThread::cThread( void )
{
	this->m_bCancel			= true;
	this->m_hThread			= nullptr;
	this->m_BeginTheadParam	= nullptr;
}


////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////
cThread::~cThread( void )
{
	bool bWaiting = this->Waiting();
	if( bWaiting == false ){
		::TerminateThread( (HANDLE)this->m_hThread, 0 );
	}
}


////////////////////////////////////////////////////////////////
// スレッド開始
//
// 引数:	スレッドに渡すポインタ
// 返値:	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool cThread::BeginThread ( void* lpVoid )
{
	bool bSuccess = false;
	
	if( this->m_hThread == nullptr ){
		this->m_BeginTheadParam = lpVoid;
		this->m_bCancel			= false;
		
		HTHREAD hThread = (HTHREAD)::_beginthread( ThreadProc, 0, reinterpret_cast<void*>(this) );
		if( hThread != (HTHREAD)(unsigned int)-1 ){
			this->m_hThread = hThread;
			::SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
			bSuccess = true;
		}
	}
	
	return bSuccess;
}


////////////////////////////////////////////////////////////////
// スレッド終了を待つ
//
// 引数:	なし
// 返値:	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool cThread::Waiting( void )
{
	bool bSuccess = false;
	
	if( this->m_hThread != nullptr ){
		DWORD dwRet = WaitForSingleObject( (HANDLE)this->m_hThread, INFINITE );
		if( dwRet == WAIT_OBJECT_0 ){
			this->m_hThread = nullptr;
			bSuccess = true;
		}
	}else{
		bSuccess = true;
	}
	
	return bSuccess;
}


////////////////////////////////////////////////////////////////
// スレッド終了要求
////////////////////////////////////////////////////////////////
void cThread::Cancel( void )
{
	this->cMutex::lock();
	this->m_bCancel = true;
	this->cMutex::unlock();
}


////////////////////////////////////////////////////////////////
// スレッド終了要求された？
//
// 引数:	なし
// 返値:	true:された false:されない
////////////////////////////////////////////////////////////////
bool cThread::IsCancel( void )
{
	bool bCancel = false;
	
	this->cMutex::lock();
	bCancel = this->m_bCancel;
	this->cMutex::unlock();
	return bCancel;
}


////////////////////////////////////////////////////////////////
// デフォルトスレッド関数
////////////////////////////////////////////////////////////////
void cThread::ThreadProc( void* lpVoid )
{
	static thread_local cThread* lpThis;
	
	if( !lpThis ) lpThis = STATIC_CAST( cThread*, lpVoid );	// 自分自身のオブジェクトポインタ取得
	lpThis->OnThread( lpThis->m_BeginTheadParam );			// virtual Procedure 
	lpThis->m_hThread = nullptr;
	::_endthread();
}
