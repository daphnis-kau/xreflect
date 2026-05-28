/**@file  		CTypeJS.h
* @brief		Data interface between V8&c++
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __TYPE_JS_H__
#define __TYPE_JS_H__
#include <cstdint>
#include <cstddef>
#include "CScriptJS.h"

namespace ScriptBinding
{
	using namespace XReflect;
	#define MAX_UNKNOW_ARRAYBUFFER_SIZE	(100*1024*1024)

	class CJSTypeBase;
	//=====================================================================
	/// Base class of data type
	//=====================================================================
	class CJSTypeBase
	{
	public:
		CJSTypeBase() {}
		virtual const char* GetTypeName() = 0;
		virtual void FromVMValue( UPakDataType Type,
			JSContext* pContext, char* pDataBuf, JSValue obj ) = 0;
			virtual JSValue ToVMValue( UPakDataType Type,
				JSContext* pContext, char* pDataBuf ) = 0;
	};
	
	//=====================================================================
	/// Common class of data type
	//=====================================================================
	template<typename T>
	class TJSValue : public CJSTypeBase
	{
	public:
		const char* GetTypeName()
		{
			return TypeName<T>();
		}

		void FromVMValue( UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
		{
			int64 nResult = 0;
			JS_ToInt64Ext( pContext, &nResult, obj );
			*(T*)(pDataBuf) = (T)nResult;
		}

		JSValue ToVMValue( UPakDataType Type, JSContext* pContext, char* pDataBuf)
		{
			if( std::is_unsigned<T>::value )
			{
				if( *(T*)(pDataBuf) < 0x7fffffff )
					return JS_NewInt32( pContext, (int32)*(T*)(pDataBuf) );
				return JS_NewBigUint64( pContext, (uint64)*(T*)(pDataBuf) );
			}
			else
			{
				if( *(T*)(pDataBuf) < 0x7fffffff && *(T*)(pDataBuf) >= (int32)0x80000000 )
					return JS_NewInt32( pContext, (int32)*(T*)(pDataBuf) );
				return JS_NewBigInt64( pContext, (int64)*(T*)(pDataBuf) );
			}
		}

		static TJSValue<T>& GetInst()
		{
			static TJSValue<T> s_Instance;
			return s_Instance;
		}
	};

	// POD type class specialization
	template<> inline void TJSValue<double>::FromVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
	{
		JS_ToFloat64( pContext, (double*)(pDataBuf), obj );
	}

	template<> inline JSValue TJSValue<double>::ToVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf)
	{
		return JS_NewFloat64( pContext, *(double*)(pDataBuf) );
	}

	template<> inline void TJSValue<float>::FromVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
	{
		double fResult = 0;
		JS_ToFloat64( pContext, &fResult, obj );
		*(float*)(pDataBuf) = (float)fResult;
	}

	template<> inline JSValue TJSValue<float>::ToVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf)
	{
		return JS_NewFloat64( pContext, *(float*)(pDataBuf) );
	}

	template<> inline void TJSValue<bool>::FromVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
	{
		*(bool*)(pDataBuf) = JS_ToBool( pContext, obj ) != 0;
	}

	template<> inline JSValue TJSValue<bool>::ToVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf)
	{
		return JS_NewBool( pContext, *(bool*)(pDataBuf) );
	}

	template<> inline void TJSValue<void*>::FromVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
	{		
		if( JS_IsNull( obj ) || JS_IsUndefined( obj ) )
		{
			*(void**)( pDataBuf ) = nullptr;
			return;
		}

		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		auto nClassID = JS_GetClassID( obj, nullptr );
		if( Script.IsBufferClass( nClassID ) )
		{
			size_t nSize = 0;
			*(void**)(pDataBuf) = JS_GetArrayBuffer( pContext, &nSize, obj );
			return;
		}

		void* pObj = Script.BindObj( (void*)obj, obj, nullptr, false );
		*(void**)( pDataBuf ) = pObj;
	}

	template<> inline JSValue TJSValue<void*>::ToVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf )
	{
		void* pObj = *(void**)(pDataBuf);
		if( !pObj )
			return JS_NULL;

		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		const SJSObjInfo* pObjInfo = nullptr;
		if ( ( pObjInfo = Script.FindExistObjInfo( pObj ) ) != nullptr &&
			pObjInfo->m_pClass == nullptr )
			return JS_DupValue( pContext, pObjInfo->m_Object );

		return JS_NewArrayBuffer( pContext,
			(uint8_t*)pObj, MAX_UNKNOW_ARRAYBUFFER_SIZE,
			[]( JSRuntime* rt, void* opaque, void* ptr ) {}, nullptr, true );
	}

	template<> inline void TJSValue<const char*>::FromVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf, JSValue obj)
	{
		*(const char**)(pDataBuf) = JS_ToCString( pContext, obj );
	}

	template<> inline JSValue TJSValue<const char*>::ToVMValue(
		UPakDataType Type, JSContext* pContext, char* pDataBuf)
	{
		return JS_NewString( pContext, *(const char**)(pDataBuf) );
	}

	//=====================================================================
	/// Interface of class pointer
	//=====================================================================
	class CJSObject : public TJSValue<void*>
	{
	protected:
		const CClass* _FromVMValue( UPakDataType Type, 
			JSContext* pContext, char* pDataBuf, JSValue obj);
		JSValue _ToVMValue( UPakDataType Type, 
			JSContext* pContext, void* pObj, bool bCopy);
	public:
		CJSObject();

		static CJSObject& GetInst();
		virtual void FromVMValue( UPakDataType Type, 
			JSContext* pContext, char* pDataBuf, JSValue obj);
		virtual JSValue ToVMValue( UPakDataType Type, 
			JSContext* pContext, char* pDataBuf);
	};

	//=====================================================================
	/// Interface of class value
	//=====================================================================
	class CJSValueObject : public CJSObject
	{
	public:
		CJSValueObject();
		static CJSValueObject& GetInst();
		virtual void FromVMValue( UPakDataType Type, 
			JSContext* pContext, char* pDataBuf, JSValue obj);
		virtual JSValue ToVMValue( UPakDataType Type, 
			JSContext* pContext, char* pDataBuf);
	};
}

#endif
