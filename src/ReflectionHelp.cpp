#include "ReflectionHelp.h"
#include <stdlib.h>
#include <setjmp.h>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static uint32_t GetMemoryPageMask(const void* pAddress) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(pAddress, &mbi, sizeof(mbi))) return 0;
    uint32_t mask = 0;
    DWORD prot = mbi.Protect;
    if (prot & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))
        mask |= 0x04;
    if (prot & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))
        mask |= 0x02;
    if (prot != PAGE_NOACCESS) mask |= 0x01;
    return mask;
}
#else
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <mutex>
#ifdef __APPLE__
#include <mach/vm_map.h>
#include <mach/mach.h>
#endif

static uint32_t GetMemoryPageMask(const void* pAddress) {
    struct SPageInfo { uint64_t nSize : 60; uint64_t nMask : 4; };
    struct SMemoryPageMask
    {
        std::mutex m_MaskLock;
        std::map<const char*, SPageInfo> m_mapPageMask;

        SMemoryPageMask()
        {
#ifdef __linux__
            FILE* fp;
            char szBuffer[2048];
            snprintf(szBuffer, sizeof(szBuffer), "/proc/%d/maps", (int)getpid());
            if ((fp = fopen(szBuffer, "r")) != nullptr)
            {
                while (fgets(szBuffer, sizeof(szBuffer), fp))
                {
                    uint32_t nIndex = 0;
                    const char* szStart = szBuffer;
                    while (nIndex < sizeof(szBuffer) && szBuffer[nIndex] != '-')
                        nIndex++;
                    szBuffer[nIndex] = '\0';
                    const char* szEnd = szBuffer + (++nIndex);
                    while (nIndex < sizeof(szBuffer) && szBuffer[nIndex] != ' ')
                        nIndex++;
                    szBuffer[nIndex] = '\0';
                    const char* szMask = szBuffer + (++nIndex);
                    auto nStart = strtoull(szStart, nullptr, 16);
                    auto nEnd = strtoull(szEnd, nullptr, 16);
                    SPageInfo PageInfo = {};
                    PageInfo.nSize = nEnd - nStart;
                    if (szMask[0] == 'r')
                        PageInfo.nMask |= 0x01;
                    if (szMask[1] == 'w')
                        PageInfo.nMask |= 0x02;
                    if (szMask[2] == 'x')
                        PageInfo.nMask |= 0x04;
                    m_mapPageMask[(const char*)nStart] = PageInfo;
                }
                fclose(fp);
            }
#endif
        }

        uint32_t GetPageMask(const void* pAddress)
        {
            std::lock_guard<std::mutex> Lock(m_MaskLock);
            auto pKey = (const char*)pAddress;
            auto it = m_mapPageMask.upper_bound(pKey);
            if (it != m_mapPageMask.begin())
            {
                auto& PageMask = *(--it);
                if (pKey < PageMask.first + PageMask.second.nSize)
                    return (uint32_t)PageMask.second.nMask;
            }

#ifdef __APPLE__
            mach_port_t Task;
            vm_size_t nRegionSize = 0;
            vm_address_t pRegion = (vm_address_t)pAddress;
            vm_region_basic_info_data_64_t Info;
            mach_msg_type_number_t InfoCount = VM_REGION_BASIC_INFO_COUNT_64;
            if (vm_region_64(mach_task_self(), &pRegion, &nRegionSize,
                VM_REGION_BASIC_INFO_64, (vm_region_info_t)&Info,
                (mach_msg_type_number_t*)&InfoCount,
                (mach_port_t*)&Task) != KERN_SUCCESS)
                return 0;
            uint32_t nMask = 0;
            if (Info.protection & VM_PROT_READ)
                nMask |= 0x01;
            if (Info.protection & VM_PROT_WRITE)
                nMask |= 0x02;
            if (Info.protection & VM_PROT_EXECUTE)
                nMask |= 0x04;
            SPageInfo PageInfo = {};
            PageInfo.nSize = nRegionSize;
            PageInfo.nMask = nMask;
            m_mapPageMask.insert({ (const char*)pRegion, PageInfo });
            return nMask;
#else
            return 0;
#endif
        }
    };

    if (pAddress == nullptr)
        return 0;
    static SMemoryPageMask s_PageMasks;
    return s_PageMasks.GetPageMask(pAddress);
}
#define VIRTUAL_PAGE_EXECUTE 0x04
#endif

#if !defined(VIRTUAL_PAGE_EXECUTE)
#define VIRTUAL_PAGE_EXECUTE 0x04
#endif

#if defined(_MSC_VER)
#define NO_SANITIZE_ADDRESS __declspec(no_sanitize_address)
#elif defined(__clang__) || defined(__GNUC__)
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define NO_SANITIZE_ADDRESS
#endif
#include "CClass.h"
#include "CField.h"

#pragma warning( disable: 4611 )

