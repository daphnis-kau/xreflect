#include <array>
#include "ReflectionHelp.h"
#include "CClass.h"
#include <cstdlib>
#include "CField.h"
#include "CReflection.h"
#include <mutex>

namespace XReflect
{
	#define GET_CONSTRUCTOR( return_expression ) \
		assert( GetClassSize() && m_pClassDefine ); \
		if( !m_pClassDefine ) \
			return_expression;
	const char* FormatNameSpace( const char* szNameSpace );
	std::recursive_mutex& GetClassRegisterMutex();

	//=====================================================================
	// Auto-destroyed objects
	//=====================================================================
	struct SAutoReleaseObject
	{
		void*			m_pParent;
		void*			m_pObject;
		const CClass*	m_pParentClass;
		SFieldNode		m_FieldNode;

		SAutoReleaseObject() 
			: m_pObject( nullptr )
			, m_pParentClass( nullptr )
		{
		}

		~SAutoReleaseObject()
		{
			if( !m_pObject )
				return;
			assert( m_FieldNode.GetType() != eFNT_TypeCast );
			auto ValueType = GetNodeValueType();
			auto pClass = ValueType.GetClass();
			// If ClassPointer is returned by value, execution also reaches here
			if( pClass && !pClass->IsEnum() && ValueType.IsValue() )
				pClass->Destruct( m_pObject );
			AlignedFree( m_pObject );
		}

		void* Create( void* pParent,
			const CClass* pParentClass, SFieldNode FieldNode )
		{
			m_pParent = pParent;
			m_FieldNode = FieldNode;
			m_pParentClass = pParentClass;
			auto ValueType = GetNodeValueType();
			m_pObject = AlignedAlloc( ValueType.GetSizeOfType() );
			return m_pObject;
		}

	private:
		UPakDataType GetNodeValueType()
		{
			switch( m_FieldNode.GetType() )
			{
			case eFNT_NormalField:
				return m_FieldNode.GetField()->GetResultType();
			case eFNT_CustomFetcher:
				return m_FieldNode.GetNodeFetcher()->GetFieldType();
			case eFNT_ElementIndex:
				return m_pParentClass->GetContainerElemType();
			default:
				return UPakDataType();
			}
		}
	};

	template<typename Elem, uint64_t nDefaultCount = 32>
	class TTempBuffer
	{
		typedef std::array<Elem, (size_t)nDefaultCount> ArrayType;
		union { tbyte m_aryBuffer[sizeof(ArrayType)]; Elem* m_pBuffer; };
		uint64_t m_nBufferCount;
	public:
		TTempBuffer(uint64_t nElemCount) : m_nBufferCount(nElemCount)
		{
			if (m_nBufferCount <= nDefaultCount)
				new (m_aryBuffer) ArrayType();
			else
				m_pBuffer = new Elem[(size_t)m_nBufferCount];
		}
		~TTempBuffer()
		{
			if (m_nBufferCount <= nDefaultCount)
				((ArrayType*)m_aryBuffer)->~ArrayType();
			else
				delete[] m_pBuffer;
		}
		Elem* GetBuffer() { return m_nBufferCount <= nDefaultCount ? ((ArrayType*)m_aryBuffer)->data() : m_pBuffer; }
		Elem& operator[](uint64_t nIndex) { return GetBuffer()[nIndex]; }
		operator Elem*() { return GetBuffer(); }
	};

