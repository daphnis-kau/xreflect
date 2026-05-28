#pragma warning(disable: 4576)
#include "CField.h"
#include "CClass.h"
#include "CReflection.h"
#include "CScriptJS.h"
#include "CTypeJS.h"
#include <functional>
#include <array>

#ifdef _MSC_VER
#include <malloc.h>
static size_t GetAllocSize( void* p ) { return p ? _msize( p ) : 0; }
#else
#include <malloc.h>
static size_t GetAllocSize( void* p ) { return p ? malloc_usable_size( p ) : 0; }
#endif

namespace ScriptBinding
{
	struct SFunctions
	{
		JSValue( *funCall )( JSContext*, JSValue, int32, JSValue*, int32 );
		JSValue( *funGetter )( JSContext*, JSValue, int32 );
		JSValue( *funSetter )( JSContext*, JSValue, JSValue, int32 );

		template<int32 n, typename ... ParamTyps>
		static JSValue CallFromVM( ParamTyps ... Params )
		{
			return CScriptJS::CallFromVM( Params..., n );
		}

		template<int32 n>
		static void Build( std::array<SFunctions, 256>& aryFunctions )
		{
			SFunctions Functions =
			{
				&CallFromVM<n, JSContext*, JSValue, int32, JSValue*, int32>,
				&CallFromVM<n, JSContext*, JSValue, int32>,
				&CallFromVM<n, JSContext*, JSValue, JSValue, int32>
			};
			aryFunctions[n] = Functions;
			return Build<n - 1>( aryFunctions );
		}

		template<>
		static void Build<-1>( std::array<SFunctions, 256>& aryFunctions ) {}
	};

	static CJSTypeBase* s_aryJSTypes[] =
	{
		nullptr,
		&TJSValue<char>::GetInst(),
		&TJSValue<int8>::GetInst(),
		&TJSValue<int16>::GetInst(),
		&TJSValue<int32>::GetInst(),
		&TJSValue<int64>::GetInst(),
		&TJSValue<long>::GetInst(),
		&TJSValue<uint8>::GetInst(),
		&TJSValue<uint16>::GetInst(),
		&TJSValue<uint32>::GetInst(),
		&TJSValue<uint64>::GetInst(),
		&TJSValue<ulong>::GetInst(),
		&TJSValue<wchar_t>::GetInst(),
		&TJSValue<char16_t>::GetInst(),
		&TJSValue<char32_t>::GetInst(),
		&TJSValue<bool>::GetInst(),
		&TJSValue<float>::GetInst(),
		&TJSValue<double>::GetInst(),
		&TJSValue<const char*>::GetInst(),
	};

	inline CJSTypeBase* GetJSTypeBase( UPakDataType Type )
	{
		uint32 nPoints = Type.GetMulPointer() + Type.IsReference();
		if( nPoints >= 2 )
			return &TJSValue<void*>::GetInst();

		if( Type.IsBaseType() )
		{
			if( nPoints )
				return &TJSValue<void*>::GetInst();
			return s_aryJSTypes[Type.GetDataType()];
		}

		auto pClass = Type.GetClass();
		if( pClass->IsEnum() )
		{
			if( nPoints )
				return &TJSValue<void*>::GetInst();
			uint32 nSize = pClass->GetClassAlignSize();
			if( nSize == 1 )
				return s_aryJSTypes[eDT_int8];
			if( nSize == 2 )
				return s_aryJSTypes[eDT_int16];
			return s_aryJSTypes[eDT_int32];
		}
		else
		{
			if( !nPoints )
				return &CJSValueObject::GetInst();
			return &CJSObject::GetInst();
		}
	}