namespace XReflect
{
	XREFLECT_API UPakDataType::UPakDataType( STypeInfo TypeInfo )
	{
		static auto& Reflection = CReflection::GetInstance();
		if( TypeInfo.eType < eDT_enum )
			m_eDataType = TypeInfo.eType;
		else if( TypeInfo.eType == eDT_enum )
			m_pClass = Reflection.DeclareEnumType( TypeInfo.szTypeName );
		else if( TypeInfo.eType == eDT_class )
			m_pClass = Reflection.DeclareClass( TypeInfo.szTypeName );
		assert( TypeInfo.nMultiPointers <= 3 );
		m_nPointers = TypeInfo.nMultiPointers;
		m_bReference = TypeInfo.nReference != 0;
		m_bConstant = TypeInfo.bConstType;
	}

	size_t UPakDataType::GetSizeOfType() const
	{
		static const size_t s_aryOrgSize[] =
		{
			0,
			sizeof(char),
			sizeof(int8),
			sizeof(int16),
			sizeof(int32),
			sizeof(int64),
			sizeof(long),
			sizeof(uint8),
			sizeof(uint16),
			sizeof(uint32),
			sizeof(uint64),
			sizeof(ulong),
			sizeof(wchar_t),
			sizeof(char16_t),
			sizeof(char32_t),
			sizeof(bool),
			sizeof(float),
			sizeof(double),
			sizeof(const char*),
			sizeof(void*)
		};

		if( GetMulPointer() )
			return sizeof( void* );
		if( !m_bClass )
			return s_aryOrgSize[m_eDataType];
		return GetClass()->GetClassSize();
	}

	size_t UPakDataType::GetAligenSizeOfType() const
	{
		if( !IsValue() )
			return sizeof( void* );
		if( !m_bClass )
			return m_eDataType == eDT_void ? 0 : sizeof( void* );
		return GetClass()->GetClassAlignSize();
	}

	static void NullFunCall()
	{ throw( "Can not call an invalid function!"); }

	void InitFunctionTable( void* pFun[], uint32 nCount )
	{
		for( uint32 i = 0; i < nCount; i++ )
			pFun[i] = (void*)&NullFunCall;
	}

	int32 CalculateFunctionCount( const SFunctionTable* pTable )
	{        
		for( int32 n = 0; n < MAX_VTABLE_SIZE; n++ )
		{
			uint32 nPageMask = GetMemoryPageMask( pTable->aryFun[n] );
			if( ((nPageMask) & (VIRTUAL_PAGE_EXECUTE )) != 0 )
				continue;
			return n;
		}
		return MAX_VTABLE_SIZE;
	}

	///< Virtual function table for getting function index
	struct SFunctionContext : public SFunctionTable { jmp_buf JumpFlag; };

	///< Build virtual table by template
	template<uint32 nStart, uint32 nCount>
	class TBuildTable
	{
		enum
		{ 
			nLStart = nStart, 
			nLCount = nCount/2, 
			nRStart = nLStart + nLCount, 
			nRCount = nCount - nLCount,
		};

	public:
		TBuildTable( SFunctionTable& FunctionTable )
		{
			TBuildTable<nLStart, nLCount> Left( FunctionTable );
			TBuildTable<nRStart, nRCount> Right( FunctionTable );
		}
	};

	template<uint32 nStart>
	class TBuildTable<nStart, 1>
	{	
		NO_SANITIZE_ADDRESS void GetIndex() 
		{
			SVirtualClassInstance* pObject = (SVirtualClassInstance*)this;
			auto pContext = (SFunctionContext*)pObject->pVirtualTable;
			longjmp( pContext->JumpFlag, nStart + 1 );
		}

	public:
		TBuildTable( SFunctionTable& FunctionTable )
		{
			auto funPointer = &TBuildTable<nStart, 1>::GetIndex;
			FunctionTable.aryFun[nStart] = *(void**)&funPointer;
		}
	};

	///< Find function index
	static NO_SANITIZE_ADDRESS uint32 FindVirtualFunctionImpl(
		uint32 nClassSize, VirtualFunCallback funCallback, void* pContext,
		SFunctionTable& FunctionTable )
	{
		SFunctionContext FunContext;
		uint32 nAllocSize = (uint32)AlignUp( (uint32)nClassSize, sizeof( void* ) );
		void** pObj = (void**)alloca( nAllocSize );
		for( uint32 i = 0; i < nAllocSize/sizeof(void*); i++ )
			pObj[i] = &FunContext;
		(SFunctionTable&)FunContext = FunctionTable;
		uint32 nIndex = setjmp( FunContext.JumpFlag );
		if( nIndex == 0 )
			funCallback( pObj, pContext );
		return nIndex - 1;
	}

