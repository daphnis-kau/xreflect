/**@file  		CReflection.h
* @brief		Reflection manager of c++ type
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>
#include "CClass.h"
#include "TemplateWrap.h"

namespace XReflect
{
	struct SFuncTableBuffer;
	typedef std::unordered_map<uint32, CClass*> CClassMap;

	class XREFLECT_API CReflection
	{
	public:
		struct SFunTableHead
		{
			ICallbackHook*	m_pHook;
			const CClass*	m_pClass;
			SFunctionTable*	m_pOldFunTable;
		};

	private:
		CClassMap			m_mapTypeID2ClassInfo;
		CClassMap			m_mapClassID2ClassInfo;
		std::vector<CClass*> m_vecClassPending;
		SFuncTableBuffer*	m_pCurFuctionTable;
		std::mutex			m_AllocLock;

		CReflection();
		~CReflection();
		friend union UPakDataType;
		friend class CClass;

		void				AllocNewFuncTableBuffer();
		CClass*				DeclareClass( const char* szTypeIDName );
		CClass*				DeclareEnumType( const char* szTypeIDName );
		CField*				RegisterCallBack( const char* szTypeInfoName, CField* pCallBackFun );
	public:
		static CReflection& GetInstance();

		const CClass*		RegisterEnumType( const char* szNameSpace,
								const char* szEnumType, uint32 nClassID,
								const char* szTypeIDName, int32 nTypeSize, bool bMask );
		const CField*		RegisterEnumValue(
								const CFieldTags& Tag, const char* szTypeIDName,
								const char* szEnumValue, int32 nValue );

		const CClass*		RegisterClass( const char* szTypeIDName, IClassDefinition* pDef );
		const CClass*		RegisterClassBase( const char* szTypeIDName, uint32 nCount,
								const char** aryBaseType, const ptrdiff_t* aryOffset );
		const CField*		RegisterDestructor( const CFieldTags& Tag,
								DtorWrapFun funWrap, FunctionPointer funOrg,
								uint32 nFunIndex, const STypeInfoArray& aryTypeInfo );
		const CField*		RegisterSmartPtr( const CFieldTags& Tag,
								GetWrapFun funPtrGet, const STypeInfoArray& aryTypeInfo );
		const CField*		RegisterClassMember( const CFieldTags& Tag,
								EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
								const STypeInfoArray& aryTypeInfo, const char* szMemberName );
		const CField*		RegisterStaticMember( const CFieldTags& Tag,
								EMemberQualifier Qualifier, GetWrapFun funGet, SetWrapFun funSet,
								const STypeInfoArray& aryTypeInfo, const char* szMemberName, EFieldType eFieldType );
		const CField*		RegisterClassFunction( const CFieldTags& Tag, CallWrapFun funWrap,
								const STypeInfoArray& aryTypeInfo, const char* szFunctionName );
		const CField*		RegisterClassCallback( const CFieldTags& Tag, CallbackWrapFun funWrap,
								FunctionPointer funBoot, uint32 nFunIndex, bool bPureVirtual,
								const STypeInfoArray& aryTypeInfo, const char* szFunctionName );
		const CField*		RegisterGlobalFunction( const CFieldTags& Tag, const char* szNameSpace,
								CallWrapFun funWrap, const STypeInfoArray& aryTypeInfo,
								const char* szTypeInfoName, const char* szFunctionName );

		bool				IsAllocVirtualTable( void* pVirtualTable );
		SFunctionTable*		GetOrgVirtualTable( SFunctionTable* pTable );
		SFunctionTable*		AllocVirtualTable( ICallbackHook* pHook,
								const CClass* pClass, SFunctionTable* pFunTable );
		void				FreeVirtualTable( SFunctionTable* pFunTable );

		const CClass*		FindClassByClassID( uint32 nClassID ) const;
		const CClass*		FindClassByTypeID( CName TypeIDName ) const;
		const char*			GetTypeReadableName( STypeInfo TypeInfo ) const;
		const CClassMap&	GetRegistClassMap() const { return m_mapTypeID2ClassInfo; }

		CClass*				FindClassByTypeIDMutable( CName TypeIDName );

		template<typename T>
		const CClass*		FindClassByTypeID() const;
	};

	template<typename T>
	inline const CClass* CReflection::FindClassByTypeID() const
	{
		TRegisterTypes<T>();
		static CName TypeIDName( TypeName<T>() );
		return FindClassByTypeID( TypeIDName );
	}
}
