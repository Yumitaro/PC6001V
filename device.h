#ifndef DEVICE_H_INCLUDE
#define DEVICE_H_INCLUDE

#include <memory>
#include <unordered_map>

#include "typedef.h"


#define DEV_ID(a)	BTODW((BYTE)a[0], (BYTE)a[1], (BYTE)a[2], (BYTE)a[3])


class VM6;


// ---------------------------------------------------------------------------
//	デバイスのインターフェース
//	  Original     : cisc
//	  Modification : Yumitaro
// ---------------------------------------------------------------------------
struct IDevice {
	typedef DWORD ID;
	typedef BYTE (IDevice::*InFuncPtr)( int );
	typedef void (IDevice::*OutFuncPtr)( int, BYTE );
	typedef BYTE (IDevice::*RFuncPtr)( BYTE*, WORD );
	typedef void (IDevice::*WFuncPtr)( BYTE*, WORD, BYTE );
	
	struct Descriptor{
		std::unordered_map<int, InFuncPtr>  indef;
		std::unordered_map<int, OutFuncPtr> outdef;
	};
	
	virtual const ID& GetID() const = 0;
	virtual const Descriptor& GetDescriptors() const = 0;
	virtual void EventCallback( int, int ) = 0;
};


// ---------------------------------------------------------------------------
//	Device
//	  Original     : cisc
//	  Modification : Yumitaro
// ---------------------------------------------------------------------------
class Device : public IDevice {
protected:
	VM6* vm;
	
	ID id;
	Descriptor descs;

public:
	Device( VM6*, const ID& );
	virtual ~Device();
	
	const ID& GetID() const override;
	const Descriptor& GetDescriptors() const override { return descs; }
	virtual void EventCallback( int, int ) override;
};


// ---------------------------------------------------------------------------
// デバイスリストクラス
//	  Original     : cisc
//	  Modification : Yumitaro
// ---------------------------------------------------------------------------
class DeviceList {
public:
	typedef IDevice::ID ID;

private:
	struct Node{
		std::shared_ptr<IDevice> entry;
		int count;
	};
	
	std::unordered_map<int, Node> NodeMap;
	std::shared_ptr<IDevice> dummydev;

public:
	DeviceList();
	~DeviceList();
	
	bool Add( const std::shared_ptr<IDevice>& );
	bool Del( const ID );
	std::shared_ptr<IDevice>& Find( const ID );
};


#endif // DEVICE_H_INCLUDE