	//====================================================================================
	// CScriptJS
	//====================================================================================
	CScriptJS::CScriptJS( 
		const char* strDebugHost, uint16 nDebugPort )
		: m_pRunTime( nullptr )
		, m_pContext( nullptr )
		, m_pFreeObjectInfo( nullptr )
	{
		JSMallocFunctions Funs = { &Malloc, &Free, &Realloc, &GetMemorySize };
		m_pRunTime = JS_NewRuntime2( &Funs, this );

		JS_SetRuntimeOpaque( m_pRunTime, this );
		m_pContext = JS_NewContext( m_pRunTime );
		assert( m_pContext );
		JS_SetContextOpaque( m_pContext, this );
		JS_AddIntrinsicBigFloat( m_pContext );
		JS_AddIntrinsicBigDecimal( m_pContext );
		JS_AddIntrinsicOperators( m_pContext );
		JS_EnableBignumExt( m_pContext, true );

		JS_SetModuleLoaderFunc( m_pRunTime, NULL, 
			(JSModuleLoaderFunc*)&CScriptJS::LoadModule, this );
		auto globalObj = JS_GetGlobalObject( m_pContext );
		JS_SetPropertyStr( m_pContext, globalObj, "window", globalObj );

		auto console = JS_NewObject( m_pContext );
		JS_SetPropertyStr( m_pContext, console, "log",
			JS_NewCFunction( m_pContext, &CScriptJS::Log, "log", 1 ) );
		JS_SetPropertyStr( m_pContext, globalObj, "console", console );

		JS_SetPropertyStr( m_pContext, globalObj, "gdb",
			JS_NewCFunction( m_pContext, &CScriptJS::Break, "gdb", 1 ) );

#ifdef CONFIG_DEBUGGER
		JS_SetDebuggerMode( m_pContext, true );
#endif

		auto ArrayBuffer = JS_NewArrayBuffer( 
			m_pContext, nullptr, 0, nullptr, nullptr, false );
		auto ShareBuffer = JS_NewArrayBuffer( 
			m_pContext, nullptr, 0, nullptr, nullptr, true );
		m_ArrayBufferClassID = JS_GetClassID( ArrayBuffer, nullptr );
		m_ShareBufferClassID = JS_GetClassID( ShareBuffer, nullptr );
		m_CppField = JS_NewAtom( m_pContext, "__cpp_obj_info__" );
		m_Prototype = JS_NewAtom( m_pContext, "prototype" );
		m_Deconstruction = JS_NewAtom( m_pContext, "deconstruction" );
		m___proto__ = JS_NewAtom( m_pContext, "__proto__" );
		JS_FreeValue( m_pContext, ArrayBuffer );
		JS_FreeValue( m_pContext, ShareBuffer );

		const char* szInitJS =
			"var XJS = {};\n"
			"(function()\n"
			"{\n"
			"	XJS.class = function(Derive, szGlobalName, Base)\n"
			"	{\n"
			"		if (Base)\n"
			"		{\n"
			"			for (var Property in Base)\n"
			"			{\n"
			"				if (!Base.hasOwnProperty(Property))\n"
			"					continue;\n"
			"				var Descriptor = Object.getOwnPropertyDescriptor(Base, Property);\n"
			"				if (!Descriptor.get && !Descriptor.set)\n"
			"					Derive[Property] = Base[Property];\n"
			"				else\n"
			"					Object.defineProperty(Derive, Property, Descriptor);\n"
			"			}\n"
			"			var Super = function() {};\n"
			"			Super.prototype = Base.prototype;\n"
			"			Derive.prototype = new Super();\n"
			"			Derive.prototype.__super = Base;\n"
			"		}\n"
			"		Derive.prototype.__class__ = Derive;\n"

			"		var s_aryAlloc = [];\n"
			"		var s_nAllocCount = 0;\n"
			"		Derive.Alloc = function()\n"
			"		{\n"
			"			if (s_nAllocCount)\n"
			"				return s_aryAlloc[--s_nAllocCount];\n"
			"			return new Derive;\n"
			"		}\n"

			"		Derive.Free = function(Obj)\n"
			"		{\n"
			"			s_aryAlloc[s_nAllocCount++] = Obj;\n"
			"		}\n"

			"		if (!szGlobalName)\n"
			"			return Derive;\n"
			"		var CurPackage = window, aryPath = szGlobalName.split('.');\n"
			"		for (var i = 0; i < aryPath.length - 1; i++)\n"
			"		{\n"
			"			if (!CurPackage[aryPath[i]])\n"
			"				CurPackage[aryPath[i]] = {}\n"
			"			CurPackage = CurPackage[aryPath[i]];\n"
			"		}\n"
			"		CurPackage[aryPath[i]] = Derive;\n"
			"		Derive.prototype.__class__name__ = aryPath[i];\n"
			"		Derive.prototype.__class__path__ = szGlobalName;\n"
			"		return Derive;\n"
			"	};\n"

			"	XJS.getset = function(Class, szName, funGet, funSet)\n"
			"	{\n"
			"		if (funGet && funSet)\n"
			"			Object.defineProperty(Class, szName, { get:funGet, set : funSet, enumerable : false, configurable : true });\n"
			"		else if (funGet)\n"
			"			Object.defineProperty(Class, szName, { get:funGet, enumerable : false, configurable : true });\n"
			"		else if (funSet)\n"
			"			Object.defineProperty(Class, szName, { set:funSet, enumerable : false, configurable : true });\n"
			"	};\n"

			"	XJS.getClass = function(szName)\n"
			"	{\n"
			"		var CurPackage = window, aryPath = szName.split('.');\n"
			"		for (var i = 0; i < aryPath.length - 1; i++)\n"
			"		{\n"
			"			if (!CurPackage[aryPath[i]])\n"
			"				return null;\n"
			"			CurPackage = CurPackage[aryPath[i]];\n"
			"		}\n"
			"		return CurPackage[aryPath[i]];\n"
			"	}\n"

			"	XJS.classCast = function( obj, __class, ... arguments )\n"
			"	{\n"
			"		obj.__proto__ = __class.prototype;\n"
			"		__class.apply( obj, arguments );\n"
			"		return obj;\n"
			"	}\n"

			"	return this;\n"
			"})()"
		;
		RunString( szInitJS, strlen( szInitJS ), "init.js", true );

		auto Scope = JS_GetPropertyStr( m_pContext, globalObj, "XJS" );
		auto Class = JS_GetPropertyStr( m_pContext, Scope, "class" );
		m_objClass = Class;
		m_objNameSpace = Scope;

		BuildRegisterInfo();
	}

	CScriptJS::~CScriptJS(void)
	{
		m_listCallInfoMap.clear();
		m_listCallFromVM.clear();
	}

	//====================================================================
	// Internal protected functions
	//====================================================================	
	void* CScriptJS::Malloc( JSMallocState* s, size_t nSize )
	{
		return Realloc( s, nullptr, nSize );
	}

	void CScriptJS::Free( JSMallocState* s, void* pBuffer )
	{
		Realloc( s, pBuffer, 0 );
	}

	void* CScriptJS::Realloc( JSMallocState* s, void* pPreBuff, size_t nNewSize )
	{
		if( pPreBuff )
		{
			s->malloc_size -= GetAllocSize( pPreBuff );
			s->malloc_count--;
		}
		void* p = realloc( pPreBuff, nNewSize );
		if( p )
		{
			s->malloc_size += GetAllocSize( p );
			s->malloc_count++;
		}
		return p;
	}

	size_t CScriptJS::GetMemorySize( const void* pBuffer )
	{
		return pBuffer ? GetAllocSize( (void*)pBuffer ) : 0;
	}

	void* CScriptJS::LoadModule( JSContext* pContext,
		const char* szModuleName, CScriptJS* pThis )
	{
		return nullptr;
	}

