#include "CTypeJS.h"
#include "CScriptJS.h"

namespace ScriptBinding
{
	//=====================================================================
	/// CJSObject
	//=====================================================================
	CJSObject::CJSObject()
	{
	}

	CJSObject& CJSObject::GetInst()
	{
		static CJSObject s_Instance;
		return s_Instance;
	}

	inline const CClass* CJSObject::_FromVMValue( UPakDataType Type,
		JSContext* pContext, char* pDataBuf, JSValue obj )
	{
		auto pExpectClass = Type.GetClass();
		auto& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );

		if( !JS_IsObject( obj ) )
		{
			*(void**)( pDataBuf ) = nullptr;
			return nullptr;
		}

		SJSObjInfo* pInfo = nullptr;
		JS_GetClassID( obj, (void**)&pInfo );
		if( !pInfo || !pInfo->m_pObject )
		{
			*(void**)( pDataBuf ) = nullptr;
			return nullptr;
		}

		const CClass* pObjClass = pInfo->m_pClass;
		if( pObjClass == pExpectClass )
		{
			*(void**)( pDataBuf ) = pInfo->m_pObject;
			return pObjClass;
		}

		int32 nOffset = pObjClass->CalculateBaseOffset( pExpectClass );
		if( nOffset >= 0 )
			*(void**)( pDataBuf ) = ( (char*)pInfo->m_pObject ) + nOffset;
		else if( ( nOffset = pExpectClass->CalculateBaseOffset( pObjClass ) ) >= 0 )
			*(void**)( pDataBuf ) = ( (char*)pInfo->m_pObject ) - nOffset;
		else
			*(void**)( pDataBuf ) = pInfo->m_pObject;
		return pObjClass;
	}

	inline JSValue CJSObject::_ToVMValue( UPakDataType Type, 
		JSContext* pContext, void* pObj, bool bCopy )
	{
		auto pCppClass = Type.GetClass();
		if( !pObj || !pCppClass )
			return JS_NULL;

		CScriptJS& Script = *(CScriptJS*)JS_GetContextOpaque( pContext );
		const SJSObjInfo* pObjInfo = nullptr;
		if( !bCopy && ( pObjInfo = Script.FindExistObjInfo( pObj ) ) != nullptr &&
			pObjInfo->m_pClass->IsA( pCppClass ) )
			return JS_DupValue( pContext, pObjInfo->m_Object );

		auto itCallInfo = Script.m_listCallInfoMap.find( (const void*)pCppClass );
		auto pCallInfo = itCallInfo != Script.m_listCallInfoMap.end() ? &itCallInfo->second : nullptr;
		auto scopeObj = Script.GetClassScope( pCallInfo ? pCallInfo->pClass : nullptr );
		auto classJS = JS_GetProperty( pContext, scopeObj, pCallInfo->Name );
		auto Prototype = JS_GetProperty( pContext, classJS, Script.m_Prototype );
		auto NewObj = JS_NewObjectClass( pContext, Script.m_ClassID );
		JS_SetProperty( pContext, NewObj, Script.m___proto__, Prototype );

		if( !bCopy )
		{
			Script.BindObj( pObj, NewObj, pCppClass, false );
			return NewObj;
		}

		if( !pCppClass->IsCopyConstructible() )
			return JS_NULL;

		void* pNewObject = new tbyte[pCppClass->GetClassSize()];
		pCppClass->Construct( eBCI_CopyConstructor, pNewObject, &pObj  );
		Script.BindObj( pNewObject, NewObj, pCppClass, true );
		return NewObj;
	}

	void CJSObject::FromVMValue( UPakDataType Type, 
		JSContext* pContext, char* pDataBuf, JSValue obj)
	{
		_FromVMValue( Type, pContext, pDataBuf, obj );
	}

	JSValue CJSObject::ToVMValue( UPakDataType Type, 
		JSContext* pContext, char* pDataBuf)
	{
		void* pObj = *(void**)(pDataBuf);
		if (!pObj)
			return JS_NULL;
		return _ToVMValue( Type, pContext, pObj, false );
	}

	//=====================================================================
	/// CJSValueObject
	//=====================================================================
	CJSValueObject::CJSValueObject()
	{
	}

	CJSValueObject& CJSValueObject::GetInst()
	{
		static CJSValueObject s_Instance;
		return s_Instance;
	}

	void CJSValueObject::FromVMValue( UPakDataType Type,
		JSContext* pContext, char* pDataBuf, JSValue obj )
	{
		CJSObject::FromVMValue( Type, pContext, pDataBuf, obj );
	}

	JSValue CJSValueObject::ToVMValue( UPakDataType Type, 
		JSContext* pContext, char* pDataBuf)
	{
		void* pObj = pDataBuf;
		if( !pObj )
			return JS_NULL;
		return _ToVMValue( Type, pContext, pObj, true );
	}
}