	XREFLECT_API uint32 FindVirtualFunction( 
		uint32 nClassSize, VirtualFunCallback funCallback, void* pContext )
	{
		static SFunctionTable FunctionTable;
		static TBuildTable<0, MAX_VTABLE_SIZE> s_FunIndex( FunctionTable );
		return FindVirtualFunctionImpl( nClassSize, funCallback, pContext, FunctionTable );
	}

	XREFLECT_API bool IsMemoryCompatible( const UPakDataType* aryTargetType, 
		const UPakDataType* arySourceType, uint32 nParamTypeCount )
	{
		// Helper: determine if type is an integer-like type
		auto IsInteger = []( const UPakDataType& t ) -> bool
		{
			switch( t.GetDataType() )
			{
			case eDT_char: case eDT_int8: case eDT_int16: case eDT_int32: case eDT_int64:
			case eDT_long: case eDT_uint8: case eDT_uint16: case eDT_uint32: case eDT_uint64:
			case eDT_ulong: case eDT_bool: case eDT_enum:
				return true;
			default: return false;
			}
		};

		// Iterate parameters and check compatibility one by one
		for( uint32 i = 0; i < nParamTypeCount; i++ )
		{
			const UPakDataType& TargetType = aryTargetType[i];
			const UPakDataType& SourceType = arySourceType[i];

			if( TargetType.GetMulPointer() != SourceType.GetMulPointer() )
				return false;

			if( TargetType.IsClass() != SourceType.IsClass() )
				return false;

			if( TargetType.GetDataType() == eDT_const_char_str || 
				SourceType.GetDataType() == eDT_const_char_str )
			{
				if( TargetType.GetDataType() != SourceType.GetDataType() )
					return false;
				continue;
			}

			if( SourceType.IsConstant() == true && 
				TargetType.IsConstant() == false && 
				TargetType.IsReference() )
				return false;

			if( !TargetType.IsClass() )
			{
				if( TargetType.GetMulPointer() )
				{
					if( TargetType.GetDataType() == SourceType.GetDataType() )
						continue;
					return false;
				}

				if( TargetType.GetDataType() == SourceType.GetDataType() )
					continue;
				if( TargetType.GetSizeOfType() == SourceType.GetSizeOfType() &&
					IsInteger( TargetType ) && IsInteger( SourceType ) )
					continue;
				return false;
			}

			const CClass* pTargetClass = TargetType.GetClass();
			const CClass* pSrcClass = SourceType.GetClass();
			if( !pTargetClass || !pSrcClass )
				return false;

			if( pTargetClass == pSrcClass )
				continue;
			if( pSrcClass->IsA( pTargetClass ) )
				continue;
			if( !pTargetClass->IsSmartPtrClass() || 
				!pSrcClass->IsSmartPtrClass() )
				return false;
			if( pTargetClass->GetTemplateName() != pSrcClass->GetTemplateName() )
				return false;
			auto pTargetPtrField = pTargetClass->GetSmartPtrField();
			auto pSrcPtrField = pSrcClass->GetSmartPtrField();
			if( !pTargetPtrField || !pSrcPtrField )
				return false;
			auto TargetPtrType = pTargetPtrField->GetResultType();
			auto SrcPtrType = pSrcPtrField->GetResultType();
			if( IsMemoryCompatible( &TargetPtrType, &SrcPtrType, 1 ) )
				continue;
			return false;
		}
		return true;
	}

	XREFLECT_API const char* GetTypeReadableName( STypeInfo TypeInfo )
	{
		auto& Reflection = CReflection::GetInstance();
		return Reflection.GetTypeReadableName( TypeInfo );
	}

	const char* CFieldTags::GetGroupName() const
	{
		return m_nGroupTagCount ? m_aryGroupTags[0] : "";
	}

	const char* CFieldTags::GetGroupTagData( uint32 nIndex ) const
	{
		if( nIndex + 1 >= m_nGroupTagCount )
			return nullptr;
		return m_aryGroupTags[nIndex + 1];
	}

	const char* CFieldTags::GetGroupTagData( const char* szStartWith ) const
	{
		uint32 nLen = (uint32)strlen( szStartWith );
		for( uint32 i = 1; i < m_nGroupTagCount; i++ )
			if( !strncmp( m_aryGroupTags[i], szStartWith, nLen ) )
				return m_aryGroupTags[i];
		return nullptr;
	}

	const char* CFieldTags::GetFieldTagData( uint32 nIndex ) const
	{
		if( nIndex >= m_nFieldTagCount )
			return nullptr;
		return m_aryFieldTags[nIndex];
	}

	const char* CFieldTags::GetFieldTagData( const char* szStartWith ) const
	{
		uint32 nLen = (uint32)strlen( szStartWith );
		for( uint32 i = 0; i < m_nFieldTagCount; i++ )
			if( !strncmp( m_aryFieldTags[i], szStartWith, nLen ) )
				return m_aryFieldTags[i];
		return nullptr;
	}
}
