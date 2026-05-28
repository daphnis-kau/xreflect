#include "CClass.h"
/**@file  		CClass.inl
* @brief		Register information of class 
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once

namespace XReflect
{
	//==============================================================================
	// UPakDataType
	//==============================================================================
	inline void UPakDataType::SetMulPointer( uint8 nPointers )
	{ 
		m_nPointers = nPointers; 
	}

	inline void UPakDataType::SetReference( bool bReference ) 
	{
		m_bReference = bReference;
	}

	inline void UPakDataType::SetConstant( bool bConstant ) 
	{
		m_bConstant = bConstant;
	}

	inline bool UPakDataType::IsVoid() const
	{
		return m_pClass == nullptr;
	}

	inline bool UPakDataType::IsBaseType() const
	{
		return !m_bClass;
	}

	inline const CClass* UPakDataType::GetClass() const
	{
		if( !m_bClass )
			return nullptr;
		ptrdiff_t nAddress = (ptrdiff_t)m_pClass;
		return (const CClass*)( nAddress & ( ~0xFULL ) );
	}

	inline EDataType UPakDataType::GetDataType() const
	{
		if( !m_bClass )
			return (EDataType)( m_eDataType );
		return GetClass()->IsEnum() ? eDT_enum : eDT_class;
	}

	inline bool UPakDataType::IsEnum() const
	{
		return m_bClass && GetClass()->IsEnum();
	}

	inline bool UPakDataType::IsClass() const
	{
		if( !m_bClass )
			return false;
		ptrdiff_t nAddress = ( (ptrdiff_t)m_pClass ) & ( ~0xFULL );
		return ( (const CClass*)( nAddress ) )->IsEnum() == false;
	}

	inline bool UPakDataType::IsValue() const
	{
		return !m_nPointers && !m_bReference;
	}

	inline bool UPakDataType::IsConstant() const
	{
		return m_bConstant;
	}

	inline bool UPakDataType::IsReference() const
	{
		return m_bReference;
	}

	inline uint8 UPakDataType::GetMulPointer() const
	{
		return (uint8)m_nPointers;
	}

	inline bool UPakDataType::IsClassValue() const
	{
		return IsValue() && IsClass();
	}

	inline bool UPakDataType::IsClassReference() const
	{
		return !m_nPointers && m_bReference && IsClass();
	}

	inline UPakDataType UPakDataType::OverrideMulPointer( uint8 nMulPointer ) const
	{
		return UPakDataType( *this, nMulPointer, IsReference(), IsConstant() );
	}

	inline UPakDataType UPakDataType::OverrideReference( bool bReference ) const
	{
		return UPakDataType( *this, GetMulPointer(), bReference, IsConstant() );
	}

	inline UPakDataType UPakDataType::OverrideConstant( bool bConstant ) const
	{
		return UPakDataType( *this, GetMulPointer(), IsReference(), bConstant );
	}

	inline bool UPakDataType::operator == ( const UPakDataType& other ) const
	{
		return m_pClass == other.m_pClass;
	}

	//==============================================================================
	// CClass
	//==============================================================================
	inline const UPakDataType* SParamsInfo::GetParamType() const
	{
		return (const UPakDataType*)( this + 1 );
	}

	inline const uint32* SParamsInfo::GetParamSize() const
	{
		return (const uint32*)( ( (const UPakDataType*)( this + 1 ) ) + nParamCount );
	}

	//==============================================================================
	// CClass
	//==============================================================================
	inline const SParamsInfo* CClass::GetConstructorParamsInfo( int32 n ) const
	{
		static SParamsInfo DefaultParamInfo = { 0, 0 };
		if( n == eBCI_DefaultConstructor )
			return IsDefaultConstructible() ? &DefaultParamInfo : nullptr;

		if( n == eBCI_CopyConstructor || n == eBCI_MoveConstructor )
		{
			if( !IsCopyConstructible() && n == eBCI_CopyConstructor ||
				!IsMoveConstructible() && n == eBCI_MoveConstructor )
				return nullptr;
			uint32 nOffset = m_aryConstructorParamInfo[m_nCustomConstructorCount];
			return (const SParamsInfo*)( ( (tbyte*)m_aryConstructorParamInfo ) + nOffset );
		}

		if( (uint32)n >= m_nCustomConstructorCount )
			return nullptr;
		uint32 nOffset = m_aryConstructorParamInfo[n];
		return (const SParamsInfo*)( ((tbyte*)m_aryConstructorParamInfo) + nOffset );
	}

	inline ptrdiff_t CClass::CalculateBaseOffset( CName BaseClassTypeName ) const
	{
		if( m_TypeIDName == BaseClassTypeName )
			return 0;

		for( size_t i = 0; i < m_nBaseClassCount; i++ )
		{
			auto& BaseInfo = m_aryBaseClasses[i];
			auto nOffset = BaseInfo.pClass->CalculateBaseOffset( BaseClassTypeName );
			if( nOffset < 0 )
				continue;
			return BaseInfo.nOffset + nOffset;
		}

		return -1;
	}

	inline ptrdiff_t CClass::CalculateBaseOffset( const CClass* pBaseClass ) const
	{
		if( pBaseClass == this )
			return 0;

		for( size_t i = 0; i < m_nBaseClassCount; i++ )
		{
			auto& BaseInfo = m_aryBaseClasses[i];
			auto nOffset = BaseInfo.pClass->CalculateBaseOffset( pBaseClass );
			if( nOffset < 0 )
				continue;
			return BaseInfo.nOffset + nOffset;
		}

		return -1;
	}

	inline bool CClass::IsA( CName ClassTypeName ) const
	{
		if( m_TypeIDName == ClassTypeName )
			return true;
		for( uint32 i = 0; i < m_nBaseClassCount; i++ )
			if( m_aryBaseClasses[i].pClass->IsA( ClassTypeName ) )
				return true;
		return false;
	}

	inline bool CClass::IsA( const CClass* pClass ) const
	{
		if( pClass == this )
			return true;
		for( uint32 i = 0; i < m_nBaseClassCount; i++ )
			if( m_aryBaseClasses[i].pClass->IsA( pClass ) )
				return true;
		return false;
	}

	inline bool CClass::IsBaseOffset( ptrdiff_t nDiff ) const
	{
		// Exact match
		if( nDiff == 0 )
			return true;

		// Beyond class memory range
		if( nDiff > (int32)GetClassSize() )
			return false;

		for( size_t i = 0; i < m_nBaseClassCount; i++ )
		{
			ptrdiff_t nBaseDiff = m_aryBaseClasses[i].nOffset;
			if( nDiff < nBaseDiff )
				continue;
			if( m_aryBaseClasses[i].pClass->IsBaseOffset( nDiff - nBaseDiff ) )
				return true;
		}

		// No matching base class
		return false;
	}

	inline bool CClass::Fetch( void* pObject, 
		SFieldPath FieldPath, std::function<void( void* )> fun ) const
	{
		void( *Callback )( void*, void* ) = []( void* pElement, void* pContext )
			{ (*( std::function<void( void* )>* )pContext)( pElement ); };
		return Fetch( pObject, FieldPath, &fun, Callback );
	}

	template<class T>
	inline T CClass::Fetch( void* pObject, SFieldPath FieldPath ) const
	{
		T Result;
		void( *Callback )( void*, void* ) = []( void* pData, void* pContext )
			{ *(T*)pContext = *(const T*)pData; };
		Fetch( pObject, FieldPath, &Result, Callback );
		return Result;
	}

	inline void CClass::EnumFields( uint32 nTypeMask, CFieldEnumFun funEnum ) const
	{
		static auto s_funEnum = []( const CField* pField, void* pContext )
			{ return (*(CFieldEnumFun*)pContext )( pField ); };
		EnumFields( nTypeMask, s_funEnum, &funEnum );
	}
}
      
