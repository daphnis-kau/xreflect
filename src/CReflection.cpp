#include "ReflectionHelp.h"
#include "CClass.h"
#include "CField.h"
#include <cstdio>
#include <mutex>
#include <cstdlib>

namespace XReflect
{
	const char* FormatNameSpace( const char* szNameSpace )
	{
		if( szNameSpace[0] != ':' )
			return szNameSpace;
		assert( szNameSpace[1] == ':' );
		return szNameSpace + 2;
	}

	struct SFuncTableBuffer
	{
		SFuncTableBuffer* pNextBuffer;
		uint32 nCommitPtrCount;
		uint32 nUsePtrCount;
	};

	enum
	{
		eVirtualFunTableSize = sizeof( void* ) * 1024 * 1024,
		eFuncTableBufferSize = eVirtualFunTableSize - sizeof( SFuncTableBuffer ),
		eMaxVirtualFunCount = eFuncTableBufferSize / sizeof( void* ),
		eFunctionTableHeadSize = sizeof( CReflection::SFunTableHead ),
		ePointerCount = eFunctionTableHeadSize / sizeof( void* ),
		eFunctionTableHeadAligSize = ePointerCount * sizeof( void* )
	};

	XReflect::CReflection& CReflection::GetInstance()
	{
		static_assert( eFunctionTableHeadAligSize == eFunctionTableHeadSize,
			"CReflection::SFunctionTableHead size invalid!!!" );
		static CReflection s_Instance;
		return s_Instance;
	}

	std::recursive_mutex& GetClassRegisterMutex() { static std::recursive_mutex m; return m; }
	//=====================================================
	// private
	//=====================================================
	CReflection::CReflection()
		: m_pCurFuctionTable( nullptr )
	{
		AllocNewFuncTableBuffer();
	}

	CReflection::~CReflection()
	{
		for( auto& pair : m_mapTypeID2ClassInfo )
			delete pair.second;
		m_mapTypeID2ClassInfo.clear();
		m_mapClassID2ClassInfo.clear();

		uint32_t nPageSize = GetVirtualMemoryPageSize();
		while( m_pCurFuctionTable )
		{
			SFuncTableBuffer* pCurVTBuffer = m_pCurFuctionTable;
			m_pCurFuctionTable = m_pCurFuctionTable->pNextBuffer;
			DecommitVirtualMemory( pCurVTBuffer, nPageSize );
			FreeVirtualMemory( pCurVTBuffer, nPageSize );
		}
	}

	void CReflection::AllocNewFuncTableBuffer()
	{
		void* pNewMemory = ReserveVirtualMemory( eVirtualFunTableSize );
		uint32_t nPageSize = GetVirtualMemoryPageSize();
		uint32_t nPageFuctionCount = nPageSize / sizeof( void* );
		assert( nPageFuctionCount * sizeof( void* ) == nPageSize );
		CommitVirtualMemory( pNewMemory, nPageSize );
		SFuncTableBuffer* pNewVTBuffer = (SFuncTableBuffer*)pNewMemory;
		pNewVTBuffer->nCommitPtrCount = nPageFuctionCount;
		pNewVTBuffer->nUsePtrCount = sizeof( SFuncTableBuffer ) / sizeof( void* );
		pNewVTBuffer->pNextBuffer = m_pCurFuctionTable;
		m_pCurFuctionTable = pNewVTBuffer;
	}