	std::pair<void*, UPakDataType> LocationTarget( bool bReadOnly,
		TTempBuffer<SAutoReleaseObject, 32>& CachedObjects,
		const CClass* pClass, void* pObject, SFieldPath FieldPath )
	{
		assert( pObject );
		std::pair<void*, UPakDataType> Result( 
			pObject, UPakDataType( pClass, 0, false ) );
		if( FieldPath.nNodeCount == 0 )
			return Result;
		assert( FieldPath.aryPath );
		uint32 nNodeCount = FieldPath.nNodeCount;

		for( uint32 i = 0; i < nNodeCount; i++ )
		{
			std::pair<void*, UPakDataType> ParentInfo = Result;
			auto FieldNode = FieldPath.aryPath[i];

			// Type casting
			if( FieldNode.GetType() == eFNT_TypeCast )
			{
				if( ParentInfo.second.GetMulPointer() )
					return std::pair<void*, UPakDataType>();
				auto pSrcClass = ParentInfo.second.GetClass();
				auto pDesClass = FieldNode.GetTargetClass();
				auto nOffset = pSrcClass->CalculateBaseOffset( pDesClass );
				if( nOffset >= 0 )
				{
					Result.first = ( (tbyte*)ParentInfo.first ) + nOffset;
					Result.second = UPakDataType( pDesClass, 0, false );
					continue;
				}
                
				nOffset = pDesClass->CalculateBaseOffset( pSrcClass );
				if( nOffset < 0 )
					return std::pair<void*, UPakDataType>();
				Result.first = ( (tbyte*)ParentInfo.first ) - nOffset;
				Result.second = UPakDataType( pDesClass, 0, false );
				continue;
			}
			
			// Access array element / pointer dereference
			if( ParentInfo.second.GetMulPointer() )
			{
				// Pointer has only one operation: ElementIndex
				if( FieldNode.GetType() != eFNT_ElementIndex )
					return std::pair<void*, UPakDataType>();

				tbyte* pArray = *(tbyte**)ParentInfo.first;
				Result.second = UPakDataType( ParentInfo.second, 
					ParentInfo.second.GetMulPointer() - 1, 
					true, ParentInfo.second.IsConstant() );
				size_t nElemIndex = (size_t)FieldNode.GetIndex();
				size_t nElemSize = Result.second.GetSizeOfType();
				Result.first = pArray + nElemSize*nElemIndex;
				continue;
			}

			// Access container element
			if( FieldNode.GetType() == eFNT_ElementIndex )
			{
				if( !ParentInfo.second.IsClass() )
					return std::pair<void*, UPakDataType>();
				auto pParentClass = ParentInfo.second.GetClass();
				auto ElemType = pParentClass->GetContainerElemType();
				if( ElemType.IsVoid() )
					return std::pair<void*, UPakDataType>();
				
				// Otherwise call the iterator's add function
				auto pBegin = pParentClass->FindField( CPP_CONTAINER_BEGIN );
				if( !pBegin )
				{
					auto pElemGet = pParentClass->FindField( ARRAY_ELEM_GET );
					if( !pElemGet )
						return std::pair<void*, UPakDataType>();

					void* pContainer = ParentInfo.first;
					size_t nElemIndex = (size_t)FieldNode.GetIndex();
					void* aryContainerArg[] = { &pContainer, &nElemIndex };
					Result.second = pElemGet->GetResultType();
					// If addressable class member, write variable address to pChild
					if( Result.second.IsReference() )
					{
						pElemGet->Call( &Result.first, aryContainerArg );
						if( !bReadOnly )
						{
							void* pClone = CachedObjects[i].Create(
								pContainer, pParentClass, FieldNode );
							if( Result.second.IsClass() && !Result.second.GetMulPointer() )
								Result.second.GetClass()->Assign( pClone, Result.first );
							else
								memcpy( pClone, Result.first, Result.second.GetSizeOfType() );
							Result.first = pClone;
						}
					}
					// If non-addressable return value (bitfield, property, etc.), write variable content to allocated memory
					else
					{
						Result.first = CachedObjects[i].Create(
							pContainer, pParentClass, FieldNode );
						pElemGet->Call( Result.first, aryContainerArg );
					}
					continue;
				}

				void* pContainer = ParentInfo.first;
				size_t nElemIndex = (size_t)FieldNode.GetIndex();
				char* pCurIterator = (char*)alloca( pBegin->GetResultSize() );
				auto pIteratorType = pBegin->GetResultType().GetClass();
				auto pAdd = pIteratorType->FindField( ITERATOR_ADD );
				auto pValue = pIteratorType->FindField( ITERATOR_VALUE );
				void* aryContainerArg[] = { &pContainer };
				void* aryIteratorArg[] = { &pCurIterator, &nElemIndex };
				Result.second = pValue->GetResultType();
				pBegin->Call( pCurIterator, aryContainerArg );
				pAdd->Call( nullptr, aryIteratorArg );
				// If addressable element, write variable address to Result.first
				if( ElemType.GetMulPointer() || Result.second.IsReference() )
					pValue->Call( &Result.first, aryIteratorArg );
				// If non-addressable return value (bitfield, property, etc.), write variable content to allocated memory
				else
				{
					Result.first = CachedObjects[i].Create(
						pContainer, pParentClass, FieldNode );
					pValue->Call( Result.first, aryContainerArg );
				}
				pIteratorType->Destruct( pCurIterator );
				continue;
			}

			// Access class member variable
			if( !ParentInfo.second.IsClass() )
				return std::pair<void*, UPakDataType>();

			void* pParent = ParentInfo.first;
			const CClass* pParentClass = ParentInfo.second.GetClass();
			assert( FieldNode.GetType() == eFNT_NormalField ||
				FieldNode.GetType() == eFNT_CustomFetcher );
			const CClass* pTargetClass = 
				FieldNode.GetType() == eFNT_NormalField
				? FieldNode.GetField()->GetClass()
				: FieldNode.GetNodeFetcher()->GetObjectClass();

			if( pTargetClass != pParentClass )
			{
				assert( pParentClass && pTargetClass );
				auto nDiff = pTargetClass->CalculateBaseOffset( pParentClass );
				if( nDiff >= 0 )
				{
					pParentClass = pTargetClass;
					pParent = ( (tbyte*)pParent ) - nDiff;
				}
				else if( ( nDiff = pParentClass->
					CalculateBaseOffset( pTargetClass ) ) >= 0 )
				{
					pParentClass = pTargetClass;
					pParent = ( (tbyte*)pParent ) + nDiff;
				}
				else
				{
					// Type mismatch
					return std::pair<void*, UPakDataType>();
				}
			}

			if( FieldNode.GetType() == eFNT_CustomFetcher )
			{
				auto pFetcher = FieldNode.GetNodeFetcher();
				if( !pFetcher )
					return std::pair<void*, UPakDataType>();
				Result.second = pFetcher->GetFieldType();
				void* pResult = nullptr;
				if( Result.second.IsReference() )
					pResult = &Result.first;
				else
					pResult = Result.first = CachedObjects[i].Create(
						pParent, pParentClass, FieldNode );
				if( !pFetcher->GetFieldData( pParent, pResult ) )
					return std::pair<void*, UPakDataType>();
				continue;
			}
			else
			{
				const CField* pField = FieldNode.GetField();
				assert( pField->GetType() == eFDT_Member || pField->GetType() == eFDT_StaticMember );
				Result.second = pField->GetResultType();
				// If addressable class member, write variable address to pChild
				if( pField->GetResultType().IsReference() )
					pField->GetData( pParent, &Result.first );
				// If non-addressable return value (bitfield, property, etc.), write variable content to allocated memory
				else
				{
					Result.first = CachedObjects[i].Create(
						pParent, pParentClass, FieldNode );
					pField->GetData( pParent, Result.first );
				}
			}
		}
		return Result;
	}

