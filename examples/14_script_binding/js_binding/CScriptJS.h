/**@file  		CScriptJS.h
* @brief		Javascript engine
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#ifndef __SCRIPT_JS_H__
#define __SCRIPT_JS_H__
#pragma warning(push)
#pragma warning(disable: 4576)
#include "quickjs.h"
#include "quickjs-libc.h"
#pragma warning(pop)
#include "Reflection.h"
#include <map>

struct JSRuntime;
struct JSContext;
struct JSModuleDef;
typedef uint32_t JSAtom;

namespace ScriptBinding
{
	using namespace XReflect;
	struct SJSCallInfo;
	struct SJSObjInfo;
	struct SCallIndex { uint16 nFunIndex, nMagic; };
	typedef std::vector<SJSCallInfo*> CJSCallInfoList;
	typedef std::map<const void*, SJSCallInfo> CJSCallInfoMap;
	typedef std::map<void*, SJSObjInfo> CJSObjInfoMap;

	struct SJSCallInfo
	{
		bool			bDestructor;
		JSAtom			Name;
		union { const CField* pField; const CClass* pClass; };
	};

	struct SJSObjInfo
	{
		void*			m_pObject;
		JSValue			m_Object;
		const CClass*	m_pClass;
		bool			m_bRecycle;
		bool			m_bFirstAddress;
	};

	class CScriptJS : public XReflect::ICallbackHook
	{
		JSRuntime*		m_pRunTime;
		JSContext*		m_pContext;
		JSAtom			m_CppField;
		JSAtom			m_Prototype;
		JSAtom			m_Deconstruction;
		JSAtom			m___proto__;
		JSValue			m_objNameSpace;
		JSValue			m_objClass;
		JSValue			m_FunctionPrototype;
		JSClassID		m_ArrayBufferClassID;
		JSClassID		m_ShareBufferClassID;
		JSClassID		m_ClassID;

		CJSCallInfoList	m_listCallFromVM;
		CJSCallInfoMap	m_listCallInfoMap;

		SJSObjInfo*		m_pFreeObjectInfo;
		CJSObjInfoMap	m_mapObjInfo;
	public:
		friend class CJSObject;
		friend struct SFunctions;
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
		CScriptJS( const char* strDebugHost, uint16 nDebugPort );
		~CScriptJS( void );

	protected:
		static void*	Malloc( JSMallocState* s, size_t nSize );
		static void		Free( JSMallocState* s, void* pBuffer );
		static void*	Realloc( JSMallocState* s, void* pPreBuff, size_t nNewSize );
		static size_t	GetMemorySize( const void* pBuffer );

		static void*	LoadModule( JSContext* pContext,
							const char* szModuleName, CScriptJS* pThis );
		static JSValue	Log( JSContext* pContext, JSValue this_val,
							int argc, JSValue* argv );
		static JSValue	Break( JSContext* pContext, JSValue this_val, 
							int argc, JSValue* argv );
		static JSValue	NewObject( JSContext* pContext, 
							JSValueConst func_obj, JSValueConst this_val,
							int argc, JSValueConst* argv, int flags );
		static void		ClassFinalizer( JSRuntime* pRuntime, JSValue val );
		static JSValue	CallFromVM( JSContext* pContext, JSValue this_val,
							int argc, JSValue* argv, int32 nMagic, uint16 nIndex );
		static JSValue	CallFromVM( JSContext* pContext, JSValue this_val,
							int32 nMagic, uint16 nIndex );
		static JSValue	CallFromVM( JSContext* pContext, JSValue this_val,
							JSValue val, int32 nMagic, uint16 nIndex );

		SCallIndex		AddCallInfo( const CField* pField );
		SCallIndex		AddCallInfo( const CClass* pClass );
		SJSCallInfo&	GetCallInfo( SCallIndex Index );
		SJSCallInfo*	GetCallInfo( const CField* pField );

		SJSObjInfo*		AllocObjectInfo();
		void			FreeObjectInfo( SJSObjInfo* pObjectInfo );
		void			BuildRegisterInfo();

		virtual bool	OnCallBack( const CField* pField, void* pRetBuf, void** pArgArray );
		virtual void	OnDestruct( const CField* pField, void* pObject );

	public:
		JSContext*		GetContext() const { return m_pContext; }
		bool			IsBufferClass( JSClassID nClassID );
		void*			BindObj( void* pObject, JSValue ScriptObj, 
							const CClass* pInfo, bool bRecycle );
		void			UnbindObj( SJSObjInfo* pObjectInfo, bool bFromGC );
		SJSObjInfo*		FindExistObjInfo( void* pObj );
		JSValue			GetClassScope( const CClass* pClass );

	protected:
		virtual bool	Evaluate( const char* szExpression, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool	Set( void* pObject, int32 nIndex, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool	Get( void* pObject, int32 nIndex, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool	Set( void* pObject, const char* szName, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool	Get( void* pObject, const char* szName, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool	Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, const char* szFunction, void** aryArg );
		virtual bool	Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, void* pFunction, void** aryArg );

	public:
		virtual bool    RunString( const void* pBuffer, size_t nSize, const char* szFileName, bool bForceBuild = false );
		virtual void*	GetVM();
		virtual int32	IncRef( void* pObj );
		virtual int32	DecRef( void* pObj );
		virtual void	UnlinkCppObjFromScript( void* pObj );
		virtual bool	IsValid( void* pObject );
		virtual void    GC();
		virtual void    GCAll();

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

	inline bool CScriptJS::IsBufferClass( JSClassID nClassID )
	{
		return nClassID == m_ArrayBufferClassID || 
			nClassID == m_ShareBufferClassID;
	}
}

#endif