	CClass* CReflection::DeclareClass( const char* szTypeIDName )
	{
		CName Key( szTypeIDName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		if( pClass )
			return pClass;
		pClass = new CClass( szTypeIDName );
		pClass->m_eRegisterType = eCRT_NormalClass;
		m_mapTypeID2ClassInfo[Key.GetIndex()] = pClass;
		std::lock_guard<std::recursive_mutex> Lock( GetClassRegisterMutex() );
		m_vecClassPending.push_back( pClass );
		return pClass;
	}

	CClass* CReflection::DeclareEnumType( const char* szTypeIDName )
	{
		CName Key( szTypeIDName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		if( pClass )
			return pClass;
		pClass = new CClass( szTypeIDName );
		pClass->m_eRegisterType = eCRT_Enumeration;
		m_mapTypeID2ClassInfo[Key.GetIndex()] = pClass;
		std::lock_guard<std::recursive_mutex> Lock( GetClassRegisterMutex() );
		m_vecClassPending.push_back( pClass );
		return pClass;
	}

	CField* CReflection::RegisterCallBack(
		const char* szTypeInfoName, CField* pCallBack )
	{
		CName Key( szTypeInfoName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		if( !pClass )
			return nullptr;

		// Cannot register duplicate
		uint32 nVirtualTableIndex = pCallBack->GetVirtualTableIndex();
		assert( !pClass->FindOverrideFun( nVirtualTableIndex ) );
		AppendArray( pClass->m_aryOverridableFun,
			pClass->m_nOverridableCount, pCallBack );

		for( uint32 i = 0; i < pClass->m_nSubClassCount; ++i )
		{
			if( pClass->m_arySubClasses[i].nOffset )
				continue;
			auto pSubClass = pClass->m_arySubClasses[i].pClass;
			auto strName = pSubClass->GetTypeIDName();
			RegisterCallBack( strName.c_str(), pCallBack );
		}
		return pCallBack;
	}

	//=====================================================
	// public
	//=====================================================
	const CClass* CReflection::RegisterEnumType(
		const char* szNameSpace, const char* szEnumType, uint32 nClassID,
		const char* szTypeIDName, int32 nTypeSize, bool bMask )
	{
		szNameSpace = FormatNameSpace( szNameSpace );
		if( nClassID == 0 )
			nClassID = StringHash( szEnumType );

		CName Key( szTypeIDName );
		CClass* pInfo = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( !pInfo || pInfo->m_nClassID == 0 );
		assert( nTypeSize <= sizeof( uint32 ) );
		if( !pInfo )
		{
			pInfo = new CClass( szTypeIDName );
		}
		pInfo->m_eRegisterType = eCRT_Enumeration;
		m_mapTypeID2ClassInfo[Key.GetIndex()] = pInfo;
		pInfo->m_bMaskEnum = bMask;
		pInfo->m_nEnumPrefixLen = UINT8_MAX;
		pInfo->m_nSizeOfClass = nTypeSize;
		pInfo->m_nClassAligenSize = sizeof( void* );
		pInfo->m_ClassName = szEnumType;
		pInfo->FinishRegister( szNameSpace, nClassID );
		return pInfo;
	}

	const CField* CReflection::RegisterEnumValue( const CFieldTags& Tag,
		const char* szTypeIDName, const char* szEnumValue, int32 nValue )
	{
		CName Key( szTypeIDName );
		CName ValueKey( szEnumValue );
		assert( m_mapTypeID2ClassInfo.count( Key.GetIndex() ) );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass && !pClass->GetRegistFields().count( ValueKey.GetIndex() ) );
		auto nCurLen = strlen( szEnumValue );
		assert( nCurLen < UINT8_MAX );
		if( pClass->m_nEnumPrefixLen > nCurLen )
			pClass->m_nEnumPrefixLen = (uint8)nCurLen;
		auto pFirstField = pClass->m_mapRegistFields.empty() ? nullptr : pClass->m_mapRegistFields.begin()->second;
		if( pFirstField )
		{
			uint32 nMaxSameLen = 0;
			auto szFirstName = pFirstField->GetName().c_str();
			while( nMaxSameLen < pClass->m_nEnumPrefixLen &&
				szFirstName[nMaxSameLen] == szEnumValue[nMaxSameLen] )
				nMaxSameLen++;
			pClass->m_nEnumPrefixLen = nMaxSameLen;
		}

		return new CField( Tag, pClass, szEnumValue, nValue );
	}

	const CClass* CReflection::RegisterClass(
		const char* szTypeIDName, IClassDefinition* pDef )
	{
		CName Key( szTypeIDName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		if( !pClass )
		{
			pClass = new CClass( szTypeIDName );
			pClass->m_eRegisterType = eCRT_NormalClass;
			m_mapTypeID2ClassInfo[Key.GetIndex()] = pClass;
			std::lock_guard<std::recursive_mutex> Lock( GetClassRegisterMutex() );
			m_vecClassPending.push_back( pClass );
		}

		if( pClass->m_nCustomConstructorCount || pClass->m_pClassDefine )
			return nullptr;

		pClass->m_nSizeOfClass = pDef->GetClassSize();
		pClass->m_nClassAligenSize =
			(uint32)AlignUp( pClass->m_nSizeOfClass, sizeof( void* ) );

		pClass->m_pClassDefine = pDef;
		pClass->m_ContainerElemType = pDef->GetContainerElemType();
		pClass->m_bDefaultConstructor = pDef->IsDefaultConstructible();
		pClass->m_bCopyConstructor = pDef->IsCopyConstructible();
		pClass->m_bMoveConstructor = pDef->IsMoveConstructible();
		pClass->m_bCopyAssignable = pDef->IsCopyAssignable();
		pClass->m_bMoveAssignable = pDef->IsMoveAssignable();
		uint32 nConstructorCount = pDef->GetCustomConstructorCount();
		bool bCopyValid = pClass->m_bCopyConstructor || pClass->m_bMoveConstructor;
		if( !nConstructorCount && !bCopyValid )
			return pClass;

		assert( nConstructorCount <= (uint8)-1 );
		uint64 nParamInfoTotalSize = sizeof( uint32 ) * ( nConstructorCount + 1 );
		nParamInfoTotalSize = AlignUp( nParamInfoTotalSize, sizeof( void* ) );
		for( uint32 i = 0; i < nConstructorCount; i++ )
		{
			auto aryTypeInfo = pDef->GetCustomConstructorParam( i );
			uint32 nParamCount = aryTypeInfo.nInfoCount - 1;
			nParamInfoTotalSize += sizeof( SParamsInfo );
			nParamInfoTotalSize += sizeof( UPakDataType ) * nParamCount;
			nParamInfoTotalSize += sizeof( uint32 ) * nParamCount;
			nParamInfoTotalSize = AlignUp( nParamInfoTotalSize, sizeof( void* ) );
		}

		uint64 nBuiltinSize =
			sizeof( SParamsInfo ) + sizeof( UPakDataType ) + sizeof( uint32 );
		nBuiltinSize = AlignUp( nBuiltinSize, sizeof( void* ) );
		nParamInfoTotalSize += nBuiltinSize;
		nParamInfoTotalSize = AlignUp( nParamInfoTotalSize, sizeof( void* ) );
		assert( nParamInfoTotalSize % sizeof( void* ) == 0 );

		pClass->m_nCustomConstructorCount = (uint8)nConstructorCount;
		pClass->m_aryConstructorParamInfo = (uint32*)AlignedAlloc( nParamInfoTotalSize );
		tbyte* pParamBuffer = (tbyte*)pClass->m_aryConstructorParamInfo;

		uint64 nParamInfoOffset = sizeof( uint32 ) * ( nConstructorCount + 1 );
		nParamInfoOffset = AlignUp( nParamInfoOffset, sizeof( void* ) );
		for( uint8 i = 0; i < nConstructorCount; i++ )
		{
			pClass->m_aryConstructorParamInfo[i] = (uint32)nParamInfoOffset;
			auto aryTypeInfo = pDef->GetCustomConstructorParam( i );
			auto& TypeInfo = aryTypeInfo.aryInfo[0];
			uint32 nParamCount = aryTypeInfo.nInfoCount - 1;

			// Fill constructor parameters
			auto pParamInfo = (SParamsInfo*)( pParamBuffer + nParamInfoOffset );
			nParamInfoOffset += sizeof( SParamsInfo );
			auto aryParamTypes = (UPakDataType*)( pParamInfo + 1 );
			nParamInfoOffset += sizeof( UPakDataType ) * nParamCount;
			auto aryParamSizes = (uint32*)( aryParamTypes + nParamCount );
			nParamInfoOffset += sizeof( uint32 ) * nParamCount;
			nParamInfoOffset = AlignUp( nParamInfoOffset, sizeof( void* ) );

			pParamInfo->nTotalParamSize = 0;
			pParamInfo->nParamCount = nParamCount;
			for( uint32 i = 0; i < nParamCount; i++ )
			{
				aryParamTypes[i] = UPakDataType( aryTypeInfo.aryInfo[i] );
				aryParamSizes[i] = (uint32)aryParamTypes[i].GetAligenSizeOfType();
				pParamInfo->nTotalParamSize += aryParamSizes[i];
			}
		}

		if( pClass->m_bCopyConstructor || pClass->m_bMoveConstructor )
		{
			pClass->m_aryConstructorParamInfo[nConstructorCount] = (uint32)nParamInfoOffset;
			auto pParamInfo = (SParamsInfo*)( pParamBuffer + nParamInfoOffset );
			auto aryParamTypes = (UPakDataType*)( pParamInfo + 1 );
			auto aryParamSizes = (uint32*)( aryParamTypes + 1 );
			nParamInfoOffset += nBuiltinSize;
			nParamInfoOffset = AlignUp( nParamInfoOffset, sizeof( void* ) );

			pParamInfo->nTotalParamSize = sizeof( void* );
			pParamInfo->nParamCount = 1;
			aryParamTypes[0] = UPakDataType( pClass, 0, true, true );
		}

		assert( nParamInfoOffset == nParamInfoTotalSize );
		return pClass;
	}

	const CClass* CReflection::RegisterClassBase(
		const char* szTypeIDName, uint32 nCount,
		const char** aryBaseType, const ptrdiff_t* aryOffset )
	{
		CName Key( szTypeIDName );
		assert( m_mapTypeID2ClassInfo.count( Key.GetIndex() ) );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass && pClass->GetBaseClassCount() == 0 );
		pClass->m_nBaseClassCount = nCount;
		if( pClass->m_nBaseClassCount )
			pClass->m_aryBaseClasses = new SClassOffset[nCount];

		for( uint32 i = 0; i < pClass->m_nBaseClassCount; i++ )
		{
			ptrdiff_t nOffset = aryOffset[i];
			CClass* pBase = DeclareClass( aryBaseType[i] );
			assert( nOffset >= 0 );
			SClassOffset BaseInfo = { pBase, nOffset };
			if( pBase->m_nInheritDepth + 1 > pClass->m_nInheritDepth )
				pClass->m_nInheritDepth = pBase->m_nInheritDepth + 1;
			pClass->m_aryBaseClasses[i] = BaseInfo;

			AppendArray( pBase->m_arySubClasses,
				pBase->m_nSubClassCount, { pClass, -BaseInfo.nOffset } );
			if( nOffset )
				continue;

			// Natural inheritance (offset 0), vtable should be inherited
			for( uint32 i = 0; i < pBase->m_nOverridableCount; i++ )
			{
				auto pBaseField = pBase->m_aryOverridableFun[i];
				auto nVirtualTableIndex = pBaseField->GetVirtualTableIndex();
				assert( !pClass->FindOverrideFun( nVirtualTableIndex ) );
				RegisterCallBack( szTypeIDName, pBaseField );
			}
		}
		return pClass;
	}

	const CField* CReflection::RegisterDestructor( const CFieldTags& Tag,
		DtorWrapFun funWrap, FunctionPointer funBoot,
		uint32 nFunIndex, const STypeInfoArray& aryTypeInfo )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		auto pNewField = new CField( Tag, pClass,
			funBoot, nFunIndex, funWrap, aryTypeInfo );
		RegisterCallBack( aryTypeInfo.aryInfo[0].szTypeName, pNewField );
		return pNewField;
	}

	const CField* CReflection::RegisterSmartPtr( const CFieldTags& Tag,
		GetWrapFun funPtrGet, const STypeInfoArray& aryTypeInfo )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		return new CField( Tag, pClass, funPtrGet, aryTypeInfo );
	}

