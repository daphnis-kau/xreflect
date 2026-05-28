/**@file  		IReflection.h
* @brief		Base interface of Reflection
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>
#include <stdexcept>
#include "CName.h"
#include "TypeParser.h"

// DLL export
#if defined(_WIN32) && defined(XREFLECT_DLL)
    #if defined(XREFLECT_EXPORTS)
        #define XREFLECT_API __declspec(dllexport)
    #else
        #define XREFLECT_API __declspec(dllimport)
    #endif
#else
    #define XREFLECT_API
#endif


#ifdef _WIN32
	#undef GetObject
	#undef RegisterClass
	#undef UnregisterClass
	#undef GetClassName
	#undef CreateSemaphore
	#undef SetPort
	#undef DrawText
	#undef DeleteFile
	#undef CreateEvent
	#undef min
	#undef max
#endif

namespace XReflect
{
	class CClass;
	class CField;
	class CReflection;
	class ICallbackHook;
	#define MAX_VTABLE_SIZE		512

	enum EFieldType
	{
		eFDT_Enumeration		= 0,
		eFDT_GlobalFunction		= 1,
		eFDT_ClassStaticFunction= 2,
		eFDT_StaticMember		= 3,
		eFDT_Member				= 4,
		eFDT_ClassFunction		= 5,
		eFDT_ClassCallBack		= 6,
		eFDT_ClassPureCallBack	= 7,
		eFDT_ClassDestructor	= 8,
	};

	enum EMemberQualifier 
	{ 
		eMBQ_None				= 0,
		eMBQ_DisableSerialize	= 1 << 0,
		eMBQ_DisableEdit		= 1 << 1,
		eMBQ_DisableWrite		= 1 << 2,
	};

	enum EClassRegisterType
	{
		eCRT_NormalClass		= 0x00,
		eCRT_GlobalScope		= 0x01,
		eCRT_Enumeration		= 0x02,
	};

	enum EFieldNodeType
	{
		eFNT_NormalField		= 0x0,
		eFNT_TypeCast		    = 0x1,
		eFNT_ElementIndex		= 0x2,
		eFNT_CustomFetcher		= 0x3,
	};
	
	enum EBuiltinConstructorIndex
	{
		eBCI_DefaultConstructor	= -3,
		eBCI_CopyConstructor	= -2,
		eBCI_MoveConstructor	= -1,
		eBCI_CustomBegin		= 0,
	};

	enum EFieldOperateType
	{
		eFOT_Modify				= 0,
		eFOT_InsertElem			= 1,
		eFOT_RemoveElem			= 2,
	};

	/**
	 * @brief 64-bit compact type descriptor
	 *
	 * Design:
	 * - Uses a 64-bit union to store complete type information (type + modifiers)
	 * - Zero-overhead type discrimination via bit-field layout
	 *
	 * Memory layout (LSB to MSB):
	 * Bit 0-1 : m_nPointers  (pointer depth 0-3)
	 * Bit 2   : m_bReference (is reference)
	 * Bit 3   : m_bConstant  (is const)
	 * Bit 4-63: data payload (class pointer or base type ID)
	 */
	union XREFLECT_API UPakDataType
	{
	private:
		const CClass* m_pClass;			// Full 64-bit pointer
	
		// Modifier + data bit-field layout (LSB order)
		struct
		{
			uint64 m_nPointers	: 2;	// Bit 0-1: pointer depth (0-3), max 3
			uint64 m_bReference : 1;	// Bit 2  : Is reference
			uint64 m_bConstant	: 1;	// Bit 3  : Is const
			uint64 m_eDataType	: 60;	// Bit 4-63: Base type ID (must be < 2^16 so high bits are 0)
		};
				   
		// Type discrimination: check if bits 16-63 are zero to distinguish base types from class types
		struct	   
		{		   
			uint64 m_nReserved	: 16;	// Bit 0-15: Modifier + low data bits
			uint64 m_bClass		: 48;	// Bit 16-63: Discriminator (0 = fundamental type, non-zero = class type)
		};

	public:
		UPakDataType( STypeInfo TypeInfo );
		UPakDataType() : m_pClass( nullptr ){};
		UPakDataType( const UPakDataType& Type ) : m_pClass( Type.m_pClass ) {};

		UPakDataType( const UPakDataType& Type, uint8 nMultPoints,
			bool bReference = false, bool bConstant = false )
			: UPakDataType( Type ) 
		{ m_nPointers = nMultPoints; m_bReference = bReference; m_bConstant = bConstant; }

		UPakDataType( const CClass* pClass, uint8 nMultPoints = 0,
			bool bReference = false, bool bConstant = false )
			: m_pClass( pClass )
		{ m_nPointers = nMultPoints; m_bReference = bReference; m_bConstant = bConstant; }

		UPakDataType( EDataType eType, uint8 nMultPoints = 0,
			bool bReference = false, bool bConstant = false )
			: m_eDataType( eType )
		{ m_nPointers = nMultPoints; m_bReference = bReference; m_bConstant = bConstant; }

		void				SetMulPointer( uint8 nPointers );
		void				SetReference( bool bReference );
		void				SetConstant( bool bConstant );
		bool				IsVoid() const;
		bool				IsBaseType() const;
		const CClass*		GetClass() const;
		EDataType			GetDataType() const;
		bool				IsEnum() const;
		bool				IsClass() const;
		bool				IsConstant() const;
		bool				IsValue() const;
		bool				IsReference() const;
		uint8				GetMulPointer() const;
		bool				IsClassValue() const;
		bool				IsClassReference() const;
		size_t				GetSizeOfType() const;
		size_t				GetAligenSizeOfType() const;
		UPakDataType		OverrideMulPointer( uint8 nMulPointer = 0 ) const;
		UPakDataType		OverrideReference( bool bReference = false ) const;
		UPakDataType		OverrideConstant( bool bConstant = false ) const;
		bool				operator == ( const UPakDataType& other ) const;
	};

	class IFieldNodeFetcher
	{
	public:
		virtual const CClass*	GetObjectClass() const = 0;
		virtual UPakDataType	GetFieldType() const = 0;
		virtual bool			GetFieldData( void* pObject, void* pResult ) const = 0;
		virtual bool			SetFieldData( void* pObject, void* pData ) const = 0;	
	};

	// Provides encapsulation for field access, including field information
	struct SFieldNode
	{
	private:
		union
		{
			const CField*		m_pField;
            const CClass*		m_pTargetClass;
			IFieldNodeFetcher*	m_pNodeFetcher;
			struct
			{
				uint64			m_eNodeType : 2;
				uint64			m_nIndex : 62;
			};
		};

	public:
		SFieldNode() : m_pField( nullptr ) {};

		SFieldNode( const CField* pField )
			: m_pField( pField )
		{
            m_eNodeType = eFNT_NormalField;
		}

		SFieldNode( IFieldNodeFetcher* pFetcher )
			: m_pNodeFetcher( pFetcher )
		{
			m_eNodeType = eFNT_CustomFetcher;
		}
        
        SFieldNode( const CClass* pTargetClass )
            : m_pTargetClass( pTargetClass )
        {
            m_eNodeType = eFNT_TypeCast;
        }

		SFieldNode( uint64 nIndex )
			: m_nIndex( nIndex )
			, m_eNodeType( eFNT_ElementIndex )
		{
		}

		EFieldNodeType GetType() const
		{
			return (EFieldNodeType)m_eNodeType;
		}

		const CField* GetField() const
		{
			if( m_eNodeType != eFNT_NormalField )
				return nullptr;
			return (const CField*)( ( (ptrdiff_t)m_pField ) & ~0x3LL );
		}

		IFieldNodeFetcher* GetNodeFetcher() const
		{
			if( m_eNodeType != eFNT_CustomFetcher )
				return nullptr;
			return (IFieldNodeFetcher*)( ( (ptrdiff_t)m_pNodeFetcher ) & ~0x3LL );
		}
        
        const CClass* GetTargetClass() const
        {
            if( m_eNodeType != eFNT_TypeCast )
                return nullptr;
            return (const CClass*)( ( (ptrdiff_t)m_pTargetClass ) & ~0x3LL );
        }

		uint64 GetIndex() const
		{
            if( m_eNodeType != eFNT_ElementIndex )
				return UINT64_MAX;
			return m_nIndex;
		}

		bool operator == ( const SFieldNode& rhs ) const
		{
			return m_pField == rhs.m_pField;
		}

		bool operator != ( const SFieldNode& rhs ) const
		{
			return m_pField != rhs.m_pField;
		}
	};

	// Provides encapsulation for field access paths and path comparison functions
	struct SFieldPath { const SFieldNode* aryPath; uint32 nNodeCount; };
	inline bool operator< ( const SFieldPath& Left, const SFieldPath& Right )
	{
		uint32 nLeftLen = Left.nNodeCount;
		uint32 nRightLen = Right.nNodeCount;
		uint32 nCompareSize = sizeof( SFieldNode )*std::min( nLeftLen, nRightLen );
		int32 nResult = memcmp( Left.aryPath, Right.aryPath, nCompareSize );
		return nResult ? ( nResult < 0 ) : ( nLeftLen < nRightLen );
	};

	///< Find virtual function index in virtual table
	typedef void( *VirtualFunCallback )( void*, void* );

	/**@struct Define virtual table
	 * @brief Assume C++ Object structure as blow
	 * (Depend on compiler, such as MSVC/GCC/Clang)
	 * pObj-> -----------------
	 *       | SFunctionTable* |  -> --------------
	 *        -----------------     |  FunMember1  |
	 *       |   DataMember1   |    |  FunMember2  |
	 *       |   DataMember2   |    |  FunMember3  |
	 *       |   DataMember3   |    |  FunMember4  |
	 *       |   DataMember4   |    |  FunMember5  |
	 *       |   DataMember5   |    | ...........  |
	 *       |   ...........   |     --------------
	 *        -----------------
	 */
	struct SFunctionTable
	{
		void* aryFun[MAX_VTABLE_SIZE];
	};

	struct SVirtualClassInstance	
	{
		SFunctionTable* pVirtualTable;
	};

	struct STypeInfoArray
	{
		const STypeInfo* aryInfo = 0;
		uint32 nInfoCount = 0;
	};

	struct STagArray
	{
		const char* const* aryTags = nullptr;
		uint32 nTagCount = 0;
	};

	class XREFLECT_API CFieldTags
	{
		const char* const*		m_aryGroupTags = 0;
		const char* const*		m_aryFieldTags = 0;
		uint32					m_nGroupTagCount = 0;
		uint32					m_nFieldTagCount = 0;
	public:
		constexpr CFieldTags( STagArray GroupTags, STagArray FieldTags )
            : m_aryGroupTags( GroupTags.aryTags )
            , m_aryFieldTags( FieldTags.aryTags )
            , m_nGroupTagCount( GroupTags.nTagCount )
            , m_nFieldTagCount( FieldTags.nTagCount )
        {}
		const char* 			GetGroupName() const;
		const char* 			GetGroupTagData( uint32 nIndex ) const;
		const char* 			GetGroupTagData( const char* szStartWith ) const;
		const char* 			GetFieldTagData( uint32 nIndex ) const;
		const char* 			GetFieldTagData( const char* szStartWith ) const;
	};

	class IClassDefinition
	{
	public:
		virtual const char*		GetNameSpace() = 0;
		virtual const char*		GetTemplateName() = 0;
		virtual uint32			GetTemplateTypeCount() = 0;
		virtual const char*		GetTemplateParamter( uint32 n ) = 0;
		virtual STypeInfoArray	GetTemplateParamters() = 0;
		virtual STypeInfo		GetContainerElemType() = 0;
		virtual const char*		GetClassName() = 0;
		virtual bool			IsDefaultConstructible() = 0;
		virtual bool			IsCopyConstructible() = 0;
		virtual bool			IsMoveConstructible() = 0;
		virtual uint32			GetCustomConstructorCount() = 0;
		virtual STypeInfoArray	GetCustomConstructorParam( uint32 n ) = 0;
		virtual bool			IsCopyAssignable() = 0;
		virtual bool			IsMoveAssignable() = 0;
		virtual uint32			GetClassID() = 0;
		virtual uint32			GetClassSize() = 0;
		
		virtual void			Assign( void* pDest, void* pSrc ) = 0;
		virtual void			Move( void* pDest, void* pSrc ) = 0;
		virtual void			Construct( int32 nIndex, void* pObj, void** aryArg ) = 0;
		virtual void			Destruct( void* pObj ) = 0;
		virtual void*			New( int32 nIndex, void** aryArg ) = 0;
		virtual void			Delete( void* pObj ) = 0;
	};

	class ICallbackHook
	{
	public:
		virtual SFunctionTable*	FindHookVirtualTable( const CClass* pClass, 
									SFunctionTable* pOldFunTable ) = 0;
		virtual void			SetHookVirtualTable( const CClass* pClass, 
									SFunctionTable* pOldFunTable, 
									SFunctionTable* pNewFunTable ) = 0;
		virtual bool			OnCallBack( const CField* pField, 
									void* pRetBuf, void** pArgArray ) = 0;
		virtual void			OnDestruct( const CField* pField, void* pObj ) = 0;
	};

	typedef void (*FunctionPointer)();
	typedef void (*DtorWrapFun)( void* pObject );
	typedef void (*SetWrapFun)( void* pObject, void* pData );
	typedef void (*GetWrapFun)( void* pObject, void* pResult );
	typedef void (*CallWrapFun)( void* pRetBuf, void** pArgArray );
	typedef void (*CallbackWrapFun)( void* pRetBuf, 
		void** pArgArray, FunctionPointer funOrg );

	XREFLECT_API uint32 FindVirtualFunction( 
		uint32 nClassSize, VirtualFunCallback funCallback, void* pContext );
	XREFLECT_API bool IsMemoryCompatible( const UPakDataType* aryTargetType, 
		const UPakDataType* arySourceType, uint32 nParamTypeCount );
	XREFLECT_API const char* GetTypeReadableName( STypeInfo TypeInfo );

	#define ITERATOR_INC			"Inc"
	#define ITERATOR_DEC 			"Dec"
	#define ITERATOR_ADD			"Add"
	#define ITERATOR_SUB 			"Sub"
	#define ITERATOR_EQUAL			"Equal"
	#define ITERATOR_NOTEQUAL		"NotEqual"
	#define ITERATOR_ASSIGN			"Assign"
	#define ITERATOR_VALUE			"Value"

	#define CPP_CONTAINER_SIZE		"size"
	#define CPP_CONTAINER_BEGIN		"begin"
	#define CPP_CONTAINER_END		"end"
	#define CPP_CONTAINER_R_BEGIN	"rbegin"
	#define CPP_CONTAINER_R_END		"rend"
	#define CPP_CONTAINER_ERASE		"erase"
	#define CPP_CONTAINER_INSERT	"insert"

	#define ARRAY_ELEM_GET			"GetElementAt"
	#define ARRAY_ELEM_SET			"SetElementAt"

	#define DISPLAY_TAG				"Display"

	// UPakDataType design verification (after class definition)
	// Ensure 64-bit compact storage
	static_assert(sizeof(UPakDataType) == 8, "UPakDataType must be exactly 64 bits");
	static_assert(sizeof(void*) == 8, "This design requires 64-bit pointers");
}