	//=====================================================================
	// CClass
	//=====================================================================
	CClass::CClass( const char* szTypeIDName )
		: m_eRegisterType( eCRT_NormalClass )
		, m_TypeIDName( szTypeIDName )
		, m_nClassID( 0 )
		, m_nSizeOfClass( 0 )
		, m_nClassAligenSize( 0 )
		, m_nInheritDepth( 0 )
		, m_nNameSpaceCount( UINT8_MAX )
		, m_nCustomConstructorCount( 0 )
		, m_bAnyPointerExist( false )
		, m_bDefaultConstructor( false )
		, m_bCopyConstructor( false )
		, m_bMoveConstructor( false )
		, m_bCopyAssignable( false )
		, m_bMoveAssignable( false )
		, m_bMaskEnum( false )
		, m_bDisplayTag( false )
		, m_nEnumPrefixLen( 0 )
		, m_nBaseClassCount( 0 )
		, m_nSubClassCount( 0 )
		, m_nOverridableCount( 0 )
		, m_aryBaseClasses( nullptr )
		, m_arySubClasses( nullptr )
		, m_aryNameSpace( nullptr )
		, m_pClassDefine( nullptr )
		, m_aryConstructorParamInfo( nullptr )
		, m_aryOverridableFun( nullptr )
		, m_pSmartPtrField( nullptr )
	{
		assert( !( ( (ptrdiff_t)this ) & 0xF ) );
	}