	const CField* CReflection::RegisterClassMember( const CFieldTags& Tag,
		EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
		const STypeInfoArray& aryTypeInfo, const char* szMemberName )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		return new CField( Tag, pClass, szMemberName,
			Qualifier, funGet, funSet, aryTypeInfo );
	}

	const CField* CReflection::RegisterStaticMember( const CFieldTags& Tag,
		EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
		const STypeInfoArray& aryTypeInfo, const char* szMemberName, EFieldType eFieldType )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		return new CField( Tag, pClass, szMemberName,
			Qualifier, funGet, funSet, aryTypeInfo, eFieldType );
	}

	const CField* CReflection::RegisterClassFunction(
		const CFieldTags& Tag, CallWrapFun funWrap,
		const STypeInfoArray& aryTypeInfo, const char* szFunctionName )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		return new CField( Tag, pClass,
			szFunctionName, true, funWrap, aryTypeInfo );
	}

	const CField* CReflection::RegisterClassCallback(
		const CFieldTags& Tag, CallbackWrapFun funWrap,
		FunctionPointer funBoot, uint32 nFunIndex, bool bPureVirtual,
		const STypeInfoArray& aryTypeInfo, const char* szFunctionName )
	{
		CName Key( aryTypeInfo.aryInfo[0].szTypeName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		auto pNewField = new CField( Tag, pClass, szFunctionName,
			bPureVirtual, funBoot, nFunIndex, funWrap, aryTypeInfo );
		RegisterCallBack( aryTypeInfo.aryInfo[0].szTypeName, pNewField );
		return pNewField;
	}

	const CField* CReflection::RegisterGlobalFunction(
		const CFieldTags& Tag, const char* szNameSpace,
		CallWrapFun funWrap, const STypeInfoArray& aryTypeInfo,
		const char* szTypeInfoName, const char* szFunctionName )
	{
		szNameSpace = FormatNameSpace( szNameSpace );
		if( !szTypeInfoName || !szTypeInfoName[0] )
		{
			CName Key( szNameSpace );
			if( !m_mapTypeID2ClassInfo.count( Key.GetIndex() ) )
			{
				CClass* pClass = new CClass( szNameSpace );
				pClass->m_eRegisterType = eCRT_GlobalScope;
				m_mapTypeID2ClassInfo[Key.GetIndex()] = pClass;
				pClass->FinishRegister( szNameSpace, 0 );
			}
			szTypeInfoName = szNameSpace;
		}

		CName Key( szTypeInfoName );
		CClass* pClass = m_mapTypeID2ClassInfo[Key.GetIndex()];
		assert( pClass );
		return new CField( Tag, pClass,
			szFunctionName, false, funWrap, aryTypeInfo );
	}

	bool CReflection::IsAllocVirtualTable( void* pVirtualTable )
	{
		SFuncTableBuffer* pCurFuctionTable = m_pCurFuctionTable;
		do
		{
			void** pStart = (void**)pCurFuctionTable;
			void** pEnd = pStart + pCurFuctionTable->nCommitPtrCount;
			if( pVirtualTable >= pStart && pVirtualTable < pEnd )
				return true;
		} while( ( pCurFuctionTable = pCurFuctionTable->pNextBuffer ) );
		return false;
	}

	SFunctionTable* CReflection::GetOrgVirtualTable( SFunctionTable* pTable )
	{
		if( !IsAllocVirtualTable( pTable ) )
			return pTable;
		SFunTableHead* pFunTableHead = ( (SFunTableHead*)pTable ) - 1;
		return pFunTableHead->m_pOldFunTable;
	}

	SFunctionTable* CReflection::AllocVirtualTable(
		ICallbackHook* pHook, const CClass* pClass, SFunctionTable* pFunTable )
	{
		assert( !IsAllocVirtualTable( pFunTable ) );
		int32 nFunCount = CalculateFunctionCount( pFunTable );
		uint32 nArraySize = nFunCount + ePointerCount;

		m_AllocLock.lock();
		uint32 nUseCount = m_pCurFuctionTable->nUsePtrCount + (uint32)nArraySize;
		if( nUseCount > m_pCurFuctionTable->nCommitPtrCount )
		{
			if( nUseCount > eMaxVirtualFunCount )
			{
				AllocNewFuncTableBuffer();
				nUseCount = m_pCurFuctionTable->nUsePtrCount + (uint32)nArraySize;
			}

			uint32_t nPageSize = GetVirtualMemoryPageSize();
			uint32_t nPageFuctionCount = nPageSize / sizeof( void* );
			assert( nPageFuctionCount * sizeof( void* ) == nPageSize );

			void** pBufferStart = (void**)m_pCurFuctionTable;
			void* pCommitStart = pBufferStart + m_pCurFuctionTable->nCommitPtrCount;
			uint32_t nCommitEnd = AlignUp( nUseCount, nPageFuctionCount );
			uint32_t nCommitCount = nCommitEnd - m_pCurFuctionTable->nCommitPtrCount;
			CommitVirtualMemory( pCommitStart, nCommitCount * sizeof( void* ) );
			m_pCurFuctionTable->nCommitPtrCount = nCommitEnd;
		}

		void** pCurBufferStart = (void**)m_pCurFuctionTable;
		void** aryFun = pCurBufferStart + m_pCurFuctionTable->nUsePtrCount;
		m_pCurFuctionTable->nUsePtrCount += (uint32)nArraySize;
		m_AllocLock.unlock();

		auto pFunTableHead = (SFunTableHead*)aryFun;
		SFunctionTable* pNewFunTable = (SFunctionTable*)( pFunTableHead + 1 );
		memcpy( pNewFunTable->aryFun, pFunTable->aryFun, nFunCount * sizeof( void* ) );
		pNewFunTable->aryFun[nFunCount] = nullptr;
		pFunTableHead->m_pHook = pHook;
		pFunTableHead->m_pOldFunTable = pFunTable;
		pFunTableHead->m_pClass = pClass;
		for( uint32 i = 0; i < pClass->GetOverridableCount(); i++ )
		{
			auto pField = pClass->GetOverridableFun( i );
			assert( pField != nullptr );
			uint32 nIndex = pField->GetVirtualTableIndex();
			auto CallbackInfo = pField->m_FieldInfo.m_CallbackInfo;
			pNewFunTable->aryFun[nIndex] = (void*)CallbackInfo.m_funBoot;
		}
		return pNewFunTable;
	}

	void CReflection::FreeVirtualTable( SFunctionTable* pFunTable )
	{
		// VTable is not intended for reuse because objects may still be using it
		assert( IsAllocVirtualTable( pFunTable ) );
		SFunTableHead* pFunTableHead = ( (SFunTableHead*)pFunTable ) - 1;
		auto aryOldFun = pFunTableHead->m_pOldFunTable->aryFun;
		pFunTableHead->m_pClass = nullptr;
		pFunTableHead->m_pHook = nullptr;
		int32 nFunCount = CalculateFunctionCount( pFunTable );
		memcpy( pFunTable->aryFun, aryOldFun, nFunCount * sizeof( void* ) );
	}

	const CClass* CReflection::FindClassByClassID( uint32 nClassID ) const
	{
		auto pThis = const_cast<CReflection*>( this );
		std::lock_guard<std::recursive_mutex> Lock( GetClassRegisterMutex() );
		for( CClass* pPending : pThis->m_vecClassPending )
			pPending->FinishRegister( nullptr, 0 );
		pThis->m_vecClassPending.clear();

		auto it = m_mapClassID2ClassInfo.find( nClassID );
		return it != m_mapClassID2ClassInfo.end() ? it->second : nullptr;
	}

	const CClass* CReflection::FindClassByTypeID( CName TypeIDName ) const
	{
		auto it = m_mapTypeID2ClassInfo.find( TypeIDName.GetIndex() );
		return it != m_mapTypeID2ClassInfo.end() ? it->second : nullptr;
	}

	CClass* CReflection::FindClassByTypeIDMutable( CName TypeIDName )
	{
		auto it = m_mapTypeID2ClassInfo.find( TypeIDName.GetIndex() );
		return it != m_mapTypeID2ClassInfo.end() ? it->second : nullptr;
	}

	const char* CReflection::GetTypeReadableName( STypeInfo TypeInfo ) const
	{
		auto GetTypeName = [this]( STypeInfo TypeInfo )
		{
			auto eType = TypeInfo.eType;
			if( eType == eDT_char ) return "char";
			if( eType == eDT_int8 ) return "int8";
			if( eType == eDT_int16 ) return "int16";
			if( eType == eDT_int32 ) return "int32";
			if( eType == eDT_int64 ) return "int64";
			if( eType == eDT_long ) return "long";
			if( eType == eDT_uint8 ) return "uint8";
			if( eType == eDT_uint16 ) return "uint16";
			if( eType == eDT_uint32 ) return "uint32";
			if( eType == eDT_uint64 ) return "uint64";
			if( eType == eDT_ulong ) return "ulong";
			if( eType == eDT_wchar ) return "wchar_t";
			if( eType == eDT_char16 ) return "char16";
			if( eType == eDT_char32 ) return "char32";
			if( eType == eDT_bool ) return "bool";
			if( eType == eDT_float ) return "float";
			if( eType == eDT_double ) return "double";
			if( eType == eDT_const_char_str )
				return "const char*";
			assert( eType == eDT_enum || eType == eDT_class );
			auto pClass = FindClassByTypeID( TypeInfo.szTypeName );
			if( pClass == nullptr )
			{
				auto pThis = const_cast<CReflection*>( this );
				pClass = pThis->DeclareClass( TypeInfo.szTypeName );
			}

			return pClass->GetClassName().c_str();
		};

		const char* szTypeName = GetTypeName( TypeInfo );
		if( szTypeName == nullptr )
			return nullptr;
		char szFullName[2048];
		int nPos = 0;
		if( TypeInfo.bConstType )
			nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, "const " );
		nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, "%s", szTypeName );

		uint32 nPointerConstMask = TypeInfo.nPointerConstMask;
		for( uint32 i = 0; i < TypeInfo.nMultiPointers; i++ )
		{
			nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, "*" );
			if( nPointerConstMask & 1 )
				nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, " const" );
			nPointerConstMask = nPointerConstMask >> 1;
		}

		if( TypeInfo.nReference == 2 )
			nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, " &&" );
		else if( TypeInfo.nReference == 1 )
			nPos += snprintf( szFullName + nPos, sizeof( szFullName ) - nPos, " &" );
		if( !strcmp( szFullName, szTypeName ) )
			return szTypeName;
		return CName( szFullName ).c_str();
	}
};