	JSValue CScriptJS::Log( JSContext* pContext, 
		JSValue this_val, int argc, JSValue* argv )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		for( int32 i = 0; i < argc; i++ )
		{
			const char* szValue = JS_ToCString( pContext, argv[i] );
			printf( "%s", szValue );
			printf( " " );
		}
		printf( "\n", -1 );
		return JS_UNDEFINED;
	}

	JSValue CScriptJS::Break( JSContext* pContext,
		JSValue this_val, int argc, JSValue* argv )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		return JS_NULL;
	}

	JSValue CScriptJS::NewObject( JSContext* pContext, JSValueConst func_obj,
		JSValueConst this_val, int argc, JSValueConst* argv, int flags )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		const CClass* pClass = nullptr;
		JS_GetClassID( func_obj, (void**)&pClass );
		assert( pClass != nullptr );

		auto __Proto__ = JS_GetProperty( 
			pContext, this_val, Script.m___proto__ );
		if( __Proto__ == Script.m_FunctionPrototype )
		{
			auto ProtoType = JS_GetProperty( 
				pContext, func_obj, Script.m_Prototype );
			JS_SetProperty( pContext, this_val, Script.m___proto__, ProtoType );
		}
		JS_FreeValue( pContext, __Proto__ );

		uint32 nEvaluaError = UINT32_MAX;
		int32 nConstructor = UINT16_MAX;
		int32 nCount = pClass->GetCustomConstructorCount();
		for( int32 i = eBCI_DefaultConstructor; i < nCount; i++ )
		{
			// Ignore move constructor when copy constructor exists; script objects are GC-based, no rvalue reference concept
			if( pClass->IsCopyConstructible() && i == eBCI_MoveConstructor )
				continue;
			auto pParamInfo = pClass->GetConstructorParamsInfo( i );
			if( !pParamInfo || pParamInfo->nParamCount != argc )
				continue;
			uint32 nCurError = 0;
			for( int32 j = 0; j < argc; j++ )
			{
				JSValue arg = argv[j];
				auto Type = pParamInfo->GetParamType()[j];
				auto DataType = Type.GetDataType();
				auto nPointers = Type.GetMulPointer();
				if( DataType == eDT_const_char_str && !nPointers )
				{
					if( JS_IsString( arg ) )
						continue;
					nCurError += JS_IsNumber( arg ) ? 0x0001 : 0x0002;
				}
				else if( DataType <= eDT_enum && !nPointers )
				{
					if( JS_IsNumber( arg ) )
						continue;
					nCurError += JS_IsString( arg ) ? 0x0001 : 0x10000;
				}
				else
				{
					if( JS_IsObject( arg ) )
						continue;
					nCurError += 0x10000;
				}
			}
			if( nCurError >= nEvaluaError )
				continue;
			nConstructor = i;
			nEvaluaError = nCurError;
		}

		if( nConstructor == UINT16_MAX )
			return JS_NULL;


		auto pParamInfo = pClass->GetConstructorParamsInfo( nConstructor );
		uint32 nParamSize = pParamInfo->nTotalParamSize;
		uint32 nParamCount = pParamInfo->nParamCount;
		size_t nArgSize = nParamCount * sizeof( void* );
		char* pDataBuf = (char*)alloca( nParamSize + nArgSize );
		void** pArgArray = (void**)(pDataBuf + nParamSize);

		JSValue undefined = JS_UNDEFINED;
		for( uint32 nParamIndex = 0; nParamIndex < nParamCount; nParamIndex++ )
		{
			UPakDataType Type = pParamInfo->GetParamType()[nParamIndex];
			CJSTypeBase* pParamType = GetJSTypeBase( Type );
			JSValue arg = (int32)nParamIndex < argc ? 
				argv[nParamIndex] : undefined;
			pParamType->FromVMValue( Type, pContext, pDataBuf, arg );
			pArgArray[nParamIndex] = 
				Type.IsClassValue() ? *(void**)pDataBuf : pDataBuf;
			pDataBuf += pParamInfo->GetParamSize()[nParamIndex];
		}

		void* pObject = new tbyte[pClass->GetClassSize()];
		pClass->Construct( nConstructor, pObject, pArgArray );
		Script.BindObj( pObject, this_val, pClass, true );
		return JS_DupValue( pContext, this_val );
	}

	void CScriptJS::ClassFinalizer( JSRuntime* pRuntime, JSValue val )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetRuntimeOpaque( pRuntime );
		SJSObjInfo* pInfo = nullptr;
		JS_GetClassID( val, (void**)&pInfo );
		if( !pInfo )
			return;
		Script.UnbindObj( pInfo, true );
	}

	JSValue CScriptJS::CallFromVM( JSContext* pContext, JSValue this_val, 
		int argc, JSValue* argv, int32 nMagic, uint16 nIndex )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );


		auto& CallInfo = Script.GetCallInfo( { nIndex, (uint16)nMagic } );
		if( CallInfo.bDestructor )
		{
			SJSObjInfo* pInfo = nullptr;
			JS_GetClassID( this_val, (void**)&pInfo );
			if( pInfo )
				Script.UnbindObj( pInfo, false );
			return this_val;
		}

		auto pCall = CallInfo.pField;
		JSValue Result = JS_UNDEFINED;

		try
		{
			uint32 nParamSize = pCall->GetParamTotalSize();
			uint32 nParamCount = pCall->GetParamCount();
			UPakDataType ResultType = pCall->GetResultType();
			size_t nReturnSize = ResultType.IsVoid() ?
				sizeof( int64 ) : pCall->GetResultSize();
			size_t nArgSize = nParamCount * sizeof( void* );
			size_t nTotalSize = nParamSize + nArgSize + nReturnSize;
			char* pDataBuf = (char*)alloca( nTotalSize );
			void** pArgArray = (void**)(pDataBuf + nParamSize);
			char* pResultBuf = pDataBuf + nParamSize + nArgSize;

			JSValue undefined = JS_UNDEFINED;
			int32 nArgIndex = pCall->GetType() >= eFDT_ClassFunction ? -1 : 0;
			for( uint32 i = 0; i < nParamCount; i++, nArgIndex++ )
			{
				UPakDataType Type = pCall->GetParamType( i );
				CJSTypeBase* pParamType = GetJSTypeBase( Type );
				JSValue arg = undefined;
				if( nArgIndex < 0 )
					arg = this_val;
				else if( nArgIndex < argc )
					arg = argv[nArgIndex];
				pParamType->FromVMValue( Type, pContext, pDataBuf, arg );
				pArgArray[i] = Type.IsClass() && !Type.GetMulPointer()
					? *(void**)pDataBuf : pDataBuf;
				pDataBuf += pCall->GetParamSize( i );
			}

			pCall->Call( pResultBuf, pArgArray );
			if( ResultType.IsVoid() )
				return Result;
			CJSTypeBase* pReturnType = GetJSTypeBase( ResultType );
			Result = pReturnType->ToVMValue( ResultType, pContext, pResultBuf );
			if( ResultType.IsClassValue() )
			{
				auto pClass = ResultType.GetClass();
				pClass->Destruct( pResultBuf );
			}
		}
		catch( ... )
		{
		}
		return Result;
	}

	JSValue CScriptJS::CallFromVM( JSContext* pContext,
		JSValue this_val, int32 nMagic, uint16 nIndex )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		auto& CallInfo = Script.GetCallInfo( { nIndex, (uint16)nMagic } );
		const CField* pField = CallInfo.pField;
		JSValue Result = JS_NULL;


		try
		{
			void* pObject = nullptr;
			UPakDataType ThisType = pField->GetParamType( 0 );
			CJSObject::GetInst().FromVMValue( 
				ThisType, pContext, (char*)&pObject, this_val );
			if( pObject == nullptr )
				return Result;

			UPakDataType DataType = pField->GetResultType();
			assert( !DataType.IsVoid() );
			size_t nDataSize = DataType.IsReference()
				? sizeof( void* ) : pField->GetResultSize();
			char* pDataBuf = (char*)alloca( nDataSize );

			pField->GetData( pObject, pDataBuf );

			if( DataType.IsClassValue() )
			{
				CJSTypeBase* pDataType = GetJSTypeBase( DataType );
				Result = pDataType->ToVMValue( DataType, pContext, pDataBuf );
				auto pClass = DataType.GetClass();
				pClass->Destruct( pDataBuf );
			}
			else 
			{ 
				if( DataType.IsReference() )
				{
					if( DataType.IsClass() )
					{
						CJSTypeBase* pDataType = GetJSTypeBase( DataType );
						return pDataType->ToVMValue( DataType, pContext, pDataBuf );
					}
					pDataBuf = *(char**)pDataBuf;
					DataType = UPakDataType( DataType, DataType.GetMulPointer() );
				}
				CJSTypeBase* pDataType = GetJSTypeBase( DataType );
				Result = pDataType->ToVMValue( DataType, pContext, pDataBuf );
			}
		}
		catch( ... )
		{
		}
		return Result;
	}

	JSValue CScriptJS::CallFromVM( JSContext* pContext,
		JSValue this_val, JSValue val, int32 nMagic, uint16 nIndex )
	{
		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		auto& CallInfo = Script.GetCallInfo( { nIndex, (uint16)nMagic } );
		const CField* pField = CallInfo.pField;


		try
		{
			void* pObject = nullptr;
			UPakDataType ThisType = pField->GetParamType( 0 );
			CJSObject::GetInst().FromVMValue( 
				ThisType, pContext, (char*)&pObject, this_val );
			if( pObject == nullptr )
				return JS_NULL;

			UPakDataType DataType = pField->GetResultType();
			bool bIsClass = DataType.IsClass();
			auto nMulPointer = DataType.GetMulPointer();
			auto bClassValue = !nMulPointer && bIsClass;
			// If class member is a value-type class object, add reference to avoid construction overhead
			DataType.SetReference( bClassValue );

			assert( !DataType.IsVoid() );
			size_t nDataSize = DataType.IsClassValue() ?
				sizeof( void* ) : pField->GetResultSize();
			char* pDataBuf = (char*)alloca( nDataSize );

			if( !bClassValue )
			{
				CJSTypeBase* pDataType = GetJSTypeBase( DataType );
				pDataType->FromVMValue( DataType, pContext, pDataBuf, val );
				pField->SetData( pObject, pDataBuf );
			}
			else
			{
				DataType = UPakDataType( DataType, 1 );
				CJSTypeBase* pDataType = GetJSTypeBase( DataType );
				pDataType->FromVMValue( DataType, pContext, pDataBuf, val );
				pField->SetData( pObject, *(void**)pDataBuf );
			}
		}
		catch( ... )
		{
		}
		return JS_NULL;
	}

	SCallIndex CScriptJS::AddCallInfo( const CField* pField )
	{
		uint32 nCurCount = (uint32)m_listCallFromVM.size();
		uint16 nFunctionIndex = (uint16)( nCurCount >> 16 );
		SCallIndex Index = { nFunctionIndex, (uint16)nCurCount };
		SJSCallInfo& Info = m_listCallInfoMap[pField];
		Info.bDestructor = false;
		Info.pField = pField;
		Info.Name = JS_NewAtom( m_pContext, pField->GetName().c_str() );
		m_listCallFromVM.push_back( &Info );
		return Index;
	}

	SCallIndex CScriptJS::AddCallInfo( const CClass* pClass )
	{
		uint32 nCurCount = (uint32)m_listCallFromVM.size();
		uint16 nFunctionIndex = (uint16)( nCurCount >> 16 );
		SCallIndex Index = { nFunctionIndex, (uint16)nCurCount };
		SJSCallInfo& Info = m_listCallInfoMap[pClass];
		Info.bDestructor = true;
		Info.pClass = pClass;
		Info.Name = JS_NewAtom( m_pContext, pClass->GetClassName().c_str() );
		m_listCallFromVM.push_back( &Info );
		return Index;
	}

	SJSCallInfo& CScriptJS::GetCallInfo( SCallIndex Index )
	{
		uint32 nCurCount = ( Index.nFunIndex << 16 ) | Index.nMagic;
		assert( nCurCount < m_listCallFromVM.size() );
		return *m_listCallFromVM[nCurCount];
	}

	SJSCallInfo* CScriptJS::GetCallInfo( const CField* pField )
	{
		auto it = m_listCallInfoMap.find( pField );
		return it != m_listCallInfoMap.end() ? &it->second : nullptr;
	}

	SJSObjInfo* CScriptJS::AllocObjectInfo()
	{
		if( !m_pFreeObjectInfo )
		{
			SJSObjInfo* aryObjectInfo = new SJSObjInfo[512];
			m_pFreeObjectInfo = aryObjectInfo;
			m_pFreeObjectInfo->m_bFirstAddress = true;
			for( size_t i = 0; i < 511; i++, aryObjectInfo++ )
			{
				aryObjectInfo->m_pObject = aryObjectInfo + 1;
				aryObjectInfo->m_bFirstAddress = false;
			}
			aryObjectInfo->m_pObject = nullptr;
			aryObjectInfo->m_bFirstAddress = false;
		}
		SJSObjInfo* pObjectInfo = m_pFreeObjectInfo;
		m_pFreeObjectInfo = (SJSObjInfo*)m_pFreeObjectInfo->m_pObject;
		return pObjectInfo;
	}

	void CScriptJS::FreeObjectInfo( SJSObjInfo* pObjectInfo )
	{
		pObjectInfo->m_Object = 0;
		pObjectInfo->m_pObject = m_pFreeObjectInfo;
		m_pFreeObjectInfo = pObjectInfo;
	}

	void CScriptJS::BuildRegisterInfo()
	{
		JSClassDef ClassDef =
		{
			"CScriptJSNativeClass",
			&CScriptJS::ClassFinalizer,
			nullptr,
			&CScriptJS::NewObject,
			nullptr
		};

		m_ClassID = 0;
		m_ClassID = JS_NewClassID( &m_ClassID );
		JS_NewClass( m_pRunTime, m_ClassID, &ClassDef );
		JS_SetClassProto( m_pContext, m_ClassID, JS_NewObject( m_pContext ) );
		auto globalObj = JS_GetGlobalObject( m_pContext );
		auto Function = JS_GetPropertyStr( m_pContext, globalObj, "Function" );
		m_FunctionPrototype = JS_GetProperty( m_pContext, Function, m_Prototype );
		JS_FreeValue( m_pContext, Function );

		std::array<SFunctions, 256> s_aryCallByVM;
		SFunctions::Build<255>( s_aryCallByVM );
		std::function<void( const CClass*, JSValue, JSValue, bool )> funRegister;

		funRegister = [&]( const CClass* pClass,
			JSValue NewClass, JSValue Prototype, bool bBase )->void
		{
			for( auto& fieldPair : pClass->GetRegistFields() )
			{
				auto pField = fieldPair.second;
				const char* szFunName = pField->GetName().c_str();
				auto Index = AddCallInfo( pField );
				auto& CallInfo = GetCallInfo( Index );
				auto atomName = CallInfo.Name;
				assert( Index.nFunIndex < std::size( s_aryCallByVM ) );
				auto funCallByVM = s_aryCallByVM[Index.nFunIndex];

				if( pField->GetType() == eFDT_Member )
				{
					auto Getter = JS_NewCFunctionMagic( m_pContext, 
						(JSCFunctionMagic*)funCallByVM.funGetter,
						nullptr, 0, JS_CFUNC_getter_magic, Index.nMagic );
					auto Setter = JS_NewCFunctionMagic( m_pContext, 
						(JSCFunctionMagic*)funCallByVM.funSetter,
						nullptr, 0, JS_CFUNC_setter_magic, Index.nMagic );
					JS_DefinePropertyGetSet( m_pContext, 
						Prototype, atomName, Getter, Setter, 0 );
				}
				else if( pField->GetType() == eFDT_ClassStaticFunction )
				{
					auto StaticFunction = JS_NewCFunctionMagic( m_pContext, 
						(JSCFunctionMagic*)funCallByVM.funCall,
						nullptr, 0, JS_CFUNC_generic_magic, Index.nMagic );
					JS_SetProperty( m_pContext, NewClass, atomName, StaticFunction );
				}
				else
				{
					auto StaticFunction = JS_NewCFunctionMagic( m_pContext, 
						(JSCFunctionMagic*)funCallByVM.funCall,
						nullptr, 0, JS_CFUNC_generic_magic, Index.nMagic );
					JS_SetProperty( m_pContext, Prototype, atomName, StaticFunction );
				}
			}
			if( !bBase )
				return;

			uint32 nBaseCount = pClass->GetBaseClassCount();
			for( uint32 i = 0; i < nBaseCount; i++ )
			{
				auto pBaseClass = pClass->GetBaseClass( i );
				funRegister( pBaseClass, NewClass, Prototype, true );
			}
		};

		std::map<const CClass*, JSValue> setRegistedClass;
		auto CheckClassInfo = [&]( const CClass* pClass )->JSValue
		{
			auto itClass = setRegistedClass.find( pClass );
			if( itClass != setRegistedClass.end() )
				return itClass->second;

			auto scopeObj = GetClassScope( pClass );
			auto Class = JS_NewObjectClass( m_pContext, m_ClassID );
			auto FunctiontProtoType = JS_DupValue( m_pContext, m_FunctionPrototype );
			auto szName = pClass->GetClassName().c_str();
			JS_SetConstructorBit( m_pContext, Class, true );
			JS_SetOpaque( Class, (void*)pClass );
			JS_SetPropertyStr( m_pContext, scopeObj, szName, Class );
			JS_SetProperty( m_pContext, Class, m___proto__, FunctiontProtoType );
			JS_SetProperty( m_pContext, Class, m_Prototype, JS_NewObject( m_pContext ) );
			return setRegistedClass[pClass] = Class;
		};

		for( auto& pair : CReflection::GetInstance().GetRegistClassMap() )
		{
			auto pClass = pair.second;
			if( pClass->IsEnum() )
				continue;

			if( pClass->GetRegisterType() == eCRT_GlobalScope )
			{
				auto scopeObj = GetClassScope( pClass );
				for( auto& fieldPair : pClass->GetRegistFields() )
				{
					auto pField = fieldPair.second;
					auto Index = AddCallInfo( pField );
					auto& CallInfo = GetCallInfo( Index );
					auto atomName = CallInfo.Name;
					assert( Index.nFunIndex < std::size( s_aryCallByVM ) );
					auto funCallByVM = s_aryCallByVM[Index.nFunIndex];
					auto Function = JS_NewCFunctionMagic( m_pContext,
						(JSCFunctionMagic*)funCallByVM.funCall,
						nullptr, 0, JS_CFUNC_generic_magic, Index.nMagic );
					JS_SetProperty( m_pContext, scopeObj, atomName, Function );
				}
				continue;
			}

			JSValue Base = 0;
			if( pClass->GetBaseClassCount() )
			{
				auto pBaseClass = pClass->GetBaseClass( 0 );
				Base = CheckClassInfo( pBaseClass );
			}

			auto NewClass = CheckClassInfo( pClass );
			auto szClass = pClass->GetClassName().c_str();
			JSValue args[] = { NewClass, JS_NULL, Base };
			JS_Call( m_pContext, m_objClass, globalObj, 3, args );
			auto Prototype = JS_GetProperty( m_pContext, NewClass, m_Prototype );

			auto Index = AddCallInfo( pClass );
			auto& CallInfo = GetCallInfo( Index );
			assert( Index.nFunIndex < std::size( s_aryCallByVM ) );
			auto funCallByVM = s_aryCallByVM[Index.nFunIndex];
			auto Destruction = JS_NewCFunctionMagic( m_pContext,
				(JSCFunctionMagic*)funCallByVM.funCall,
				szClass, 0, JS_CFUNC_generic_magic, Index.nMagic );
			JS_SetProperty( m_pContext, Prototype, m_Deconstruction, Destruction );
			for( uint32 i = 1; i < pClass->GetBaseClassCount(); i++ )
				funRegister( pClass->GetBaseClass( i ), NewClass, Prototype, true );
			funRegister( pClass, NewClass, Prototype, false );
		}

		JS_FreeValue( m_pContext, globalObj );
	}

	bool CScriptJS::OnCallBack( 
		const CField* pField, void* pRetBuf, void** pArgArray )
	{

		SJSCallInfo* pInfo = GetCallInfo( pField );
		void* pObject = *(void**)*pArgArray++;
		UPakDataType ThisType = pField->GetParamType( 0 );
		auto objThis = CJSObject::GetInst().ToVMValue(
			ThisType, m_pContext, (char*)&pObject );

		uint32 nParamCount = pField->GetParamCount() - 1;
		size_t nTotalSize = sizeof( JSValue ) * nParamCount;
		JSValue* args = (JSValue*)alloca( nTotalSize );
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
		{
			UPakDataType Type = pField->GetParamType( nArgIndex + 1 );
			CJSTypeBase* pParamType = GetJSTypeBase( Type );
			auto pDataBuf = Type.IsClassReference()
				? (char*)&pArgArray[nArgIndex] : (char*)pArgArray[nArgIndex];
			args[nArgIndex] = pParamType->ToVMValue( Type, m_pContext, pDataBuf );
		}

		auto fun = JS_GetProperty( m_pContext, objThis, pInfo->Name );
		if( !JS_IsFunction( m_pContext, fun ) )
			return false;

		auto result = JS_Call( m_pContext, fun, objThis, nParamCount, args );
		UPakDataType ResultType = pField->GetResultType();
		if( !ResultType.IsVoid() )
		{
			CJSTypeBase* pReturnType = GetJSTypeBase( ResultType );
			if( !ResultType.IsClassValue() )
			{
				pReturnType->FromVMValue(
					ResultType, m_pContext, (char*)pRetBuf, result );
			}
			else
			{
				void* pObject = nullptr;
				pReturnType->FromVMValue( 
					ResultType, m_pContext, (char*)&pObject, result );
				auto pClass = ResultType.GetClass();
				pClass->Assign( pRetBuf, pObject );
			}
		}

		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
			JS_FreeValue( m_pContext, args[nArgIndex] );
		JS_FreeValue( m_pContext, fun );
		JS_FreeValue( m_pContext, objThis );
		JS_FreeValue( m_pContext, result );
		return true;
	}

	void CScriptJS::OnDestruct( const CField* pField, void* pObject )
	{
	}
	
	void* CScriptJS::BindObj( void* pObject, 
		JSValue ScriptObj, const CClass* pClass, bool bRecycle )
	{
		if( !pObject )
			return nullptr;

		if( pClass == nullptr )
		{
			assert( pObject == (void*)ScriptObj );
			auto pObjInfo = FindExistObjInfo( pObject );
			if( !pObjInfo )
			{
				pObjInfo = AllocObjectInfo();
				pObjInfo->m_bRecycle = bRecycle;
				pObjInfo->m_Object = ScriptObj;
				pObjInfo->m_pClass = pClass;
				pObjInfo->m_pObject = pObject;
				m_mapObjInfo[pObjInfo->m_pObject] = *pObjInfo;
			}
			return pObjInfo->m_pObject;
		}

		SJSObjInfo& ObjectInfo = *AllocObjectInfo();
		ObjectInfo.m_bRecycle = bRecycle;
		ObjectInfo.m_Object = ScriptObj;
		ObjectInfo.m_pClass = pClass;
		ObjectInfo.m_pObject = pObject;
		m_mapObjInfo[ObjectInfo.m_pObject] = ObjectInfo;

		// Register callback function
		if( pClass && pClass->GetOverridableCount() )
			pClass->HookVirtualTable( pObject, this );
		JS_SetOpaque( ScriptObj, &ObjectInfo );
		return pObject;
	}

	void CScriptJS::UnbindObj( SJSObjInfo* pObjectInfo, bool bFromGC )
	{
		void* pObject = pObjectInfo->m_pObject;
		bool bRecycle = pObjectInfo->m_bRecycle;


		if( !bFromGC )
			JS_SetOpaque( pObjectInfo->m_Object, nullptr );

		m_mapObjInfo.erase( pObjectInfo->m_pObject );
		pObjectInfo->m_Object = 0;
		pObjectInfo->m_pObject = nullptr;

		FreeObjectInfo( pObjectInfo );
		if( !pObject || !pObjectInfo->m_pClass )
			return;
		const CClass* pClass = pObjectInfo->m_pClass;
		pClass->UnhookVirtualTable( pObject );
		if( !bRecycle )
			return;
		pClass->Destruct( pObject );
		delete[]( tbyte* )pObject;
	}

	SJSObjInfo* CScriptJS::FindExistObjInfo( void* pObj )
	{
		if( pObj == nullptr )
			return nullptr;

		auto itRight = m_mapObjInfo.upper_bound( pObj );
		if( itRight == m_mapObjInfo.begin() )
			return nullptr;

		auto itLeft = itRight != m_mapObjInfo.end()
			? std::prev( itRight )
			: std::prev( m_mapObjInfo.end() );
		SJSObjInfo* pLeft = &itLeft->second;
		ptrdiff_t nDiff = ((const char*)pObj) - ((const char*)pLeft->m_pObject);
		if( pLeft->m_pClass == nullptr )
			return nDiff ? nullptr : pLeft;
		if( nDiff >= (ptrdiff_t)(pLeft->m_pClass->GetClassSize()) )
			return nullptr;
		return pLeft;
	}

	JSValue CScriptJS::GetClassScope( const CClass* pClass )
	{
		auto scopeObj = JS_GetGlobalObject( m_pContext );
		for( uint8 i = 0; i < pClass->GetNameSpaceCount(); i++ )
		{
			auto szNameSpace = pClass->GetNameSpace( i ).c_str();
			auto subScope = JS_GetPropertyStr( m_pContext, scopeObj, szNameSpace );
			if( subScope == JS_NULL || subScope == JS_UNDEFINED )
			{
				subScope = JS_NewObject( m_pContext );
				JS_SetPropertyStr( m_pContext, scopeObj, szNameSpace, subScope );
				subScope = JS_DupValue( m_pContext, subScope );
			}
			JS_FreeValue( m_pContext, scopeObj );
			scopeObj = subScope;
		}
		JS_FreeValue( m_pContext, scopeObj );
		return scopeObj;
	}

	//===========================================================
	// External interface                                      
	//===========================================================
	bool CScriptJS::Evaluate( const char* szExpression,
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		return false;
	}

	bool CScriptJS::Set( void* pObject, int32 nIndex, 
		void* pArgBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ParamType( TypeInfo );
		if( ParamType.IsVoid() || !pArgBuf )
			return false;


		JSValue objObject = JS_UNDEFINED;
		if( pObject == nullptr )
			objObject = JS_GetGlobalObject( m_pContext );
		else if( auto pInfo = FindExistObjInfo( pObject ) )
			objObject = pInfo->m_Object;
		if( objObject == JS_UNDEFINED )
			return false;

		CJSTypeBase* pParamType = GetJSTypeBase( ParamType );
		JSValue objArg = pParamType->ToVMValue(
			ParamType, m_pContext, (char*)pArgBuf );
		JS_SetPropertyUint32( m_pContext, objObject, nIndex, objArg );
		if( pObject == nullptr )
			JS_FreeValue( m_pContext, objObject );
		JS_FreeValue( m_pContext, objArg );
		return true;
	}

	bool CScriptJS::Set( void* pObject, const char* szName, 
		void* pArgBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ParamType( TypeInfo );
		if( ParamType.IsVoid() || !pArgBuf )
			return false;


		JSValue objObject = JS_UNDEFINED;
		if( pObject == nullptr )
			objObject = JS_GetGlobalObject( m_pContext );
		else if( auto pInfo = FindExistObjInfo( pObject ) )
			objObject = pInfo->m_Object;
		if( objObject == JS_UNDEFINED )
			return false;

		CJSTypeBase* pParamType = GetJSTypeBase( ParamType );
		JSValue objArg = pParamType->ToVMValue( 
			ParamType, m_pContext, (char*)pArgBuf );
		JS_SetPropertyStr( m_pContext, objObject, szName, objArg );
		if( pObject == nullptr )
			JS_FreeValue( m_pContext, objObject );
		JS_FreeValue( m_pContext, objArg );
		return true;
	}

	bool CScriptJS::Get( void* pObject, int32 nIndex, 
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ResultType( TypeInfo );
		if( ResultType.IsVoid() || !pResultBuf )
			return false;


		JSValue objObject = JS_UNDEFINED;
		if( pObject == nullptr )
			objObject = JS_GetGlobalObject( m_pContext );
		else if( auto pInfo = FindExistObjInfo( pObject ) )
			objObject = pInfo->m_Object;
		if( objObject == JS_UNDEFINED )
			return false;

		JSValue objResult = JS_GetPropertyUint32(
			m_pContext, objObject, nIndex );
		GetJSTypeBase( ResultType )->FromVMValue(
			ResultType, m_pContext, (char*)pResultBuf, objResult );
		if( pObject == nullptr )
			JS_FreeValue( m_pContext, objObject );
		JS_FreeValue( m_pContext, objResult );
		return !JS_IsUndefined( objResult );
	}

	bool CScriptJS::Get( void* pObject, const char* szName, 
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ResultType( TypeInfo );
		if( ResultType.IsVoid() || !pResultBuf )
			return false;


		JSValue objObject = JS_UNDEFINED;
		if( pObject == nullptr )
			objObject = JS_GetGlobalObject( m_pContext );
		else if( auto pInfo = FindExistObjInfo( pObject ) )
			objObject = pInfo->m_Object;
		if( objObject == JS_UNDEFINED )
			return false;

		JSValue objResult = JS_GetPropertyStr( 
			m_pContext, objObject, szName );
		GetJSTypeBase( ResultType )->FromVMValue(
			ResultType, m_pContext, (char*)pResultBuf, objResult );
		if( pObject == nullptr )
			JS_FreeValue( m_pContext, objObject );
		JS_FreeValue( m_pContext, objResult );
		return !JS_IsUndefined( objResult );
	}

	bool CScriptJS::Call( const STypeInfoArray& aryTypeInfo,
		void* pResultBuf, const char* szFunction, void** aryArg )
	{
		JSValue args[256];
		uint32 nParamCount = aryTypeInfo.nInfoCount - 1;
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
		{
			UPakDataType nType( aryTypeInfo.aryInfo[nArgIndex] );
			CJSTypeBase* pParamType = GetJSTypeBase( nType );
			args[nArgIndex] = pParamType->ToVMValue( 
				nType, m_pContext, (char*)aryArg[nArgIndex] );
		}

		JSValue classObject = JS_GetGlobalObject( m_pContext );
		const char* szFunName = strrchr( szFunction, '.' );
		if( szFunName )
		{
			std::string szClass;
			szClass = std::string( szFunction, szFunName - szFunction );
			auto childObject = JS_GetPropertyStr( 
				m_pContext, classObject, szClass.c_str() );
			JS_FreeValue( m_pContext, classObject );
			classObject = childObject;
			szFunction = szFunName + 1;
		}

		JSValue funEval = JS_GetPropertyStr( m_pContext, classObject, szFunction );
		if( !JS_IsFunction( m_pContext, funEval ) )
			return false;

		JSValue Result = JS_Call( m_pContext, funEval, classObject, nParamCount, args );
		UPakDataType ResultType( aryTypeInfo.aryInfo[nParamCount] );
		if( !ResultType.IsVoid() && pResultBuf )
			GetJSTypeBase( ResultType )->FromVMValue( 
				ResultType, m_pContext, (char*)pResultBuf, Result );
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
			JS_FreeValue( m_pContext, args[nArgIndex] );
		JS_FreeValue( m_pContext, funEval );
		JS_FreeValue( m_pContext, classObject );
		JS_FreeValue( m_pContext, Result );
		return true;
	}

	bool CScriptJS::Call( const STypeInfoArray& aryTypeInfo,
		void* pResultBuf, void* pFunction, void** aryArg )
	{
		auto pInfo = FindExistObjInfo( pFunction );
		if( !pInfo || !JS_IsFunction( m_pContext, pInfo->m_Object ) )
			return false;
		JSValue funEval = pInfo->m_Object;
		JSValue args[256];
		uint32 nParamCount = aryTypeInfo.nInfoCount - 1;
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
		{
			UPakDataType nType( aryTypeInfo.aryInfo[nArgIndex] );
			CJSTypeBase* pParamType = GetJSTypeBase( nType );
			args[nArgIndex] = pParamType->ToVMValue(
				nType, m_pContext, (char*)aryArg[nArgIndex] );
		}

		JSValue classObject = JS_GetGlobalObject( m_pContext );
		JSValue Result = JS_Call( m_pContext, funEval, classObject, nParamCount, args );
		UPakDataType ResultType( aryTypeInfo.aryInfo[nParamCount] );
		if( !ResultType.IsVoid() && pResultBuf )
			GetJSTypeBase( ResultType )->FromVMValue(
				ResultType, m_pContext, (char*)pResultBuf, Result );
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
			JS_FreeValue( m_pContext, args[nArgIndex] );
		JS_FreeValue( m_pContext, classObject );
		JS_FreeValue( m_pContext, Result );
		return true;
	}

	bool CScriptJS::RunString( const void* pBuffer, size_t nSize, 
		const char* szFileName, bool bForceBuild )
	{
		std::string sFileName = szFileName;
		if( sFileName[0] == '/' )
			sFileName = "file://" + sFileName;
		else if( sFileName[1] == ':' )
			sFileName = "file:///" + sFileName;
		else
			sFileName = "memory:/" + sFileName;

		JS_RunGC( m_pRunTime );
		auto Result = JS_Eval( m_pContext, (const char*)pBuffer, 
			nSize, sFileName.c_str(), JS_EVAL_TYPE_GLOBAL );
		bool bSucceeded = !JS_IsException( Result );
		if( !bSucceeded )
		{
			JSValue exception_val = JS_GetException( m_pContext );
			const char* str = JS_ToCString( m_pContext, exception_val );
			printf( "%s", str );
			if( JS_IsError( m_pContext, exception_val ) )
			{
				auto val = JS_GetPropertyStr( m_pContext, exception_val, "stack" );
				if( !JS_IsUndefined( val ) ) 
				{
					const char* str = JS_ToCString( m_pContext, val );
					printf( "%s", str );
				}
				JS_FreeValue( m_pContext, val );
			}
			JS_FreeValue( m_pContext, exception_val );
		}
		JS_FreeValue( m_pContext, Result );
		return bSucceeded;
	}

	//===================================================================================
	// External functions
	//===================================================================================	
	void* CScriptJS::GetVM()
	{
		return m_pContext;
	}

	int32 CScriptJS::IncRef( void* pObj )
	{
		auto pInfo = FindExistObjInfo( pObj );
		if( pInfo == nullptr || JS_IsNull( pInfo->m_Object ) )
			return false;
		JS_DupValue( m_pContext, pInfo->m_Object );
		return true;
	}

	int32 CScriptJS::DecRef( void* pObj )
	{
		auto pInfo = FindExistObjInfo( pObj );
		if( pInfo == nullptr || JS_IsNull( pInfo->m_Object ) )
			return false;
		JS_FreeValue( m_pContext, pInfo->m_Object );
		return true;
	}

	void CScriptJS::UnlinkCppObjFromScript( void* pObj )
	{
		auto pObjInfo = FindExistObjInfo( pObj );
		if( !pObjInfo )
			return;
		// Only unbind here
		m_mapObjInfo.erase( pObjInfo->m_pObject );
		pObjInfo->m_pObject = nullptr;
	}

	bool CScriptJS::IsValid( void* pObject )
	{
		auto pInfo = FindExistObjInfo( pObject );
		return pInfo && !JS_IsNull( pInfo->m_Object );
	}

	void CScriptJS::GC()
	{
		JS_RunGC( m_pRunTime );
	}

	void CScriptJS::GCAll()
	{
		JS_RunGC( m_pRunTime );
	}
    

	SFunctionTable* CScriptJS::FindHookVirtualTable( 
		const CClass* pClass, SFunctionTable* pOldFunTable )
	{
		auto& mapOld2New = m_mapVirtualTableOld2NewOfClass[pClass];
		auto it = mapOld2New.find( pOldFunTable );
		if( it != mapOld2New.end() )
			return it->second;
		return nullptr;
	}

	void CScriptJS::SetHookVirtualTable( const CClass* pClass, 
		SFunctionTable* pOldFunTable, SFunctionTable* pNewFunTable )
	{
		auto& mapOld2New = m_mapVirtualTableOld2NewOfClass[pClass];
		assert( mapOld2New.find( pOldFunTable ) == mapOld2New.end() );
		mapOld2New[pOldFunTable] = pNewFunTable;
	}
};