	CClass::~CClass()
	{
		for( auto& pair : m_mapRegistFields )
			delete pair.second;
		m_mapRegistFields.clear();
		AlignedFree( m_aryConstructorParamInfo );
		delete[] m_aryOverridableFun ; m_aryOverridableFun  = nullptr;;
		delete[] m_aryBaseClasses ; m_aryBaseClasses  = nullptr;;
		delete[] m_arySubClasses ; m_arySubClasses  = nullptr;;
		delete[] m_aryNameSpace ; m_aryNameSpace  = nullptr;;
		m_pSmartPtrField = nullptr;
	}

	void* CClass::operator new( std::size_t nSize )
	{
		return ::operator new( nSize, std::align_val_t( 16 ) );
	}

	void CClass::operator delete( void* ptr )
	{
		::operator delete( ptr, std::align_val_t( 16 ) );
	}

	void CClass::FinishRegister( const char* szNameSpace, uint32 nClassID )
	{
		// Fast path: already registered, return directly (acquire ensures seeing all writes)
		if( m_nClassID.load( std::memory_order_acquire ) != 0 )
			return;

		std::lock_guard<std::recursive_mutex> Lock( GetClassRegisterMutex() );
		// Double-checked (relaxed is fine inside the lock because acquire already guaranteed synchronization)
		if( m_nClassID.load( std::memory_order_relaxed ) != 0 )
			return;

		assert( m_nNameSpaceCount == UINT8_MAX && !m_aryNameSpace );
		if( szNameSpace == nullptr )
		{
			assert( m_eRegisterType == eCRT_NormalClass && m_pClassDefine );
			szNameSpace = FormatNameSpace( m_pClassDefine->GetNameSpace() );
		}

		auto szEnd = strchr( szNameSpace, '<' );
		std::pair<const char*, uint32> aryNameSpace[256];
			size_t nMaxCount = 0;
			{	const char* p = szNameSpace;
				while (*p) {
					if (*p == ':' && *(p+1) == ':') {
						aryNameSpace[nMaxCount++] = {szNameSpace, (uint32_t)(p - szNameSpace)};
						szNameSpace = p + 2; p = szNameSpace;
					} else ++p;
				}
				aryNameSpace[nMaxCount++] = {szNameSpace, (uint32_t)(p - szNameSpace)};
			}
		uint8 nNameSpaceCount = 0;
		for( uint32 i = 0; i < nMaxCount; i++ )
		{
			nNameSpaceCount += aryNameSpace[i].second != 0;
			if( aryNameSpace[i].first + aryNameSpace[i].second < szEnd )
				continue;
			aryNameSpace[i].second = (uint32)strlen( aryNameSpace[i].first );
			nMaxCount = i + 1;
		}

		auto aryName = nNameSpaceCount ? new CName[nNameSpaceCount] : nullptr;
		for( uint32 i = 0, n = 0; i < nMaxCount; i++ )
		{
			if( aryNameSpace[i].second == 0 )
				continue;
			auto Name = aryNameSpace[i];
			aryName[n++] = CName( Name.first, Name.second );
		}
		m_nNameSpaceCount = nNameSpaceCount;
		m_aryNameSpace = aryName;

		if( nClassID == 0 && m_eRegisterType != eCRT_GlobalScope )
		{
			assert( m_eRegisterType == eCRT_NormalClass && m_pClassDefine );
			m_ClassName = m_pClassDefine->GetClassName();
			m_TemplateName = m_pClassDefine->GetTemplateName();

			// To ensure ClassID consistency across different compilers,
			// use ClassName to compute the hash, avoiding compiler-dependent TypeIDName differences
			nClassID = m_pClassDefine->GetClassID();
			if( nClassID == 0 )
			{
				std::string strName;
				for( uint32 i = 0; i < m_nNameSpaceCount; i++ )
				{
					strName += m_aryNameSpace[i].c_str();
					strName += "::";
				}
				const char* szClassName = m_ClassName.c_str();
				assert( szClassName && szClassName[0] );
				strName += szClassName;
				nClassID = StringHash( strName.c_str() );
			}
		}

		// release ensures prior writes are visible to other threads
		m_nClassID.store( nClassID, std::memory_order_release );
		CReflection::GetInstance().m_mapClassID2ClassInfo[ nClassID ] = this;
	}

