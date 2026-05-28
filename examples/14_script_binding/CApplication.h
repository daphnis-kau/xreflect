#pragma once
#include "Reflection.h"
#include <map>
#include <string>

enum ETestEnum
{
	eTE_0 = 1234,
};

struct SAddress
{
	SAddress(uint32 i = 0, uint16 p = 0)
		: nIP(i), nPort(p){}
	uint32 nIP;
	uint16 nPort;

	virtual void OnTestVirtualWithConstruct() const {};
	const uint16& GetPort() { return nPort; }
	void SetPort( uint16 n ) { nPort = n; }
};

struct SApplicationConfig
{
	static int nCount;

	SApplicationConfig()
	{
	}

	SApplicationConfig(const SApplicationConfig& Config)
	{
		memcpy(szName, Config.szName, sizeof(szName));
		nID = Config.nID;
		Address = Config.Address;
	}

	~SApplicationConfig()
	{
	}

	void SetName( const char* Name )
	{
		strcpy( szName, Name );
	}

	const char*	GetName()
	{
		return szName;
	}

	char		szName[32];
	uint32		nID;
	SAddress	Address;
	int32		aryInt[3][2][4];
};

class IApplicationHandler
{
public:
	virtual const char* OnTestPureVirtual(
		ETestEnum e, int8 v0, int16 v1, int32 v2, int64 v3, long v4,
		uint8 v5, uint16 v6, uint32 v7, uint64 v8, unsigned long v9,
		float v10, double v11, const char* v12 ) const = 0;

	virtual const char* OnTestNoParamPureVirtual() const = 0;
	virtual void* TestBuffer(void* buffer) = 0;
};

namespace Base
{
	class CApp
	{
	public:
		static CApp& GetInstance();
	protected:
		CApp()  { s_pInstance = this; }
		~CApp() { s_pInstance = nullptr; }
	private:
		static CApp* s_pInstance;
	};
}

typedef std::map<int32, std::string> TestMap;
class CApplication : public Base::CApp
{
	CApplication& operator=( const CApplication & );
protected:
	CApplication();
	virtual ~CApplication(){}

	virtual void			OnInit() {};
	virtual void			OnLoop() {};
	virtual void			OnExit() {};
	
	std::string				m_strName;
	TestMap					m_mapTest;
	IApplicationHandler*	m_pHandler;

public:
	static CApplication& GetInst();

	TestMap& TestTemplateMap( int nKey, const char* szValue );
	IApplicationHandler* TestCallObjectPointer( IApplicationHandler* Handler );
	SApplicationConfig& TestCallObjectReference( SApplicationConfig& Config );
	SApplicationConfig TestCallObjectValue( SApplicationConfig Config );

	virtual IApplicationHandler* TestVirtualObjectPointer( IApplicationHandler* Handler );
	virtual SApplicationConfig& TestVirtualObjectReference( SApplicationConfig& Config );
	virtual SApplicationConfig TestVirtualObjectValue( SApplicationConfig Config );

	const char* TestCallPOD( 
		ETestEnum e, int8 v0, int16 v1, int32 v2, int64 v3, long v4,
		uint8 v5, uint16 v6, uint32 v7, uint64 v8, unsigned long v9,
		float v10, double v11, const char* v12 );

	const char* TestNoParamFunction();
	void* TestBuffer(void* buffer);
};