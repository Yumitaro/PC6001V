#ifndef REPLAY_H_INCLUDED
#define REPLAY_H_INCLUDED

#include <filesystem>
#include <vector>

#include "typedef.h"
#include "ini.h"

#define	REP_IDLE	(0)
#define	REP_RECORD	(1)
#define	REP_REPLAY	(2)


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class REPLAY : public cIni {
protected:
	int RepST;					// ステータス
	DWORD RepFrm;				// フレームNo.カウンタ
	DWORD EndFrm;				// リプレイ終了フレーム
	
public:
	REPLAY();													// コンストラクタ
	~REPLAY();													// デストラクタ
	
	bool Init();												// 初期化
	
	int GetStatus() const;										// ステータス取得
	
	bool StartRecord( const std::filesystem::path& );			// リプレイ記録開始
	bool ResumeRecord( const std::filesystem::path&, int );		// リプレイ記録再開
	void StopRecord();											// リプレイ記録停止
	bool ReplayWriteFrame( const std::vector<BYTE>&, bool );	// リプレイ1フレーム書出し
	
	bool StartReplay( const std::filesystem::path& );			// リプレイ再生開始
	void StopReplay();											// リプレイ再生止
	bool ReplayReadFrame( std::vector<BYTE>& );					// リプレイ1フレーム読込み
	
};


#endif	// REPLAY_H_INCLUDED
