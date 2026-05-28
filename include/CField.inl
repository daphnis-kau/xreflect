#include "CField.h"

namespace XReflect
{
	inline void CField::Call( void* pRetBuf, void** pArgArray ) const
	{
		assert( m_eFieldType != eFDT_Member && m_eFieldType != eFDT_StaticMember );
		assert( m_eFieldType != eFDT_Enumeration );
		assert( m_eFieldType != eFDT_ClassPureCallBack );
		assert( m_eFieldType < eFDT_ClassFunction || *(char**)pArgArray[0] );

		switch( m_eFieldType )
		{
		case eFDT_ClassDestructor:
		{
			uint32 nVirtualIndex = GetVirtualTableIndex();
			auto pObject = (SVirtualClassInstance*)*(void**)pArgArray[0];
			auto pCurTable = pObject->pVirtualTable;
			if( !pCurTable || !pCurTable->aryFun[nVirtualIndex] )
				return;
			auto Wrap = m_FieldInfo.m_CallbackInfo.m_CallbackWrap;
			Wrap.m_funDestructorWrap( pObject );
		}
		break;
		case eFDT_ClassCallBack:
		{
			uint32 nVirtualIndex = GetVirtualTableIndex();
			auto funBoot = m_FieldInfo.m_CallbackInfo.m_funBoot;
			auto& Reflection = CReflection::GetInstance();
			auto pObject = (SVirtualClassInstance*)*(void**)pArgArray[0];
			auto pOrgTable = Reflection.GetOrgVirtualTable( pObject->pVirtualTable );
			if( !pOrgTable || !pOrgTable->aryFun[nVirtualIndex] )
				return;
			assert( pOrgTable->aryFun[nVirtualIndex] != funBoot );
			auto funOrg = (FunctionPointer)pOrgTable->aryFun[nVirtualIndex];
			auto Wrap = m_FieldInfo.m_CallbackInfo.m_CallbackWrap;
			Wrap.m_funCallbackWrap( pRetBuf, pArgArray, funOrg );
		}
		break;
		default:
			m_FieldInfo.m_funCall( pRetBuf, pArgArray );
		break;
		}
	}

	inline void CField::GetData( void* pObject, void* pResult ) const
	{
		assert( m_eFieldType == eFDT_Member || m_eFieldType == eFDT_StaticMember );
		m_FieldInfo.m_MemberInfo.m_funGet( pObject, pResult );
	}

	inline void CField::SetData( void* pObject, void* pData ) const
	{
		assert( m_eFieldType == eFDT_Member || m_eFieldType == eFDT_StaticMember );
		if( !m_FieldInfo.m_MemberInfo.m_funSet )
			throw std::runtime_error( "Write readonly property" );
		m_FieldInfo.m_MemberInfo.m_funSet( pObject, pData );
	}

	template<bool bReference>
	template<typename Type>
	inline Type CField::TMemberFetch<bReference>::
		Get( void* pObject, const CField* pField )
	{
		assert( pField->m_eFieldType == eFDT_Member || pField->m_eFieldType == eFDT_StaticMember );
		if( pField->GetResultType().IsReference() )
			return TMemberFetch<true>::Get<Type&>( pObject, pField );
		Type vResult;
		pField->m_FieldInfo.m_MemberInfo.m_funGet( pObject, &vResult );
		return vResult;
	}

	template<typename Type>
	inline Type CField::TMemberFetch<true>::
		Get( void* pObject, const CField* pField )
	{
		assert( pField->m_eFieldType == eFDT_Member || pField->m_eFieldType == eFDT_StaticMember );
		assert( pField->GetResultType().IsReference() );
		typename std::remove_reference<Type>::type* pResult;
		pField->m_FieldInfo.m_MemberInfo.m_funGet( pObject, &pResult );
		return *pResult;
	}

	template<typename ClassType, typename Type>
	inline Type CField::Get( ClassType* pObject ) const
	{
		typedef typename std::remove_reference<Type>::type ValueType;
		assert( m_eFieldType == eFDT_Member || m_eFieldType == eFDT_StaticMember );
		assert( UPakDataType( GetTypeInfo<ClassType*>() ) == GetParamType( 0 ) );
		assert( UPakDataType( GetTypeInfo<ValueType>() ) == GetResultType().OverrideReference() );
		typedef TMemberFetch<std::is_reference_v<Type>> FetchType;
		return FetchType::template Get<Type>( pObject, this );
	}

	template<typename ClassType, typename Type>
	inline void CField::Set( ClassType* pObject, const Type& v ) const
	{
		typedef typename std::remove_reference<Type>::type ValueType;
		if( !m_FieldInfo.m_MemberInfo.m_funSet )
			throw std::runtime_error( "Write readonly property" );
		assert( m_eFieldType == eFDT_Member || m_eFieldType == eFDT_StaticMember );
		assert( UPakDataType( GetTypeInfo<ClassType*>() ) == GetParamType( 0 ) );
		assert( UPakDataType( GetTypeInfo<ValueType>() ) == GetResultType().OverrideReference() );
		m_FieldInfo.m_MemberInfo.m_funSet( pObject, (void*)&v );
	}

	inline CName CField::GetName() const 
	{ 
		return m_FieldName; 
	}

	inline EFieldType CField::GetType() const 
	{ 
		return m_eFieldType; 
	}

	inline const CFieldTags& CField::GetTag() const 
	{ 
		return m_Tag; 
	}

	inline uint32 CField::GetParamCount() const 
	{ 
		return m_nParamCount; 
	}

	inline UPakDataType CField::GetParamType( uint32 n ) const 
	{ 
		return m_aryParamType[n]; 
	}

	inline uint32 CField::GetParamSize( uint32 n ) const 
	{ 
		return m_aryParamSize[n]; 
	}

	inline uint32 CField::GetParamTotalSize() const 
	{ 
		return m_nTotalParamSize; 
	}

	inline UPakDataType CField::GetResultType() const 
	{ 
		return m_nResult; 
	}

	inline uint32 CField::GetResultSize() const 
	{ 
		return m_nReturnSize; 
	}

	inline int32 CField::GetVirtualTableIndex() const 
	{ 
		return m_nRegisterValue; 
	}

	inline EMemberQualifier CField::GetMemberQualifier() const 
	{ 
		assert( GetType() == eFDT_Member );
		return (EMemberQualifier)m_nRegisterValue;
	}

	inline bool CField::IsSmartFieldPtr() const 
	{ 
		return m_pClass->m_pSmartPtrField == this; 
	}

	inline int32 CField::GetEnumValue() const 
	{ 
		return m_nRegisterValue; 
	}

	inline const CClass* CField::GetClass() const
	{
		return m_pClass;
	}
}