	CName CClass::GetTemplateName() const 
	{
		if( m_TypeIDName && m_nClassID.load( std::memory_order_acquire ) == 0 )
			const_cast<CClass*>( this )->FinishRegister( nullptr, 0 );
		return m_TemplateName;
	}

	CName CClass::GetClassName() const 
	{
		if( m_TypeIDName && m_nClassID.load( std::memory_order_acquire ) == 0 )
			const_cast<CClass*>( this )->FinishRegister( nullptr, 0 );
		return m_ClassName;
	}

	uint32 CClass::GetClassID() const 
	{ 
		if( m_TypeIDName && m_nClassID.load( std::memory_order_acquire ) == 0 )
			const_cast<CClass*>( this )->FinishRegister( nullptr, 0 );
		return m_nClassID.load( std::memory_order_acquire );
	}

	uint8 CClass::GetNameSpaceCount() const 
	{
		if( m_TypeIDName && m_nClassID.load( std::memory_order_acquire ) == 0 )
			const_cast<CClass*>( this )->FinishRegister( nullptr, 0 );
		return m_nNameSpaceCount;
	}

	CName CClass::GetNameSpace( uint8 nIndex ) const 
	{ 
		if( m_TypeIDName && m_nClassID.load( std::memory_order_acquire ) == 0 )
			const_cast<CClass*>( this )->FinishRegister( nullptr, 0 );
		if( nIndex >= GetNameSpaceCount() )
			return CName();
		return m_aryNameSpace[nIndex];
	}

	UPakDataType CClass::GetContainerElemType() const
	{
		return m_ContainerElemType;
	}

	const CClass* CClass::GetStrongPtrClass() const
	{
		if( m_pSmartPtrField == nullptr )
			return nullptr;
		auto PtrType = m_pSmartPtrField->GetResultType();
		return PtrType.GetMulPointer() ? this : PtrType.GetClass();
	}

	bool CClass::IsStrongPtrClass() const 
	{ 
		if( m_pSmartPtrField == nullptr )
			return false;
		return m_pSmartPtrField->GetResultType().GetMulPointer();
	}

	bool CClass::IsWeakPtrClass() const 
	{ 
		if( m_pSmartPtrField == nullptr )
			return false;
		return !m_pSmartPtrField->GetResultType().GetMulPointer();
	}
	
	const CField* CClass::FindField( CName FieldName, bool bTraveBase ) const
	{
		auto it = m_mapRegistFields.find( FieldName.GetIndex() );
		const CField* pField = it != m_mapRegistFields.end() ? it->second : nullptr;
		if( pField || !bTraveBase )
			return pField;
		uint32 nBaseCount = GetBaseClassCount();
		for( uint32 i = 0; !pField && i < nBaseCount; i++ )
		{
			auto pBaseClass = GetBaseClass( i );
			pField = pBaseClass->FindField( FieldName, true );
		}
		return pField;
	}

