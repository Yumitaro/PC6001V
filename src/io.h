#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <memory>
#include <vector>

#include "typedef.h"
#include "device.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------------
//	IOBus : I/O空間を提供するクラス
//	  Original     : cisc
//	  Modification : Yumitaro
// ---------------------------------------------------------------------------
class IOBus {
// 型定義
public:
	typedef Device::InFuncPtr  InFuncPtr;
	typedef Device::OutFuncPtr OutFuncPtr;
	
	struct InBank{
		IDevice* device;
		InFuncPtr func;
	};
	struct OutBank{
		IDevice* device;
		OutFuncPtr func;
	};
	
	enum ConnectRule{
		end = 0, portin = 1, portout = 2
	};
	struct Connector{
		DWORD bank;
		BYTE rule;
		BYTE id;
	};
	
private:
	class DummyIO : public Device{
	public:
		DummyIO();
		~DummyIO();
		
		BYTE dummyin( int );
		void dummyout( int, BYTE );
	};
	
	static DummyIO dummyio;
	
	bool ConnectIn ( int bank, const std::shared_ptr<IDevice>&, InFuncPtr  );
	bool ConnectOut( int bank, const std::shared_ptr<IDevice>&, OutFuncPtr );
	
// ここから本体
public:
	IOBus();
	virtual ~IOBus();
	
	virtual bool Init( int );
	
	bool Connect( const std::shared_ptr<IDevice>&, const std::vector<Connector>& );
	bool Disconnect( const DeviceList::ID );
	
	BYTE In( int );
	void Out( int, BYTE );
	
private:
	std::vector<std::vector<InBank>> ins;
	std::vector<std::vector<OutBank>> outs;
	
	DeviceList devlist;
};



class IO6 : public IOBus {
private:
	std::vector<int> Iwait;			// IN ウェイトテーブル
	std::vector<int> Owait;			// OUTウェイトテーブル
	
public:
	IO6();
	~IO6();
	
	bool Init( int ) override;				// 初期化
	
	BYTE In( int, int* = nullptr );			// IN関数
	void Out( int, BYTE, int* = nullptr );	// OUT関数
	
	void SetInWait( int, int );				// IN ウェイト設定
	void SetOutWait( int, int );			// OUTウェイト設定
	int GetInWait( int );					// IN ウェイト取得
	int GetOutWait( int );					// OUTウェイト取得
};


#endif		// IO_H_INCLUDED
