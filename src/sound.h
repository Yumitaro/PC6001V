#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include <queue>
#include "semaphore.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
// リングバッファオブジェクト
class cRing : public cMutex {
private:
	std::queue<int> Buffer;		// バッファ
	int Size;					// バッファサイズ(データ数)
	
public:
	cRing();								// コンストラクタ
	virtual ~cRing();						// デストラクタ
	
	bool InitBuffer( int );					// バッファ初期化
	
	virtual int Get();						// 読込み
	virtual void Put( int );				// 書込み
	
	int ReadySize();						// 未読データ数取得
	int FreeSize( bool = false );			// 残りバッファ取得
	int GetSize();							// バッファサイズ取得
};


// サウンドデバイスオブジェクト
class SndDev : public cRing {
protected:
	int SampleRate;				// サンプルレート
	int Volume;					// 音量
	int LPF_Mem;				// ローパスフィルタ用ワーク
	int LPF_fc;					// ローパスフィルタカットオフ周波数
	
	int LPF( int );							// ローパスフィルタ
	
public:
	SndDev();								// コンストラクタ
	virtual ~SndDev();						// デストラクタ
	
	virtual bool Init( int );				// 初期化
	
	int Get() override;						// 読込み
	
	void SetLPF( int );						// ローパスフィルタ カットオフ周波数設定
	virtual bool SetSampleRate( int, int );	// サンプリングレート設定
	virtual void SetVolume( int );			// 音量設定
	virtual int SoundUpdate( int );			// ストリーム更新
};


// サウンドオブジェクト
class SND6 : public cRing {
private:
	std::vector<SndDev*> sdev;	// ストリームポインタ配列
	int Volume;					// マスター音量
	int SampleRate;				// サンプリングレート
	int BSize;					// バッファサイズ(倍率)
	CBF_SND CbFunc;				// コールバック関数へのポインタ
	void* CbData;				// コールバック関数に渡すデータ
	
public:
	SND6();									// コンストラクタ
	~SND6();								// デストラクタ
	
	bool Init( void*, CBF_SND, int, int );	// 初期化
	
	bool ConnectStream( SndDev* );			// ストリーム接続
	
	void Play();							// 再生
	void Pause();							// 停止
	
	bool SetSampleRate( int, int = 0 );		// サンプリングレート設定
	int GetSampleRate();					// サンプリングレート取得
	
	int GetBufferSize();					// バッファサイズ(倍率)取得
	
	void SetVolume( int );					// マスター音量設定
	
	int PreUpdate( int, cRing* = nullptr );	// サウンド事前更新関数
	void Update( BYTE*, int );				// サウンド更新関数
};

#endif	// SOUND_H_INCLUDED
