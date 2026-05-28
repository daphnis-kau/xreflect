/**@file  		CField.h
* @brief		Call each other between C++&Script
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#pragma once
#include "CClass.h"
#include "CReflection.h"
#include "CName.h"
#include <utility>

namespace XReflect
{
	class CClass;
	class CField;
	class CReflection;

    //=====================================================================
    // Information of c++ function
    //=====================================================================
    class XREFLECT_API CField final
	{
		friend class CClass;
		friend class CReflection;
		const CField& operator= ( const CField& );

		// Class virtual function( Callback )
		CField( const CFieldTags& Tag, CClass* pClass, const char* szFieldName,
			bool bPureVirtual, FunctionPointer funBoot, int32 nFunIndex,
			CallbackWrapFun funWrap, const STypeInfoArray& aryTypeInfo );
		// Class normal/static function or global function
		CField( const CFieldTags& Tag, CClass* pClass,
			const char* szFieldName, bool bThisCall,
			CallWrapFun funWrap, const STypeInfoArray& aryTypeInfo );
		// Class destructor
		CField( const CFieldTags& Tag, CClass* pClass, FunctionPointer funBoot,
			int32 nFunIndex, DtorWrapFun funWrap, const STypeInfoArray& aryTypeInfo );
		// Smart pointer
		CField( const CFieldTags& Tag, CClass* pClass,
			GetWrapFun funPtrGet, const STypeInfoArray& aryTypeInfo );
		// Class member
		CField( const CFieldTags& Tag, CClass* pClass, const char* szFieldName,
			EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
			const STypeInfoArray& aryTypeInfo );
		// Class static member / static property
		CField( const CFieldTags& Tag, CClass* pClass, const char* szFieldName,
			EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
			const STypeInfoArray& aryTypeInfo, EFieldType eFieldType );
		// Enumeration
		CField( const CFieldTags& Tag, CClass* pClass,
			const char* szFieldName, int32 nEnumValue );
		~CField();

		template<bool bReference> struct TMemberFetch
		{
			template<typename Type>
			static Type		Get( void* pObject, const CField* pField );
		};

		template<> struct TMemberFetch<true>
		{
			template<typename Type>
			static Type		Get( void* pObject, const CField* pField );
		};

		union UCallbackWrap
		{
			CallbackWrapFun m_funCallbackWrap;
			DtorWrapFun		m_funDestructorWrap;
		};

		struct SCallbackInfo
		{
			FunctionPointer	m_funBoot;
			UCallbackWrap	m_CallbackWrap;
		};

		struct SMemberInfo
		{
			GetWrapFun		m_funGet;
			SetWrapFun		m_funSet;
		};

		union UFieldInfo
		{
			SCallbackInfo	m_CallbackInfo;
			SMemberInfo		m_MemberInfo;
			CallWrapFun		m_funCall;
		};

		void				Init( const STypeInfoArray& aryTypeInfo );
	protected:
		CName				m_FieldName;
		EFieldType			m_eFieldType;
		CClass*				m_pClass;
		const CFieldTags&	m_Tag;

		UFieldInfo			m_FieldInfo;
		int32				m_nRegisterValue;

		uint32				m_nReturnSize;
		uint32				m_nParamCount;
		uint32				m_nTotalParamSize;
		uint32*				m_aryParamSize;
		UPakDataType*		m_aryParamType;
		UPakDataType		m_nResult;
    public:
		void				CallBack( void* pRetBuf, void** pArgArray ) const;
		void				Call( void* pRetBuf, void** pArgArray ) const;
		/** Raw void* read (low-level).
		 *  @param pObject Object to read from
		 *  @param pResult Buffer for the result. For addressable fields
		 *         (direct members), pass a pointer to the value type.
		 *         For non-addressable fields (properties, bitfields),
		 *         pass a buffer address directly.
		 *  @warning Caller must understand the field access semantics.
		 *           Prefer the type-safe Get<ClassType, Type>() template. */
		void				GetData( void* pObject, void* pResult ) const;
		/** Raw void* write (low-level). See GetData warning. */
		void				SetData( void* pObject, void* pData ) const;

		/** Type-safe read (recommended). Deduces reference/value semantics at compile time.
		 *  @tparam ClassType The owning class type
		 *  @tparam Type      The value type to read
		 *  @param  pObject   Object to read from
		 *  @return           The field value */
		template<typename ClassType, typename Type>
		Type				Get( ClassType* pObject ) const;
		/** Type-safe write (recommended). */
		template<typename ClassType, typename Type>
		void				Set( ClassType* pObject, const Type& v ) const;

		CName				GetName()						const;
		EFieldType			GetType()						const;
		const CFieldTags&	GetTag()						const;
		uint32				GetParamCount()					const;
		UPakDataType		GetParamType( uint32 n )		const;
		uint32				GetParamSize( uint32 n )		const;
		uint32				GetParamTotalSize()				const;
		UPakDataType		GetResultType()					const;
		uint32				GetResultSize()					const;
		int32				GetVirtualTableIndex()			const;
		EMemberQualifier	GetMemberQualifier()			const;
		bool				IsSmartFieldPtr()				const;
		int32				GetEnumValue()					const;
		const CClass*		GetClass()						const;
		const char*			GetDisplayName( bool bTagOnly )	const;

	};
}

#include "CField.inl"