	void CClass::EnumFields( uint32 nTypeMask, 
		FieldEnumFun funEnum, void* pContext ) const
	{
		std::function<bool(const CClass*, FieldEnumFun)> funEnumField;
		funEnumField = [&]( const CClass* pClass, FieldEnumFun funEnum )
		{
			for( auto& itField : pClass->m_mapRegistFields )
			{
				auto pField = itField.second;
				if( !( ( 1 << pField->GetType() ) & nTypeMask ) )
					continue;
				if( funEnum( pField, pContext ) )
					continue;
				return false;
			}

			for( uint32 i = 0; i < pClass->GetBaseClassCount(); i++ )
				if( !funEnumField( pClass->GetBaseClass( i ), funEnum ) )
					return false;
			return true;
		};
		funEnumField( this, funEnum );
	}

	const CField* CClass::FindOverrideFun( uint32 nVirtualTableIndex ) const
	{
		for( uint32 i = 0; i < m_nOverridableCount; i++ )
		{
			uint32 nIndex = m_aryOverridableFun[i]->GetVirtualTableIndex();
			if( nIndex != nVirtualTableIndex )
				continue;
			return m_aryOverridableFun[i];
		}
		return nullptr;
	}

	void CClass::Construct( int32 nConstructorIndex, 
		void* pObject, void** aryArg ) const
	{
		// Declaration-only class cannot be created
		assert( pObject );
		GET_CONSTRUCTOR( return );
		m_pClassDefine->Construct( nConstructorIndex, pObject, aryArg );
	}

	void CClass::Destruct( void* pObject ) const
	{
		// Declaration-only class cannot be created
		assert( pObject );
		GET_CONSTRUCTOR( return );
		m_pClassDefine->Destruct( pObject );
	}

	void* CClass::New( int32 nConstructorIndex, void** aryArg ) const
	{
		// Declaration-only class cannot be created
		GET_CONSTRUCTOR( return nullptr );
		return m_pClassDefine->New( nConstructorIndex, aryArg );
	}

	void CClass::Delete( void* pObject ) const
	{
		// Declaration-only class cannot be created
		assert( pObject );
		GET_CONSTRUCTOR( return );
		m_pClassDefine->Delete( pObject );
	}

	void CClass::Assign( void* pDest, void* pSrc ) const
	{
		assert( pDest && pSrc );
		GET_CONSTRUCTOR( return );
		m_pClassDefine->Assign( pDest, pSrc );
	}

	void CClass::Move( void* pDest, void* pSrc ) const
	{
		assert( pDest && pSrc );
		GET_CONSTRUCTOR( return );
		m_pClassDefine->Move( pDest, pSrc );
	}

	int64 CClass::GetContainerSize( void* pObject ) const
	{
		if( GetContainerElemType().IsVoid() )
			return -1;
		auto pSize = FindField( CPP_CONTAINER_SIZE );
		if( !pSize )
			return -1;
		size_t nSize = 0;
		void* aryContainerArg[] = { &pObject };
		pSize->Call( &nSize, aryContainerArg );
		return (int64)nSize;
	}

	bool CClass::Fetch( 
		void* pObject, SFieldPath FieldPath, void* pContext,
		void(*Receiver)( void* pData, void* pContext ) ) const
	{
		const CClass* pClass = this;
			TTempBuffer<SAutoReleaseObject, 32> CachedObjects( FieldPath.nNodeCount );
		auto Target = LocationTarget( true, CachedObjects, pClass, pObject, FieldPath );
		if( Target.second.IsVoid() )
			return false;
		Receiver( Target.first, pContext );
		return true;
	}

