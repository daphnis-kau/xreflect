/**@file  		CClass.h
* @brief		Register information of class
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once

#include "CName.h"
#include "ReflectionBase.h"
#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace XReflect
{
	class CField;
	class CClass;
	typedef std::unordered_map<uint32, CField*> CFieldMap;
	typedef bool (*FieldEnumFun)(const CField*, void*);
	typedef std::function<bool(const CField*)> CFieldEnumFun;
	struct SClassOffset { const CClass* pClass; ptrdiff_t nOffset; };

	struct SParamsInfo
	{
		uint32				nParamCount;
		uint32				nTotalParamSize;
		const UPakDataType*	GetParamType() const;
		const uint32*		GetParamSize() const;
	};

    class XREFLECT_API CClass final
	{
		typedef std::atomic<uint32> atomic_u32;

		EClassRegisterType	m_eRegisterType;			// EClassRegisterType
		CName				m_TypeIDName;				// typeid of the class( platform dependency )
		mutable	CName		m_TemplateName;				// Template name of class
		mutable CName		m_ClassName;				// Name of class
		mutable atomic_u32	m_nClassID;					// Hash value or class name or specify by user
		uint32				m_nSizeOfClass;				// Size of Class
		uint32				m_nClassAligenSize;			// Aligen size of class

		uint8				m_nInheritDepth;			// Inherit depth
		mutable uint8		m_nNameSpaceCount;			// Namespace depth
		uint8				m_nEnumPrefixLen;			// Prefix length of enumeration
		uint8				m_nCustomConstructorCount;	// Constructor Count
		uint8				m_bAnyPointerExist : 1;		// Any pointer field exists
		uint8				m_bDefaultConstructor : 1;	// Default Constructor is valid;
		uint8				m_bCopyConstructor : 1;		// Copy Constructor is valid;
		uint8				m_bMoveConstructor : 1;		// Right value reference Constructor is valid;
		uint8				m_bCopyAssignable : 1;		// Copy assignment is valid;
		uint8				m_bMoveAssignable : 1;		// Right value reference assignment is valid;
		uint8				m_bMaskEnum : 1;			// Enumeration can be used as mask
		uint8				m_bDisplayTag : 1;			// Field has display tag

		uint32				m_nBaseClassCount;			// Base classes count
		uint32				m_nSubClassCount;			// Subclasses count
		uint32				m_nOverridableCount;		// Overridable function count

		UPakDataType		m_ContainerElemType;		// Container element type
		SClassOffset*		m_aryBaseClasses;			// All base classes' information
		SClassOffset*		m_arySubClasses;			// All subclass information

		mutable CName*		m_aryNameSpace;				// NameSpace
		IClassDefinition*	m_pClassDefine;				// Definition of class
		uint32*				m_aryConstructorParamInfo;	// Parameter infos of constructors

		CField**			m_aryOverridableFun;		// All overridable function ( include base )
		CField*				m_pSmartPtrField;			// Smart pointer field
		CFieldMap			m_mapRegistFields;			// All Fields

		friend class CField;
		friend class CReflection;
		CClass( const char* szTypeIDName );
		~CClass( void );
		void* operator new( std::size_t nSize );
		void operator delete( void* ptr );

		void				FinishRegister( const char* szNameSpace, uint32 nClassID );
    public:
		IClassDefinition*	GetClassDefinition() const { return m_pClassDefine; }
		uint8				GetCustomConstructorCount() const { return m_nCustomConstructorCount; }
		bool				IsAnyPointerExist() const { return m_bAnyPointerExist; }
		bool				IsDefaultConstructible() const { return m_bDefaultConstructor; }
		bool				IsCopyConstructible() const { return m_bCopyConstructor; }
		bool				IsMoveConstructible() const { return m_bMoveConstructor; }
		const SParamsInfo*	GetConstructorParamsInfo( int32 n ) const;
		bool				IsCopyAssignable() const { return m_bCopyAssignable; }
		bool				IsMoveAssignable() const { return m_bMoveAssignable; }

		CName				GetTypeIDName() const { return m_TypeIDName; }
		EClassRegisterType	GetRegisterType() const { return m_eRegisterType; }
		uint8				GetInheritDepth() const { return m_nInheritDepth; }
		const CFieldMap&	GetRegistFields() const { return m_mapRegistFields; }
		CName				GetTemplateName() const;
		CName				GetClassName() const;
		uint32				GetClassID() const;
		uint8				GetNameSpaceCount() const;
		CName				GetNameSpace( uint8 nIndex ) const;
		UPakDataType		GetContainerElemType() const;

		uint32              GetClassSize() const { return m_nSizeOfClass; }
		uint32              GetClassAlignSize() const { return m_nClassAligenSize; }
		uint32				GetBaseClassCount() const { return m_nBaseClassCount; }
		const CClass*		GetBaseClass( uint32 n ) const { return m_aryBaseClasses[n].pClass; }
		ptrdiff_t			GetBaseClassOff( uint32 n ) const { return m_aryBaseClasses[n].nOffset; }
		uint32				GetSubClassCount() const { return m_nSubClassCount; }
		const CClass*		GetSubClass( uint32 n ) const { return m_arySubClasses[n].pClass; }
		ptrdiff_t			GetSubClassOff( uint32 n ) const { return m_arySubClasses[n].nOffset; }

		uint32              GetOverridableCount() const { return m_nOverridableCount; }
		const CField*		GetOverridableFun( uint32 n ) const { return m_aryOverridableFun[n]; }
		bool				IsEnum() const { return m_eRegisterType == eCRT_Enumeration; }
		bool				IsEnumMaskEnable() const { return IsEnum() && m_bMaskEnum; }
		uint8				GetEnumPrefixLen() const { return m_nEnumPrefixLen; }
		bool				HasDisplayTag() const { return m_bDisplayTag; }

		const CField*		GetSmartPtrField() const { return m_pSmartPtrField; }
		bool				IsSmartPtrClass() const { return m_pSmartPtrField != nullptr; }
		const CClass*		GetStrongPtrClass() const;
		bool				IsStrongPtrClass() const;
		bool				IsWeakPtrClass() const;

		const CField*		FindField( CName FieldName, bool bTraveBase = false ) const;
		void				EnumFields( uint32 nTypeMask,
								FieldEnumFun funEnum, void* pContext ) const;
		void				EnumFields( uint32 nTypeMask, CFieldEnumFun funEnum ) const;

		const CField*		FindOverrideFun( uint32 nVirtualTableIndex ) const;
		ptrdiff_t			CalculateBaseOffset( CName BaseClassTypeName ) const;
		ptrdiff_t			CalculateBaseOffset( const CClass* pBaseClass ) const;
		bool                IsA( CName ClassTypeName ) const;
        bool                IsA( const CClass* pClass ) const;
		bool				IsBaseOffset( ptrdiff_t nDiff ) const;

		void				Construct( int32 nConstructorIndex, void* pObject, void** aryArg ) const;
		void				Destruct( void* pObject ) const;
		void*				New( int32 nConstructorIndex, void** aryArg ) const;
		void				Delete( void* pObject ) const;
		void				Assign( void* pDest, void* pSrc ) const;
		void				Move( void* pDest, void* pSrc ) const;

		int64				GetContainerSize( void* pObject ) const;
		/** Read a value via field path traversal. Receiver is called with (pData, pContext) */
		bool				Fetch( void* pObject, SFieldPath FieldPath, void* pContext,
								void(*Receiver)( void* pData, void* pContext ) ) const;
		/** Read a value via field path traversal with a std::function callback */
		bool				Fetch( void* pObject, SFieldPath FieldPath,
								std::function<void(void*)> fun ) const;
		/** Read a value via field path traversal, returning typed T */
		template<class T>T	Fetch( void* pObject, SFieldPath FieldPath ) const;

		/** Write a value via field path traversal (Modify, InsertElem, or RemoveElem) */
		bool				Modify( void* pObject, void* pData, bool bIsXValue,
								SFieldPath FieldPath, EFieldOperateType eModifyType ) const;

		/** Replace the virtual table of pObj with one that routes through pHook */
		void				HookVirtualTable( void* pObj, ICallbackHook* pHook ) const;
		/** Restore the original virtual table of pObj */
		void				UnhookVirtualTable( void* pObj ) const;

    };
}

#include "CClass.inl"
