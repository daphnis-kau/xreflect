/** @file 06_virtual_override/main.cpp */
#include "Reflection.h"
#include <cstdio>
#include <functional>
#include <map>
#include <string>

class IAnimal
{
public:
	virtual void Speak()        { printf( "  [C++] IAnimal::Speak\n" ); }
	virtual int  GetAge() const { return 0; }
	virtual ~IAnimal()          {}
};

DEFINE_CLASS_BEGIN( IAnimal )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSCALLBACK( Speak )
	REGIST_CLASSCALLBACK( GetAge )
DEFINE_CLASS_END()

class IDamageable
{
public:
	int  m_HP;
	IDamageable() : m_HP( 100 ) {}
	virtual void OnDamage( int nAmount )
		{ printf( "  [C++] OnDamage(%d), HP=%d\n", nAmount, m_HP ); }
	virtual ~IDamageable() {}
};

DEFINE_CLASS_BEGIN( IDamageable )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSCALLBACK( OnDamage )
DEFINE_CLASS_END()

class IRenderable
{
public:
	virtual bool CanRender() const { return true; }
	virtual ~IRenderable()         {}
};

DEFINE_CLASS_BEGIN( IRenderable )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSCALLBACK( CanRender )
DEFINE_CLASS_END()

using OverrideFn = std::function<void(
	const XReflect::CField*, void* pRetBuf, void** pArgArray )>;

class MockScriptHook : public XReflect::ICallbackHook
{
	std::map<XReflect::SFunctionTable*, XReflect::SFunctionTable*> m_Old2New;
	std::map<std::string, OverrideFn> m_Overrides;

public:
	void AddOverride( const char* szFieldName, OverrideFn fn )
	{
		m_Overrides[ szFieldName ] = std::move( fn );
	}

	XReflect::SFunctionTable* FindHookVirtualTable(
		const XReflect::CClass*,
		XReflect::SFunctionTable* pOldFunTable ) override
	{
		auto it = m_Old2New.find( pOldFunTable );
		return it != m_Old2New.end() ? it->second : nullptr;
	}

	void SetHookVirtualTable(
		const XReflect::CClass*,
		XReflect::SFunctionTable* pOldFunTable,
		XReflect::SFunctionTable* pNewFunTable ) override
	{
		m_Old2New[ pOldFunTable ] = pNewFunTable;
	}

	bool OnCallBack(
		const XReflect::CField* pField,
		void* pRetBuf, void** pArgArray ) override
	{
		auto it = m_Overrides.find( pField->GetName().c_str() );
		if( it == m_Overrides.end() )
			return false;
		printf( "  [Hook] %s intercepted\n", pField->GetName().c_str() );
		it->second( pField, pRetBuf, pArgArray );
		return true;
	}

	void OnDestruct( const XReflect::CField*, void* ) override
	{
		printf( "  [Hook] destruction notified\n" );
	}

	~MockScriptHook()
	{
		XReflect::CReflection& R = XReflect::CReflection::GetInstance();
		for( auto& pair : m_Old2New )
			R.FreeVirtualTable( pair.second );
	}
};

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();

	printf( "=== Virtual Override ===\n\n" );

	MockScriptHook hook;

	hook.AddOverride( "Speak", []( const CField*, void*, void** )
		{ printf( "  [Hook] Speak -> override\n" ); } );

	hook.AddOverride( "GetAge", []( const CField*, void* pRet, void** )
		{ *(int*)pRet = 42; } );

	hook.AddOverride( "OnDamage", []( const CField*, void*, void** pArgs )
	{
		int nAmount = *(int*)( (void**)pArgs )[1];
		printf( "  [Hook] OnDamage(%d) -> override\n", nAmount );
	} );

	hook.AddOverride( "CanRender", []( const CField*, void* pRet, void** )
		{ *(bool*)pRet = false; } );

	// ── IAnimal ──
	{
		const CClass* pCls = R.FindClassByTypeID( TypeName<IAnimal>() );
		IAnimal* pObj = new IAnimal();

		printf( "── IAnimal ──\nBefore: " );
		pObj->Speak();
		printf( "GetAge=%d\n", pObj->GetAge() );

		pCls->HookVirtualTable( pObj, &hook );

		printf( "After:  " );
		pObj->Speak();
		printf( "GetAge=%d\n", pObj->GetAge() );

		pCls->UnhookVirtualTable( pObj );
		delete pObj;
	}

	// ── IDamageable ──
	{
		const CClass* pCls = R.FindClassByTypeID( TypeName<IDamageable>() );
		IDamageable* pObj = new IDamageable();

		printf( "\n── IDamageable ──\nBefore: " );
		pObj->OnDamage( 30 );

		pCls->HookVirtualTable( pObj, &hook );

		printf( "After:  " );
		pObj->OnDamage( 30 );

		pCls->UnhookVirtualTable( pObj );
		delete pObj;
	}

	// ── IRenderable ──
	{
		const CClass* pCls = R.FindClassByTypeID( TypeName<IRenderable>() );
		IRenderable* pObj = new IRenderable();

		printf( "\n── IRenderable ──\nBefore: CanRender=%s\n",
			pObj->CanRender() ? "true" : "false" );

		pCls->HookVirtualTable( pObj, &hook );

		printf( "After:  CanRender=%s\n",
			pObj->CanRender() ? "true" : "false" );

		pCls->UnhookVirtualTable( pObj );
		delete pObj;
	}

	// Inspect overridable functions
	{
		const CClass* pA = R.FindClassByTypeID( TypeName<IAnimal>() );
		printf( "\n── Overridable inspection ──\n" );
		printf( "Overridable count: %u\n", pA->GetOverridableCount() );
		for( uint32 i = 0; i < pA->GetOverridableCount(); ++i )
		{
			const CField* pF = pA->GetOverridableFun( i );
			printf( "  [%u] %s  vtable_idx=%d\n",
				i, pF->GetName().c_str(), pF->GetVirtualTableIndex() );
		}
		const CField* pAge = pA->FindOverrideFun( 1 );
		if( pAge )
			printf( "FindOverrideFun(1) = %s\n", pAge->GetName().c_str() );
	}

	return 0;
}