	bool CClass::Modify( void* pObject, void* pData, bool bIsXValue, 
		SFieldPath FieldPath, EFieldOperateType eModifyType ) const
	{
		if( eModifyType != eFOT_RemoveElem && pData == nullptr )
			return false;
		const CClass* pClass = this;
		bool bElemAddOrDelete = eModifyType != eFOT_Modify;
		auto LastNode = FieldPath.aryPath[FieldPath.nNodeCount - 1];
		uint32 nNodeCount = FieldPath.nNodeCount - bElemAddOrDelete;
		FieldPath.nNodeCount = nNodeCount;
			TTempBuffer<SAutoReleaseObject, 32> CachedObjects( nNodeCount );
		auto Target = LocationTarget( false, CachedObjects, pClass, pObject, FieldPath );
		if( Target.second.IsVoid() )
			return false;

		if( LastNode.GetType() == eFNT_CustomFetcher ||
			LastNode.GetType() == eFNT_NormalField )
		{
			if( Target.second.IsReference() )
			{
				auto nPointer = Target.second.GetMulPointer();
				Target.second = UPakDataType( Target.second, nPointer );
			}

			if( !Target.second.IsClassValue() )
				memcpy( Target.first, pData, Target.second.GetSizeOfType() );
			else if( bIsXValue && Target.second.GetClass()->IsMoveAssignable() )
				Target.second.GetClass()->Move( Target.first, (void*)pData );
			else if( Target.second.GetClass()->IsCopyAssignable() )
				Target.second.GetClass()->Assign( Target.first, (void*)pData );
			else
				return false;
		}
		else
		{
			const CClass* pParentClass = Target.second.GetClass();
			if( LastNode.GetType() != eFNT_ElementIndex || !bElemAddOrDelete )
				return false;
			auto pErase = pParentClass->FindField( CPP_CONTAINER_ERASE );
			auto pInsert = pParentClass->FindField( CPP_CONTAINER_INSERT );
			if( !pErase || !pInsert )
				return false;
			auto EraseIndexType = UPakDataType( pErase->GetParamType( 1 ) );
			auto InsertIndexType = UPakDataType( pInsert->GetParamType( 1 ) );
			if( EraseIndexType != InsertIndexType )
				return false;

			size_t nSize = pParentClass->GetContainerSize( Target.first );
			assert( nSize >=0 );

			if( EraseIndexType.IsBaseType() &&
				EraseIndexType.GetDataType() >= eDT_char&&
				EraseIndexType.GetDataType() <= eDT_bool )
			{
				void* pParent = Target.first;
				size_t nIndex = (size_t)LastNode.GetIndex();
				void* aryContainerArg[] = { &pParent, &nIndex, (void*)pData };
				if( eModifyType == eFOT_RemoveElem )
				{
					assert( nIndex < nSize );
					void* pObject = nullptr;
					pErase->Call( nullptr, aryContainerArg );
				}
				else
				{
					assert( nIndex <= nSize );
					pInsert->Call( nullptr, aryContainerArg );
				}
			}
			else
			{
				auto pBegin = pParentClass->FindField( CPP_CONTAINER_BEGIN );
				if( !pBegin || pBegin->GetResultType() != EraseIndexType )
					return false;
				void* pParent = Target.first;
				char* pCurIterator = (char*)alloca( pBegin->GetResultSize() );
				auto pIteratorType = pBegin->GetResultType().GetClass();
				auto pAdd = pIteratorType->FindField( ITERATOR_ADD );
				auto nIndex = LastNode.GetIndex();
				void* aryContainerArg[] = { &pParent, pCurIterator, (void*)pData };
				void* aryIteratorArg[] = { &pCurIterator, &nIndex };
				pBegin->Call( pCurIterator, aryContainerArg );
				CScopeExit DeleteIterator( [&](){ pIteratorType->Destruct( pCurIterator ); } );
				UPakDataType ValueType = pInsert->GetParamType( 2 );

				if( eModifyType == eFOT_RemoveElem )
				{
					assert( nIndex < nSize );
					void* pObject = nullptr;
					pAdd->Call( nullptr, aryIteratorArg );
					pErase->Call( nullptr, aryContainerArg );
				}
				else
				{
					assert( nIndex <= nSize );
					pAdd->Call( nullptr, aryIteratorArg );
					pInsert->Call( nullptr, aryContainerArg );
				}
			}
		}

		// If there are temporary objects, copy node by node
		for( uint32 i = nNodeCount; i > 0; i-- )
		{
			auto& Object = CachedObjects[i - 1];
			if( !Object.m_pObject || !Object.m_pParent )
				continue;
			if( Object.m_FieldNode.GetType() == eFNT_TypeCast )
				continue;

			// Write ordinary class member variable
			if( Object.m_FieldNode.GetType() == eFNT_NormalField )
			{
				auto pField = Object.m_FieldNode.GetField();
				if( pField->GetMemberQualifier()&eMBQ_DisableWrite )
					break;
				pField->SetData( Object.m_pParent, Object.m_pObject );
			}
			else if( Object.m_FieldNode.GetType() == eFNT_CustomFetcher )
			{
				auto pFetcher = Object.m_FieldNode.GetNodeFetcher();
				assert( pFetcher != nullptr );
				pFetcher->SetFieldData( Object.m_pParent, Object.m_pObject );
			}
			else if( Object.m_FieldNode.GetType() == eFNT_ElementIndex )
			{
				auto pElemSet = Object.m_pParentClass->FindField( ARRAY_ELEM_SET );
				if( !pElemSet )
					break;
				size_t nIndex = Object.m_FieldNode.GetIndex();
				void* aryArg[] = { &Object.m_pParent, &nIndex, Object.m_pObject };
				pElemSet->Call( nullptr, aryArg );
			}
		}
		return true;
	}

