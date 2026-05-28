#include "ReflectionHelp.h"
#include "CField.h"
#include "CClass.h"
#include "CReflection.h"
#include <iostream>

#ifdef _ANDROID
#include <alloca.h>
#endif

namespace XReflect
{
	//=====================================================================
	// Basic interface for script calling C++
	//=====================================================================
	CField::CField( const CFieldTags& Tag, CClass* pClass, const char* szFieldName,
		bool bPureVirtual, FunctionPointer funBoot, int32 nFunIndex, 
		CallbackWrapFun funWrap, const STypeInfoArray& aryTypeInfo )
		: m_pClass( pClass )
		, m_FieldName( szFieldName )
		, m_eFieldType( bPureVirtual ? eFDT_ClassPureCallBack : eFDT_ClassCallBack )
		, m_Tag( Tag )
		, m_nRegisterValue( nFunIndex )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		auto& Callback = m_FieldInfo.m_CallbackInfo;
		Callback.m_funBoot = funBoot;
		Callback.m_CallbackWrap.m_funCallbackWrap = funWrap;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag, CClass* pClass, 
		const char* szFieldName, bool bThisCall,
		CallWrapFun funWrap, const STypeInfoArray& aryTypeInfo )
		: m_pClass( pClass )
		, m_FieldName( szFieldName )
		, m_eFieldType( eFDT_ClassFunction )
		, m_Tag( Tag )
		, m_nRegisterValue( 0 )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		if( pClass->GetRegisterType() == eCRT_GlobalScope )
			m_eFieldType = eFDT_GlobalFunction;
		else if( !bThisCall )
			m_eFieldType = eFDT_ClassStaticFunction;
		else
			m_eFieldType = eFDT_ClassFunction;
		m_FieldInfo.m_funCall = funWrap;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag,
		CClass* pClass, FunctionPointer funBoot, int32 nFunIndex,
		DtorWrapFun funWrap, const STypeInfoArray& aryTypeInfo )
		: m_pClass( pClass )
		, m_eFieldType( eFDT_ClassDestructor )
		, m_Tag( Tag )
		, m_nRegisterValue( nFunIndex )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		const char* strName = pClass->GetClassName().c_str();
		uint64 nNameLen = strlen( strName );
		char* szFieldName = (char*)alloca( nNameLen + 2 );
		szFieldName[0] = '~';
		memcpy( szFieldName + 1, strName, nNameLen );
		szFieldName[nNameLen + 1] = 0;
		m_FieldName = szFieldName;
		auto& Callback = m_FieldInfo.m_CallbackInfo;
		Callback.m_funBoot = funBoot;
		Callback.m_CallbackWrap.m_funDestructorWrap = funWrap;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag, CClass* pClass,
		GetWrapFun funPtrGet, const STypeInfoArray& aryTypeInfo )
		: m_pClass( pClass )
		, m_FieldName( "&*" )
		, m_eFieldType( eFDT_Member )
		, m_Tag( Tag )
		, m_nRegisterValue( 0 )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		assert( !pClass->m_pSmartPtrField );
		m_FieldInfo.m_MemberInfo.m_funGet = funPtrGet;
		m_FieldInfo.m_MemberInfo.m_funSet = nullptr;
		pClass->m_pSmartPtrField = this;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag, CClass* pClass, 
		const char* szFieldName, EMemberQualifier Qualifier, 
		GetWrapFun funGet, SetWrapFun funSet, const STypeInfoArray& aryTypeInfo )
		: m_pClass( pClass )
		, m_FieldName( szFieldName )
		, m_eFieldType( eFDT_Member )
		, m_Tag( Tag )
		, m_nRegisterValue( Qualifier )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		m_FieldInfo.m_MemberInfo.m_funGet = funGet;
		m_FieldInfo.m_MemberInfo.m_funSet = funSet;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag, CClass* pClass,
		const char* szFieldName, EMemberQualifier Qualifier,
		GetWrapFun funGet, SetWrapFun funSet, const STypeInfoArray& aryTypeInfo,
		EFieldType eFieldType )
		: m_pClass( pClass )
		, m_FieldName( szFieldName )
		, m_eFieldType( eFieldType )
		, m_Tag( Tag )
		, m_nRegisterValue( Qualifier )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( aryTypeInfo.nInfoCount - 1 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		m_FieldInfo.m_MemberInfo.m_funGet = funGet;
		m_FieldInfo.m_MemberInfo.m_funSet = funSet;
		Init( aryTypeInfo );
	}

	CField::CField( const CFieldTags& Tag,
		CClass* pClass, const char* szFieldName, int32 nEnumValue )
		: m_pClass( pClass )
		, m_FieldName( szFieldName )
		, m_eFieldType( eFDT_Enumeration )
		, m_Tag( Tag )
		, m_nRegisterValue( nEnumValue )
		, m_nTotalParamSize( 0 )
		, m_nParamCount( 0 )
		, m_aryParamType( nullptr )
		, m_aryParamSize( nullptr )
	{
		STypeInfo TypeInfo = GetTypeInfo<int32>();
		STypeInfoArray aryInfo = { &TypeInfo, 1 };
		Init( aryInfo );
	}

	CField::~CField()
	{
		delete[] m_aryParamType ; m_aryParamType  = nullptr;;
		delete[] m_aryParamSize ; m_aryParamSize  = nullptr;;
	}

	void CField::Init( const STypeInfoArray& aryTypeInfo )
	{
		if( m_nParamCount )
		{
			m_aryParamType = new UPakDataType[m_nParamCount];
			m_aryParamSize = new uint32[m_nParamCount];
		}

		for( size_t i = 0; i < m_nParamCount; i++ )
		{
			m_aryParamType[i] = UPakDataType( aryTypeInfo.aryInfo[i] );
			m_aryParamSize[i] = (uint32)m_aryParamType[i].GetAligenSizeOfType();
			m_nTotalParamSize += m_aryParamSize[i];
		}

		auto& ResultTypeInfo = aryTypeInfo.aryInfo[m_nParamCount];
		m_nResult = UPakDataType( ResultTypeInfo );
		m_nReturnSize = (uint32)m_nResult.GetAligenSizeOfType();

		assert( !m_pClass->m_mapRegistFields.count( m_FieldName.GetIndex() ) );
		m_pClass->m_mapRegistFields[ m_FieldName.GetIndex() ] = this;
		if( m_nResult.GetMulPointer() ||
			m_nResult.IsClass() && m_nResult.GetClass()->IsAnyPointerExist() )
			m_pClass->m_bAnyPointerExist = true;
		m_pClass->m_bDisplayTag |= !!m_Tag.GetFieldTagData( DISPLAY_TAG );
	}
	
	void CField::CallBack( void* pRetBuf, void** pArgArray ) const
	{
		auto* pVirtualObj = *(SVirtualClassInstance**)pArgArray[0];
		SFunctionTable* pVTable = pVirtualObj->pVirtualTable;
		assert( CReflection::GetInstance().IsAllocVirtualTable( pVTable ) );
		assert( GetType() >= eFDT_ClassCallBack );
		auto pFunTableHead = ( (CReflection::SFunTableHead*)pVTable ) - 1;
		assert( pFunTableHead->m_pClass && pFunTableHead->m_pOldFunTable );
		auto pCallbackHook = pFunTableHead->m_pHook;
		if( !pCallbackHook )
			return Call( pRetBuf, pArgArray );
		if( GetType() == eFDT_ClassDestructor )
			pCallbackHook->OnDestruct( this, pVirtualObj );
		else if( pCallbackHook->OnCallBack( this, pRetBuf, pArgArray ) )
			return;
		Call( pRetBuf, pArgArray );
	}

	const char* CField::GetDisplayName( bool bTagOnly ) const
	{
		static auto nStartLen = strlen( DISPLAY_TAG );
		const char* szDisplayName = nullptr;
		if( m_pClass->HasDisplayTag() )
		{
			szDisplayName = m_Tag.GetFieldTagData( DISPLAY_TAG );
			if( szDisplayName )
				szDisplayName += nStartLen;
			if( bTagOnly || szDisplayName )
				return szDisplayName;
		}

		if( bTagOnly )
			return nullptr;

		szDisplayName = GetName().c_str();
		if( m_pClass->IsEnum() && m_pClass->GetEnumPrefixLen() )
			return szDisplayName + m_pClass->GetEnumPrefixLen();

		while( *szDisplayName && islower( *szDisplayName ) )
			++szDisplayName;
		if( *szDisplayName )
			return szDisplayName;
		return GetName().c_str();
	}
}