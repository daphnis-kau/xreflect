/**@file  		CScriptLua.h
* @brief		LUA VM base wrapper
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once
#ifndef __SCRIPT_LUA_H__
#define __SCRIPT_LUA_H__
#include "Reflection.h"
#include <map>

struct lua_State;
struct lua_Debug;

namespace ScriptBinding
{
	using namespace XReflect;
	class CDebugLua;

	class CScriptLua : public XReflect::ICallbackHook
	{
		typedef void* (*AllocFun) (void*, void*, size_t, size_t);

		std::vector<lua_State*>	m_vecLuaState; 
		bool					m_bPreventExeInRunString;

	public:
		struct SRaiiLua
		{
			CScriptLua*			m_pScript;
			SRaiiLua( lua_State* pL, CScriptLua* pScript = nullptr );
			~SRaiiLua();
		};

		//==============================================================================
		// built keys
		//==============================================================================
		static char* const		ms_pPointerStart;
		static char* const		ms_pWeakTable;
		static char* const		ms_pReferenceTable;
		static char* const		ms_pScriptLua;
		static char* const		ms_pFirstClassInfo;
		static char* const		ms_pErrorHandler;
		static char* const		ms_pCppObject;
		static char* const		ms_pCppClass;
		static char* const		ms_pPointerEnd;

		friend class CDebugLua;
		friend class CLuaBuffer;
	protected:
		typedef std::map<XReflect::SFunctionTable*, XReflect::SFunctionTable*> COld2NewMap;
		typedef std::map<const XReflect::CClass*, COld2NewMap> CFunTableMapOfClass;
		CFunTableMapOfClass	m_mapVirtualTableOld2NewOfClass;
	public:
		virtual XReflect::SFunctionTable*	FindHookVirtualTable( const XReflect::CClass* pClass, 
									XReflect::SFunctionTable* pOldFunTable );
		virtual void			SetHookVirtualTable( const XReflect::CClass* pClass, 
									XReflect::SFunctionTable* pOldFunTable, 
									XReflect::SFunctionTable* pNewFunTable );
		CScriptLua( void );
		~CScriptLua( void );

	protected:
		//==============================================================================
		// aux function
		//==============================================================================
		static void				PushCppObjWeakTable( lua_State* pL );
		static int32			GetIndexClosure( lua_State* pL );
		static int32			GetNewIndexClosure( lua_State* pL );
		static int32			GetInstanceField( lua_State* pL );
		static int32			SetInstanceField( lua_State* pL );

		static int32			ObjectGC( lua_State* pL );
		static int32			ObjectConstruct( lua_State* pL );
		static int32			ClassCast( lua_State* pL );
		static int32			CallByLua( lua_State* pL );

		static int32			ToString( lua_State* pL );
		static CScriptLua*		ToScriptLua( lua_State* pL );
		static void*			Realloc( void* pContex, void* pPreBuff, 
									size_t nOldSize, size_t nNewSize );	
		
		static bool				GetGlobObject( lua_State* pL, const char* szKey );
		static bool				SetGlobObject( lua_State* pL, const char* szKey );
		static void				DebugHookProc( lua_State *pState, lua_Debug* pDebug );
		
		void					BuildRegisterInfo();

		virtual bool			OnCallBack( const CField* pField, void* pRetBuf, void** pArgArray );
		virtual void			OnDestruct( const CField* pField, void* pObject );
	public:
		//==============================================================================
		// common function
		//==============================================================================
		static void*			NewLuaObj( lua_State* pL, const CClass* pClass );
		static void             RegistToLua( lua_State* pL, const CClass* pClass, void* pObj, 
									int32 nObj, int32 nGlobalWeakTable );
		static void				RegisterObject( lua_State* pL, 
									const CClass* pClass, void* pObj, int32 nWeakTable );
		static const char*		PresentValue( lua_State* pL, int32 nStkId, size_t* nLen = nullptr );

		lua_State*              GetLuaState();
		static CScriptLua*		GetScript( lua_State* pL );


	protected:
		virtual bool			Evaluate( const char* szExpression, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool			Set( void* pObject, int32 nIndex, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool			Get( void* pObject, int32 nIndex, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool			Set( void* pObject, const char* szName, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool			Get( void* pObject, const char* szName, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool			Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, const char* szFunction, void** aryArg );
		virtual bool        	Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, void* pFunction, void** aryArg );
		
	public:
		virtual bool        	RunString( const void* pBuffer, size_t nSize, const char* szFileName, bool bForce = false );
		virtual void*			GetVM();
		virtual int32			IncRef( void* pObj );
		virtual int32			DecRef( void* pObj );
		virtual void            UnlinkCppObjFromScript( void* pObj );
		virtual void        	GC();
		virtual void        	GCAll();
		virtual bool			IsValid( void* pObject );

		template<typename FunType, typename RetType, typename... Param>
		bool RunFunction( RetType* pRetBuf, FunType pFun, Param ... p )
		{
			void* aryParam[sizeof...( p ) + 1] = { &p ... };
			static STypeInfo aryInfo[] = { GetTypeInfo<Param>()..., GetTypeInfo<RetType>() };
			static STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
			return Call( TypeInfo, pRetBuf, pFun, aryParam );
		}

		template<typename FunType, typename... Param>
		bool RunFunction( nullptr_t, FunType pFun, Param ... p )
		{
			void* aryParam[sizeof...( p ) + 1] = { &p ... };
			static STypeInfo aryInfo[] = { GetTypeInfo<Param>()..., GetTypeInfo<void>() };
			static STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
			return Call( TypeInfo, nullptr, pFun, aryParam );
		}
	};
}

#endif