	void CClass::HookVirtualTable( void* pObj, ICallbackHook* pHook ) const
	{
		SVirtualClassInstance* pVObj = (SVirtualClassInstance*)pObj;
		SFunctionTable* pOldTable = pVObj->pVirtualTable;
		SFunctionTable* pNewTable = nullptr;
		CReflection& Reflection = CReflection::GetInstance();
		assert( !Reflection.IsAllocVirtualTable( pOldTable ) );

		if( GetOverridableCount() )
		{
			// Ensure pOldTable is the original vtable, because pVObj may have been modified
			pOldTable = Reflection.GetOrgVirtualTable( pVObj->pVirtualTable );
			pNewTable = pHook->FindHookVirtualTable( this, pOldTable );
			if( pNewTable == nullptr )
			{
				pNewTable = Reflection.
					AllocVirtualTable( pHook, this, pOldTable );
				pHook->SetHookVirtualTable( this, pOldTable, pNewTable );
			}
			assert( Reflection.IsAllocVirtualTable( pNewTable ) );
		}

		uint32 nBaseCount = GetBaseClassCount();
		for( uint32 i = 0; i < nBaseCount; i++ )
		{
			auto pBaseClass = GetBaseClass( i );
			if( pBaseClass->GetOverridableCount() )
			{
				void* pBaseObj = ( (char*)pObj ) + GetBaseClassOff(i);
				pBaseClass->HookVirtualTable( pBaseObj, pHook );
			}
		}

		if( !pNewTable )
			return;
		pVObj->pVirtualTable = pNewTable;
	}

	void CClass::UnhookVirtualTable( void* pObj ) const
	{
		SVirtualClassInstance* pVObj = (SVirtualClassInstance*)pObj;
		CReflection& Reflection = CReflection::GetInstance();
		if( !GetOverridableCount() ||
			!Reflection.IsAllocVirtualTable( pVObj->pVirtualTable ) )
			return;

		auto pOrgTable = Reflection.GetOrgVirtualTable( pVObj->pVirtualTable );
		uint32 nBaseCount = GetBaseClassCount();
		for( uint32 i = 0; i < nBaseCount; i++ )
		{
			auto pBaseClass = GetBaseClass( i );
			void* pBaseObj = ( (char*)pObj ) + GetBaseClassOff(i);
			pBaseClass->UnhookVirtualTable( pBaseObj );
		}

		if( !pOrgTable )
			return;
		pVObj->pVirtualTable = pOrgTable;
	}
}
