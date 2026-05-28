/**@file  		ReflectionMacro.h
* @brief		Registration macros for xreflect
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once

#include "ReflectionWrap.h"

#pragma warning(disable: 4624)
#pragma warning(disable: 4510)
#pragma warning(disable: 4610)

#define MAKE_UNIQUE_IDENTIFY( prefix, line, postfix )	\
	prefix##line##postfix
#define DEFINE_UNIQUE_IDENTIFY( prefix, line, postfix )	\
	MAKE_UNIQUE_IDENTIFY( prefix, line, postfix )


#define DEFINE_CLASS_BEGIN_IMPLEMENT( _namespace, _class ) \
	namespace _class##_scope { \
	using OrgClass = _namespace::_class; \
	using VTable = ::XReflect::SFunctionTable;\
	static const char* GetOrgNameSpace() { return #_namespace; }\
	static const char* GetOrgClassName() { return #_class; }\
	struct _begin_class : public ::XReflect::TConstructWrap<OrgClass, false> \
	{\
		static constexpr const char* Tags[] = { #_class }; \
		static constexpr ::XReflect::STagArray GroupTag = { Tags, std::size( Tags ) }; \
		static constexpr uint32 ClassID = 0; \
		static constexpr ::XReflect::STypeInfo ElemType = ::XReflect::GetTypeInfo<void>(); \
		static constexpr bool AllowRegisterConstruct = true; \
		static constexpr uint32 GetCustomConstructorCount() { return 0; } \
		static ::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 ) \
		{ return ::XReflect::STypeInfoArray(); }\
		using ConstructionTypes = ::XReflect::TConstructionTypes<>; \
		_begin_class( ConstructionTypes(), VTable*& pVTable, bool bNeedVTable, void** aryArg ){}\
		struct Register { Register() {} }; \
	};\
	typedef _begin_class


#define DEFINE_CLASS_END_IMPLEMENT() _last;\
	struct _end_class : public _last \
	{ \
		static constexpr bool bConstructable = std::is_convertible<_end_class*, OrgClass*>::value; \
		template<typename ...T> _end_class( T...p ) : _last( p... ){} \
		struct RegisterClass : public ::XReflect::TObjectConstruct<_end_class>\
		{\
			const char* GetNameSpace() { return GetOrgNameSpace(); }\
			const char* GetTemplateName() { return nullptr; } \
			uint32 GetTemplateTypeCount() { return 0; } \
			const char* GetTemplateParamter( uint32 n ){ return nullptr; } \
			::XReflect::STypeInfoArray GetTemplateParamters() { return ::XReflect::STypeInfoArray(); }\
			::XReflect::STypeInfo GetContainerElemType() { return _end_class::ElemType; } \
			const char* GetClassName() { return GetOrgClassName(); }\
			uint32 GetClassID() { return _end_class::ClassID; }\
			uint32 GetClassSize() { return sizeof(OrgClass); }\
			bool IsDefaultConstructible() { return bConstructable && std::is_default_constructible_v<_end_class>; } \
			bool IsCopyConstructible() { return bConstructable && std::is_copy_constructible_v<_end_class>; } \
			bool IsMoveConstructible() { return bConstructable && std::is_move_constructible_v<_end_class>; } \
			uint32 GetCustomConstructorCount() { return _end_class::GetCustomConstructorCount(); }\
			::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 n ) { return _end_class::GetCustomConstructorParam( n ); }\
			bool IsCopyAssignable() { return std::is_copy_assignable_v<_end_class>; } \
			bool IsMoveAssignable() { return std::is_move_assignable_v<_end_class>; } \
			RegisterClass()\
			{\
				auto& Reflection = ::XReflect::CReflection::GetInstance(); \
				Reflection.RegisterClass( ::XReflect::TypeName<OrgClass>(), this );\
			}\
		};\
		\
		struct Register : public RegisterClass, public _last::Register {};\
	};\
	static _end_class::Register Register; }


#define DEFINE_TEMPLATE_BEGIN_IMPLEMENT( _class ) \
	using VTable = ::XReflect::SFunctionTable;\
	static constexpr const char* szTemplateName = #_class; \
	struct _begin_class : public ::XReflect::TConstructWrap<OrgClass, false> \
	{\
		static constexpr const char* Tags[] = { #_class }; \
		static constexpr ::XReflect::STagArray GroupTag = { Tags, std::size( Tags ) }; \
		static constexpr uint32 ClassID = 0; \
		static constexpr ::XReflect::STypeInfo ElemType = ::XReflect::GetTypeInfo<void>(); \
		static constexpr bool AllowRegisterConstruct = true; \
		static constexpr uint32 GetCustomConstructorCount() { return 0; } \
		static ::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 ) \
		{ return ::XReflect::STypeInfoArray(); }\
		using ConstructionTypes = ::XReflect::TConstructionTypes<>; \
		struct Register { Register() { RegisterTemplateParam(); } }; \
		_begin_class( ConstructionTypes(), VTable*& pVTable, bool bNeedVTable, void** aryArg ){}\
	};\
	typedef _begin_class

#define DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM_IMPLEMENT(_namespace, _class, T0) \
	template<T0 P0> \
	struct ::XReflect::TTypeRegister<_namespace::_class<P0>> { \
	static const char* GetOrgClassName() \
	{ \
		return ::XReflect::CName( ( std::string(#_class "<") + \
			::XReflect::MakeTypeName<P0>(true) + ">" ).c_str() ).c_str();\
	}\
	static void RegisterTemplateParam()\
	{ \
		::XReflect::RegisterType<P0>(); \
	} \
	static CName GetParamTypeName( uint32 n ) \
	{\
		static CName s_aryTypeName[] = { \
			::XReflect::MakeTypeName<P0>(false) \
		}; \
		if( n >= 1 ) return CName(); \
		return s_aryTypeName[0]; \
	}\
	static ::XReflect::STypeInfoArray GetParamTypeNames() \
	{\
		static STypeInfo aryInfo[] = { GetTypeInfo<P0>() }; \
		return { aryInfo, std::size( aryInfo ) }; \
	}\
	static const uint32 s_TemplateTypeCount = 1;\
	using OrgClass = _namespace::_class<P0>; \
	static const char* GetOrgNameSpace() { return #_namespace; }\
	DEFINE_TEMPLATE_BEGIN_IMPLEMENT( _class )

#define DEFINE_TEMPLATE_BEGIN_WITH_2_PARAMS_IMPLEMENT(_namespace, _class, T0, T1) \
	template<T0 P0, T1 P1> \
	struct ::XReflect::TTypeRegister<_namespace::_class<P0, P1>> { \
	static const char* GetOrgClassName() \
	{ \
		return ::XReflect::CName( ( std::string(#_class "<") + \
			::XReflect::MakeTypeName<P0>(true) + "," + \
			::XReflect::MakeTypeName<P1>(true) + ">" ).c_str() ).c_str();\
	}\
	static void RegisterTemplateParam()\
	{ \
		::XReflect::RegisterType<P0>(); \
		::XReflect::RegisterType<P1>(); \
	} \
	static CName GetParamTypeName( uint32 n ) \
	{\
		static CName s_aryTypeName[] = { \
			::XReflect::MakeTypeName<P0>(false), \
			::XReflect::MakeTypeName<P1>(false) \
		}; \
		if( n >= 2 ) return CName(); \
		return s_aryTypeName[n]; \
	}\
	static ::XReflect::STypeInfoArray GetParamTypeNames() \
	{\
		static STypeInfo aryInfo[] = { GetTypeInfo<P0>(), GetTypeInfo<P1>() }; \
		return { aryInfo, std::size( aryInfo ) }; \
	}\
	static const uint32 s_TemplateTypeCount = 2;\
	using OrgClass = _namespace::_class<P0, P1>; \
	static const char* GetOrgNameSpace() { return #_namespace; }\
	DEFINE_TEMPLATE_BEGIN_IMPLEMENT( _class )

#define DEFINE_TEMPLATE_BEGIN_WITH_3_PARAMS_IMPLEMENT(_namespace, _class, T0, T1, T2) \
	template<T0 P0, T1 P1, T2 P2> \
	struct ::XReflect::TTypeRegister<_namespace::_class<P0, P1, P2>> { \
	static const char* GetOrgClassName() \
	{ \
		return ::XReflect::CName( ( std::string(#_class "<") + \
			::XReflect::MakeTypeName<P0>(true) + "," + \
			::XReflect::MakeTypeName<P1>(true) + "," + \
			::XReflect::MakeTypeName<P2>(true) + ">" ).c_str() ).c_str();\
	}\
	static void RegisterTemplateParam()\
	{ \
		::XReflect::RegisterType<P0>(); \
		::XReflect::RegisterType<P1>(); \
		::XReflect::RegisterType<P2>(); \
	} \
	static CName GetParamTypeName( uint32 n ) \
	{\
		static CName s_aryTypeName[] = { \
			::XReflect::MakeTypeName<P0>(false), \
			::XReflect::MakeTypeName<P1>(false), \
			::XReflect::MakeTypeName<P2>(false) \
		}; \
		if( n >= 3 ) return CName(); \
		return s_aryTypeName[n]; \
	}\
	static ::XReflect::STypeInfoArray GetParamTypeNames() \
	{\
		static STypeInfo aryInfo[] = { GetTypeInfo<P0>(), \
			GetTypeInfo<P1>(), GetTypeInfo<P2>() }; \
		return { aryInfo, std::size( aryInfo ) }; \
	}\
	static const uint32 s_TemplateTypeCount = 3;\
	using OrgClass = _namespace::_class<P0, P1, P2>; \
	static const char* GetOrgNameSpace() { return #_namespace; }\
	DEFINE_TEMPLATE_BEGIN_IMPLEMENT( _class )

#define DEFINE_TEMPLATE_BEGIN_WITH_4_PARAMS_IMPLEMENT(_namespace, _class, T0, T1, T2, T3) \
	template<T0 P0, T1 P1, T2 P2, T3 P3> \
	struct ::XReflect::TTypeRegister<_namespace::_class<P0, P1, P2, P3>> { \
	static const char* GetOrgClassName() \
	{ \
		return ::XReflect::CName( ( std::string(#_class "<") + \
			::XReflect::MakeTypeName<P0>(true) + "," + \
			::XReflect::MakeTypeName<P1>(true) + "," + \
			::XReflect::MakeTypeName<P2>(true) + "," + \
			::XReflect::MakeTypeName<P3>(true) + ">" ).c_str() ).c_str();\
	}\
	static void RegisterTemplateParam()\
	{ \
		::XReflect::RegisterType<P0>(); \
		::XReflect::RegisterType<P1>(); \
		::XReflect::RegisterType<P2>(); \
		::XReflect::RegisterType<P3>(); \
	} \
	static CName GetParamTypeName( uint32 n ) \
	{\
		static CName s_aryTypeName[] = { \
			::XReflect::MakeTypeName<P0>(false), \
			::XReflect::MakeTypeName<P1>(false), \
			::XReflect::MakeTypeName<P2>(false), \
			::XReflect::MakeTypeName<P3>(false) \
		}; \
		if( n >= 4 ) return CName(); \
		return s_aryTypeName[n]; \
	}\
	static ::XReflect::STypeInfoArray GetParamTypeNames() \
	{\
		static STypeInfo aryInfo[] = { \
			GetTypeInfo<P0>(), GetTypeInfo<P1>(), \
			GetTypeInfo<P2>(), GetTypeInfo<P3>() }; \
		return { aryInfo, std::size( aryInfo ) }; \
	}\
	static const uint32 s_TemplateTypeCount = 4;\
	using OrgClass = _namespace::_class<P0, P1, P2, P3>; \
	static const char* GetOrgNameSpace() { return #_namespace; }\
	DEFINE_TEMPLATE_BEGIN_IMPLEMENT( _class )


#define DEFINE_TEMPLATE_END_IMPLEMENT() _last;\
	struct _end_class : public _last \
	{ \
		static constexpr bool bConstructable = std::is_convertible<_end_class*, OrgClass*>::value; \
		template<typename ...T> _end_class( T...p ) : _last( p... ){} \
		struct RegisterClass : public ::XReflect::TObjectConstruct<_end_class>\
		{\
			const char* GetTemplateName() { return szTemplateName; } \
			const char* GetNameSpace() { return GetOrgNameSpace(); }\
			uint32 GetTemplateTypeCount() { return s_TemplateTypeCount; } \
			const char* GetTemplateParamter( uint32 n ){ return GetParamTypeName( n ).c_str(); } \
			::XReflect::STypeInfoArray GetTemplateParamters() { return GetParamTypeNames(); }\
			::XReflect::STypeInfo GetContainerElemType() { return _end_class::ElemType; } \
			const char* GetClassName() { return GetOrgClassName(); }\
			uint32 GetClassID() { return _end_class::ClassID; }\
			uint32 GetClassSize() { return sizeof(OrgClass); }\
			bool IsDefaultConstructible() { return bConstructable && std::is_default_constructible_v<_end_class>; } \
			bool IsCopyConstructible() { return bConstructable && std::is_copy_constructible_v<_end_class>; } \
			bool IsMoveConstructible() { return bConstructable && std::is_move_constructible_v<_end_class>; } \
			uint32 GetCustomConstructorCount() { return _end_class::GetCustomConstructorCount(); }\
			::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 n ) { return _end_class::GetCustomConstructorParam( n ); }\
			bool IsCopyAssignable() { return std::is_copy_assignable_v<_end_class>; } \
			bool IsMoveAssignable() { return std::is_move_assignable_v<_end_class>; } \
		};\
		\
		struct Register : public RegisterClass \
		{ \
			Register()\
			{\
				auto& Reflection = ::XReflect::CReflection::GetInstance(); \
				if( !Reflection.RegisterClass( ::XReflect::TypeName<OrgClass>(), this ) ) \
					return; \
				typename _last::Register LastRegister; \
			}\
		};\
	};\
	static void Register() \
	{ \
		static bool bRegister = false; \
		if( bRegister ) \
			return; \
		bRegister = true; \
		static typename _end_class::Register Register; \
	}\
	};

#define REGIST_ARRAY_IMPLEMENT( _elem_type, _lambda_size, _lambda_get, _lambda_set, ... ) \
	Array##_Base_Class; \
	struct Array##_class : public Array##_Base_Class \
	{ \
		using _elem_value_type = std::remove_reference_t<_elem_type>;\
		struct TOrgClass : public OrgClass \
		{ \
			size_t SizeImp() \
			{ \
				auto funImpl = _lambda_size; \
				return funImpl(); \
			}\
			_elem_type GetElemImp( size_t nIndex ) \
			{ \
				auto funImpl = _lambda_get; \
				return funImpl( nIndex ); \
			}\
			void SetElemImp( size_t nIndex, const _elem_value_type& v ) \
			{ \
				auto funImpl = _lambda_set; \
				funImpl( nIndex, v ); \
			}\
			static size_t Size( OrgClass* pThis ) \
			{ \
				return ((TOrgClass*)pThis)->SizeImp(); \
			}\
			static _elem_type GetElem( OrgClass* pThis, size_t nIndex ) \
			{ \
				return ((TOrgClass*)pThis)->GetElemImp( nIndex ); \
			}\
			static void SetElem( OrgClass* pThis, size_t nIndex, const _elem_value_type& v ) \
			{ \
				((TOrgClass*)pThis)->SetElemImp( nIndex, v ); \
			}\
		};  \
		static constexpr auto funSize = &TOrgClass::Size; \
		static constexpr auto funGetElem = &TOrgClass::GetElem; \
		static constexpr auto funSetElem = &TOrgClass::SetElem; \
		static constexpr ::XReflect::STypeInfo ElemType = ::XReflect::GetTypeInfo<_elem_type>();\
		template<typename ...T> Array##_class( T...p ) : Array##_Base_Class( p... ){} \
		struct Register : public Array##_Base_Class :: Register \
		{ \
			struct SizeCaller \
			{ rsize_t operator()( OrgClass* pThis ) \
			{ return funSize( pThis ); } }; \
			struct GetCaller \
			{ _elem_type operator()( OrgClass* pThis, size_t nIndex ) \
			{ return funGetElem( pThis, nIndex ); } }; \
			struct SetCaller \
			{ void operator()( OrgClass* pThis, size_t nIndex, const _elem_value_type& v ) \
			{ return funSetElem( pThis, nIndex, v ); } }; \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					Array##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateClassFunWrap( funSize, \
					SizeCaller(), CPP_CONTAINER_SIZE, FieldTags );\
				::XReflect::CreateClassFunWrap( funGetElem, \
					GetCaller(), ARRAY_ELEM_GET, FieldTags );\
				::XReflect::CreateClassFunWrap( funSetElem, \
					SetCaller(), ARRAY_ELEM_SET, FieldTags );\
			} \
		}; \
	};  \
	typedef Array##_class

#define REGIST_ITERATOR_IMPLEMENT( IterType, Iterator, ... ) \
	Iterator##_Base_Class; \
	typedef IterType Iterator; \
	struct Iterator##_class : public Iterator##_Base_Class \
	{ \
		static constexpr ::XReflect::STypeInfo ElemType = \
			::XReflect::GetTypeInfo<decltype(*Iterator())>();\
		template<typename ...T> Iterator##_class( T...p ) : Iterator##_Base_Class( p... ){} \
		struct Register : public Iterator##_Base_Class :: Register \
		{ \
			Register() \
			{ \
				struct RegisterIterator : public ::XReflect::IClassDefinition\
				{\
					const char* GetNameSpace() \
					{ \
						static auto strNameSpace = \
							std::string( GetOrgNameSpace() ) + "::" + GetOrgClassName(); \
						return strNameSpace.c_str(); \
					}\
					const char* GetTemplateName() { return nullptr; } \
					uint32 GetTemplateTypeCount() { return 0; } \
					const char* GetTemplateParamter( uint32 n ){ return nullptr; } \
					::XReflect::STypeInfoArray GetTemplateParamters() { return ::XReflect::STypeInfoArray(); }\
					::XReflect::STypeInfo GetContainerElemType() { return ::XReflect::GetTypeInfo<void>(); } \
					const char* GetClassName() { return #Iterator; }\
					::XReflect::UPakDataType GetBinaryCompatibilityArrayElemType() \
						{ static ::XReflect::UPakDataType s_Type; return s_Type; } \
					bool IsMemoryCopyable() { return false; } \
					bool IsDefaultConstructible() { return true; } \
					bool IsCopyConstructible() { return true; } \
					bool IsMoveConstructible() { return true; } \
					uint32 GetCustomConstructorCount() { return 1; } \
					::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 ) { return ::XReflect::MakeFunArg<void>(); }\
					bool IsCopyAssignable() { return true; } \
					bool IsMoveAssignable() { return true; } \
					uint32 GetClassID() { return 0; }\
					uint32 GetClassSize() { return sizeof(Iterator); }\
					void Assign( void* pDest, void* pSrc ) { ::XReflect::TCopy<Iterator, true>( pDest, pSrc ); } \
					void Move( void* pDest, void* pSrc ) { ::XReflect::TMove<Iterator, true>( pDest, pSrc ); } \
					void Destruct( void* pObj ) { static_cast<Iterator*>( pObj )->~Iterator(); } \
					void Delete( void* pObj ) { delete (Iterator*)pObj; } \
					void Construct( int32 n, void* pObj, void** aryArg ) \
					{ \
						assert( n >= ::XReflect::eBCI_DefaultConstructor && n < 0 ); \
						if( n == ::XReflect::eBCI_DefaultConstructor ) \
							new ( pObj ) Iterator(); \
						else if( n == ::XReflect::eBCI_CopyConstructor ) \
							new ( pObj ) Iterator( *(Iterator*)aryArg[0] ); \
						else \
							new ( pObj ) Iterator( std::move( *(Iterator*)aryArg[0] ) ); \
					} \
					void* New( int32 n, void** aryArg ) \
					{ \
						assert( n >= ::XReflect::eBCI_DefaultConstructor && n < 0 ); \
						if( n == ::XReflect::eBCI_DefaultConstructor ) \
							return new Iterator(); \
						else if( n == ::XReflect::eBCI_CopyConstructor ) \
							return new Iterator( *(Iterator*)aryArg[0] ); \
						else \
							return new Iterator( std::move( *(Iterator*)aryArg[0] ) ); \
					} \
					RegisterIterator()\
					{\
						static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
						static const ::XReflect::CFieldTags FieldTags( \
							Iterator##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
						CreateIteratorWrap<Iterator>( this, FieldTags ); \
					}\
				};\
				static RegisterIterator s_Register; \
			} \
		}; \
	};  \
	typedef Iterator##_class


#define REGIST_AS_STRONG_POINTER_IMPLEMENT( ... ) \
	_SmartPtr##_Base_Class; \
	struct _SmartPtr##_class : public _SmartPtr##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _SmartPtr##_class( T...p ) : _SmartPtr##_Base_Class( p... ){} \
		struct Register : public _SmartPtr##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_SmartPtr##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateStrongPointerWrap<OrgClass>( FieldTags );\
			} \
		}; \
	};  \
	typedef _SmartPtr##_class 


#define REGIST_AS_WEAK_POINTER_IMPLEMENT( StrongPtrType, ... ) \
	_SmartPtr##_Base_Class; \
	struct _SmartPtr##_class : public _SmartPtr##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _SmartPtr##_class( T...p ) : _SmartPtr##_Base_Class( p... ){} \
		struct Register : public _SmartPtr##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_SmartPtr##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateWeakPointerWrap<OrgClass, StrongPtrType>( FieldTags );\
			} \
		}; \
	};  \
	typedef _SmartPtr##_class 


#define REGIST_BASE_CLASS_IMPLEMENT( ... )\
	_register_base_class_Base_Class; \
	struct _register_base_class : _register_base_class_Base_Class \
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _register_base_class( T...p ) : _register_base_class_Base_Class( p... ){} \
		struct Register : public _register_base_class_Base_Class::Register \
		{ \
			Register()\
			{ \
				::XReflect::TRegisterTypes<void, ##__VA_ARGS__>(); \
				using InheritInfo = ::XReflect::TInheritInfo<OrgClass, ##__VA_ARGS__>; \
				::XReflect::CReflection::GetInstance().RegisterClassBase( \
					::XReflect::TypeName<OrgClass>(), InheritInfo::size, \
					InheritInfo::BaseTypes().data(), InheritInfo::Offsets().data() );\
			} \
		};\
	};\
	typedef _register_base_class


#define REGIST_CLASSID_IMPLEMENT( nClassID )\
	_register_class_id_Base_Class; \
	struct _register_class_id : public _register_class_id_Base_Class \
	{\
		static constexpr uint32 ClassID = nClassID; \
	};\
	typedef _register_class_id


#define REGIST_CONSTRUCTOR_BEGIN_IMPLEMENT()\
	_abandon_Base_Class; \
	static_assert(_abandon_Base_Class::AllowRegisterConstruct, \
		"Must add REGIST_CUSTOM_CONSTRUCTOR after DEFINE_CLASS_BEGIN"); \
	struct _begin_constructable_class : public ::XReflect::TConstructWrap<OrgClass, true> \
	{\
		using BaseWrapClass = ::XReflect::TConstructWrap<OrgClass, true>; \
		static constexpr ::XReflect::STagArray GroupTag = _abandon_Base_Class::GroupTag; \
		static constexpr uint32 ClassID = _abandon_Base_Class::ClassID; \
		static constexpr ::XReflect::STypeInfo ElemType = _abandon_Base_Class::ElemType; \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ConstructTypes> \
		_begin_constructable_class( ConstructTypes Types, \
			VTable*& pVTable, bool bNeedVTable, void** aryArg )\
			: BaseWrapClass( Types, bNeedVTable ? &pVTable : nullptr, aryArg ){} \
		template<int32 Count> struct TConstructor \
		{ \
			static constexpr int32 nIndex = -1; \
			static ::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 n ) \
			{ \
				return ::XReflect::MakeFunArg<void>(); \
			}\
			static void Register() {} \
			template<typename EndClass> \
			static void Construct( int32 n, EndClass* pObj, void** aryArg ) \
			{ \
				assert( n >= ::XReflect::eBCI_DefaultConstructor && n < 0 ); \
				static constexpr bool bNewable = std::is_default_constructible_v<EndClass>; \
				static constexpr bool bCopyable = std::is_copy_constructible_v<EndClass>; \
				static constexpr bool bMoveable = std::is_move_constructible_v<EndClass>; \
				::XReflect::SFunctionTable* pTable = nullptr; \
				if( n == ::XReflect::eBCI_DefaultConstructor ) \
					::XReflect::TNew<EndClass, bNewable>::New( pObj ); \
				else if( n == ::XReflect::eBCI_CopyConstructor ) \
					::XReflect::TCopyNew<EndClass, bCopyable>::New( pObj, aryArg[0] ); \
				else \
					::XReflect::TMoveNew<EndClass, bMoveable>::New( pObj, aryArg[0] ); \
				if( !pTable ) return; \
				( (::XReflect::SVirtualClassInstance*)pObj )->pVirtualTable = pTable; \
			} \
			template<typename EndClass> \
			static void New( int32 n, EndClass*& pObj, void** aryArg ) \
			{ \
				assert( n >= ::XReflect::eBCI_DefaultConstructor && n < 0 ); \
				static constexpr bool bNewable = std::is_default_constructible_v<EndClass>; \
				static constexpr bool bCopyable = std::is_copy_constructible_v<EndClass>; \
				static constexpr bool bMoveable = std::is_move_constructible_v<EndClass>; \
				::XReflect::SFunctionTable* pTable = nullptr; \
				if( n == ::XReflect::eBCI_DefaultConstructor ) \
					pObj = ::XReflect::TNew<EndClass, bNewable>::New(); \
				else if( n == ::XReflect::eBCI_CopyConstructor ) \
					pObj = ::XReflect::TCopyNew<EndClass, bCopyable>::New( aryArg[0] ); \
				else \
					pObj = ::XReflect::TMoveNew<EndClass, bMoveable>::New( aryArg[0] ); \
				if( !pTable ) return; \
				( (::XReflect::SVirtualClassInstance*)pObj )->pVirtualTable = pTable; \
			} \
		}; \
		typedef TConstructor<-1> 


#define REGIST_CONSTRUCTOR_IMPLEMENT( ... )\
		DEFINE_UNIQUE_IDENTIFY( constructor_, __LINE__, _base ); \
		_begin_constructable_class( ::XReflect::TConstructionTypes<__VA_ARGS__> Types, \
			VTable*& pVTable, bool bNeedVTable, void** aryArg )\
			: BaseWrapClass( Types, bNeedVTable ? &pVTable : nullptr, aryArg ){} \
		template<> struct TConstructor<DEFINE_UNIQUE_IDENTIFY( constructor_, __LINE__, _base )::nIndex + 1> \
		{ \
			using PreConstructor = DEFINE_UNIQUE_IDENTIFY( constructor_, __LINE__, _base ); \
			using ParamTypesClass = ::XReflect::TConstructionTypes<__VA_ARGS__>; \
			static constexpr uint32 nIndex = PreConstructor::nIndex + 1; \
			static ::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 n ) \
			{ \
				if( n != nIndex ) return PreConstructor::GetCustomConstructorParam( n ); \
				return ::XReflect::MakeFunArg<void,##__VA_ARGS__>(); \
			}\
			static void Register() \
			{ \
				PreConstructor::Register(); \
				::XReflect::TRegisterTypes<void,##__VA_ARGS__>(); \
			} \
			template<typename EndClass> \
			static void Construct( int32 n, EndClass* pObj, void** aryArg ) \
			{ \
				if( n != nIndex ) return PreConstructor::Construct( n, pObj, aryArg ); \
				::XReflect::SFunctionTable* pTable = nullptr; \
				new( pObj ) EndClass( ParamTypesClass(), pTable, false, aryArg ); \
				if( !pTable ) return; \
				( (::XReflect::SVirtualClassInstance*)pObj )->pVirtualTable = pTable; \
			} \
			template<typename EndClass> \
			static void New( int32 n, EndClass*& pObj, void** aryArg ) \
			{ \
				if( n != nIndex ) return PreConstructor::New( n, pObj, aryArg ); \
				::XReflect::SFunctionTable* pTable = nullptr; \
				pObj = new EndClass( ParamTypesClass(), pTable, false, aryArg ); \
				if( !pTable ) return; \
				( (::XReflect::SVirtualClassInstance*)pObj )->pVirtualTable = pTable; \
			} \
		}; \
		typedef TConstructor<DEFINE_UNIQUE_IDENTIFY( constructor_, __LINE__, _base )::nIndex + 1> 


#define REGIST_CONSTRUCTOR_END_IMPLEMENT()\
		__end_constructor_class; \
		static ::XReflect::STypeInfoArray GetCustomConstructorParam( uint32 n ) \
		{ return __end_constructor_class::GetCustomConstructorParam( n ); }\
		static uint32 GetCustomConstructorCount() { return __end_constructor_class::nIndex + 1; } \
		template<typename EndClass> static void Construct( int32 n, EndClass* pObj, void** aryArg ) \
		{ return __end_constructor_class::Construct( n, pObj, aryArg ); } \
		template<typename EndClass> static void New( int32 n, EndClass*& pObj, void** aryArg ) \
		{ return __end_constructor_class::New( n, pObj, aryArg ); } \
		struct Register { Register() { __end_constructor_class::Register(); } }; \
	};\
	typedef _begin_constructable_class


#define REGIST_DESTRUCTOR_IMPLEMENT() \
	destructor_Base_Class; \
	struct destructor_class : public destructor_Base_Class \
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> destructor_class( T...p ) : destructor_Base_Class( p... ){} \
		struct Register : public destructor_Base_Class :: Register \
		{ \
			Register() { \
			static const ::XReflect::CFieldTags Tag( \
				destructor_Base_Class::GroupTag, {} ); \
			::XReflect::TDestructorWrap<OrgClass>::Bind( Tag ); } \
		}; \
	};  \
	typedef destructor_class


#define REGIST_CLASSMETHOD_IMPLEMENT( _function_type, _function, _function_name, ... ) \
	_function_name##_Base_Class; \
	struct _function_name##_class : public _function_name##_Base_Class\
	{ \
		typedef typename ::XReflect::ToMemberFuntion<OrgClass, _function_type>::T OrgFunctionType;\
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _function_name##_class( T...p ) : _function_name##_Base_Class( p... ){} \
		template<typename _RetType, typename... _Param> \
		struct TFunctionRegister : public _function_name##_Base_Class::Register \
		{ \
			typedef decltype ( (OrgFunctionType)nullptr ) _fun_type;\
			static _RetType Call( OrgClass* pThis, _Param ... p ) \
			{ \
				_fun_type funMember = (_fun_type)&OrgClass::_function; \
				return (pThis->*funMember)(p...); \
			}\
			template<typename RetType, typename ... Param> struct FunCaller \
			{ RetType operator()( OrgClass* pThis, Param ... p ) \
			{ return TFunctionRegister::Call( pThis, p... ); } }; \
			template< typename RetType, typename... Param > \
			static auto GetCaller( RetType(*)( OrgClass*, Param... ) ) \
			{ return FunCaller<RetType, Param...>(); } \
			TFunctionRegister()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_function_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateClassFunWrap( &TFunctionRegister::Call, \
					GetCaller( &TFunctionRegister::Call ), #_function_name, FieldTags );\
			} \
		};  \
		\
		template<typename ClassType, typename RetType, typename... Param> \
		static TFunctionRegister<RetType, Param...> Decl( RetType ( ClassType::*pFun )( Param... ) );\
		template<typename ClassType, typename RetType, typename... Param> \
		static TFunctionRegister<RetType, Param...> Decl( RetType ( ClassType::*pFun )( Param... ) const );\
		typedef decltype( Decl( (OrgFunctionType)nullptr ) ) Register;\
	};\
	typedef _function_name##_class


#define REGIST_CLASSMETHOD_WITH_LAMBDA_IMPLEMENT( _function_type, _lambda, _function_name, ... ) \
	_function_name##_Base_Class; \
	struct _function_name##_class : public _function_name##_Base_Class\
	{ \
		typedef typename ::XReflect::ToMemberFuntion<OrgClass, _function_type>::T OrgFunctionType;\
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _function_name##_class( T...p ) : _function_name##_Base_Class( p... ){} \
		template<typename _RetType, typename... _Param> \
		struct TOrgClass : public OrgClass \
		{ \
			_RetType CallImp( _Param ... p ) \
			{ \
				auto funImpl = _lambda; \
				return funImpl(p...); \
			}\
		};  \
		template<typename _RetType, typename... _Param> \
		struct TFunctionRegister : public _function_name##_Base_Class::Register \
		{ \
			typedef decltype ( (OrgFunctionType)nullptr ) _fun_type;\
			static _RetType Call( OrgClass* pThis, _Param ... p ) \
			{ \
				return ((TOrgClass<_RetType, _Param...>*)pThis)->CallImp(p...); \
			}\
			template<typename RetType, typename ... Param> struct FunCaller \
			{ RetType operator()( OrgClass* pThis, Param ... p ) \
			{ return TFunctionRegister::Call( pThis, p... ); } }; \
			template< typename RetType, typename... Param > \
			static auto GetCaller( RetType(*)( OrgClass*, Param... ) ) \
			{ return FunCaller<RetType, Param...>(); } \
			TFunctionRegister()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_function_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateClassFunWrap( &TFunctionRegister::Call, \
					GetCaller( &TFunctionRegister::Call ), #_function_name, FieldTags );\
			} \
		};  \
		\
		template<typename ClassType, typename RetType, typename... Param> \
		static TFunctionRegister<RetType, Param...> Decl( RetType ( ClassType::*pFun )( Param... ) );\
		typedef decltype( Decl( (OrgFunctionType)nullptr ) ) Register;\
	};\
	typedef _function_name##_class


#define REGIST_CLASSCALLBACK_IMPLEMENT( _function_type, _function, _function_name, ... ) \
	_function_name##_Base_Class; \
	struct _function_name##_scope \
	{ \
		typedef _function_name##_Base_Class BaseClass;\
		typedef ::XReflect::ToMemberFuntion<OrgClass, _function_type>::T OrgFunctionType;\
		template<typename FunType> struct TFunctionTypeFetch : public OrgClass \
		{ typedef decltype( (FunType)nullptr ) FunctionType; };\
		template<> struct TFunctionTypeFetch<void> : public OrgClass \
		{ typedef decltype( &TFunctionTypeFetch::_function ) FunctionType; };\
		typedef TFunctionTypeFetch<OrgFunctionType>::FunctionType FunctionType;\
		struct RegisterImpl : public OrgClass { \
		static FunctionType GetFun() { return (FunctionType)&RegisterImpl::_function; } };\
		\
		template<typename _RetType, typename... Param> \
		struct TFunctionOverride : public BaseClass \
		{ \
			static constexpr bool AllowRegisterConstruct = false; \
			typedef TFunctionOverride<_RetType, Param...> SelfType;\
			typedef ::XReflect::TCallBackBinder<SelfType> CallbackBinder;\
			template<typename ...T> TFunctionOverride( T...p ) : BaseClass( p... ) {}\
			_RetType _function( Param ... p ) { throw; } \
			_RetType _function( Param ... p ) const { throw; } \
			struct Register : public BaseClass::Register \
			{\
				Register()\
				{ \
					static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
					static const ::XReflect::CFieldTags FieldTags( \
						_function_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
					CallbackBinder::Bind( false, RegisterImpl::GetFun(), #_function_name, FieldTags );\
				} \
			};\
		}; \
		\
		template<typename ClassType, typename RetType, typename... Param> \
		static TFunctionOverride<RetType, Param...> Decl( RetType ( ClassType::* )( Param... ) );\
		template<typename ClassType, typename RetType, typename... Param> \
		static TFunctionOverride<RetType, Param...> Decl( RetType ( ClassType::* )( Param... ) const );\
		typedef decltype (Decl( (FunctionType)nullptr )) ImplementClass;\
	};\
	typedef _function_name##_scope::ImplementClass 


#define REGIST_CLASSMEMBER_IMPLEMENT( _qualifier, _member, _member_name, _member_type, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		struct TypeFetch : public OrgClass { friend struct _member_name##_class; };\
		static constexpr bool Addressable = []<typename T = TypeFetch>() \
		{ if constexpr (requires { &T::_member; }) return true; else return false; }(); \
		using MemberWrapType = std::remove_reference_t< \
			decltype( XReflect::TMemberTypeFetcher<Addressable>::GetMemberType( ( (TypeFetch*)0 )->_member ) )>; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		template<typename _ClassType, bool bDuplicatable, bool bAddressable> \
		struct MemberWrap : public TypeFetch \
		{	\
			static constexpr bool s_bAddressable = true; \
			static constexpr auto s_eQualifier = _qualifier; \
			typedef _ClassType ClassType;\
			typedef _member_type& MemberType;\
			static _member_type& Access( ClassType* pObject, _member_type* pSetData )\
			{\
                _member_type& MemberData = *(_member_type*)&((MemberWrap*)pObject)->_member; \
				if( pSetData ) MemberData = (_member_type&)*pSetData;\
				return MemberData;\
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, true, false> : public TypeFetch \
		{	\
			static constexpr bool s_bAddressable = false; \
			static constexpr auto s_eQualifier = _qualifier; \
			typedef _ClassType ClassType;\
			typedef _member_type MemberType;\
			static MemberType Access( ClassType* pObject, MemberType* pSetData )\
			{\
				if( pSetData ) ((MemberWrap*)pObject)->_member = (MemberType&)*pSetData;\
				return (MemberType)((MemberWrap*)pObject)->_member;\
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, false, true> : public TypeFetch \
		{ \
			static constexpr bool s_bAddressable = true; \
			static constexpr auto s_eQualifier = _qualifier|::XReflect::eMBQ_DisableWrite; \
			typedef _ClassType ClassType;\
			typedef _member_type& MemberType;\
			static _member_type& Access( ClassType* pObject, _member_type* pSetData )\
			{\
				return *(_member_type*)&((MemberWrap*)pObject)->_member; \
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, false, false> : public TypeFetch \
		{ \
			static constexpr bool s_bAddressable = false; \
			static constexpr auto s_eQualifier = _qualifier|::XReflect::eMBQ_DisableWrite; \
			typedef _ClassType ClassType;\
			typedef _member_type MemberType;\
		};\
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				constexpr bool bAssignable = std::is_copy_assignable<MemberWrapType>::value;\
				constexpr bool bSameType = std::is_same<MemberWrapType, _member_type>::value;\
				constexpr bool bAddressable = bSameType && Addressable;\
				typedef MemberWrap<OrgClass, bAssignable, bAddressable> MemberWrapClass; \
				::XReflect::CreateMemberWrap<MemberWrapClass>( #_member_name, FieldTags );\
			} \
		}; \
	};  \
	typedef _member_name##_class 


#define REGIST_CLASSPROPERTY_IMPLEMENT( _qualifier, _member_name, _get_fun, _set_fun, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		struct PropertyWrap : public OrgClass\
		{ \
			typedef decltype ((_get_fun)((OrgClass*)nullptr)) OrgReturnType;\
			typedef std::remove_reference<OrgReturnType>::type PropertyType;\
			static inline auto get_fun = _get_fun; \
			static inline auto set_fun = _set_fun; \
			typedef OrgClass ClassType;\
			typedef typename std::remove_reference<OrgReturnType>::type MemberType;\
			static constexpr bool s_bAddressable = std::is_reference<OrgReturnType>::value; \
			static constexpr auto s_eQualifier = _qualifier; \
			static OrgReturnType Access( OrgClass* pObject, MemberType* pSetData )\
			{\
				if( pSetData )\
					set_fun( (PropertyWrap*)pObject, *pSetData );\
				return get_fun( (PropertyWrap*)pObject );\
			}\
		}; \
		\
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateMemberWrap<PropertyWrap>( #_member_name, FieldTags );\
			} \
		}; \
	};  \
	typedef _member_name##_class 


#define REGIST_READONLY_CLASSPROPERTY_IMPLEMENT( _member_name, _get_fun, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		struct PropertyWrap\
		{ \
			static inline auto get_fun = _get_fun; \
			typedef decltype (get_fun((OrgClass*)nullptr)) OrgReturnType;\
			typedef std::remove_reference<OrgReturnType>::type PropertyType;\
			typedef OrgClass ClassType;\
			typedef typename std::remove_reference<OrgReturnType>::type MemberType;\
			static constexpr bool s_bAddressable = std::is_reference<OrgReturnType>::value; \
			static constexpr auto s_eQualifier = ::XReflect::eMBQ_DisableWrite; \
			static OrgReturnType Access( OrgClass* pObject, MemberType* pSetData )\
			{\
				return get_fun( pObject );\
			}\
		}; \
		\
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateMemberWrap<PropertyWrap>( #_member_name, FieldTags );\
			} \
		}; \
	};  \
	typedef _member_name##_class


#define REGIST_STATICMEMBER_IMPLEMENT( _qualifier, _member, _member_name, _member_type, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		struct TypeFetch : public OrgClass { friend struct _member_name##_class; };\
		using MemberWrapType = std::remove_reference_t< \
			decltype( XReflect::TMemberTypeFetcher<true>::GetMemberType( TypeFetch::_member ) )>; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		template<typename _ClassType, bool bDuplicatable, bool bAddressable> \
		struct MemberWrap \
		{ \
			static constexpr bool s_bAddressable = true; \
			static constexpr auto s_eQualifier = _qualifier; \
			typedef _ClassType ClassType;\
			typedef _member_type& MemberType;\
			static _member_type& Access()\
			{\
				return OrgClass::_member;\
			}\
			static _member_type& Access( _member_type* pSetData )\
			{\
				if( pSetData ) OrgClass::_member = (_member_type&)*pSetData;\
				return OrgClass::_member;\
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, true, false> : public TypeFetch \
		{ \
			static constexpr bool s_bAddressable = false; \
			static constexpr auto s_eQualifier = _qualifier; \
			typedef _ClassType ClassType;\
			typedef _member_type MemberType;\
			static MemberType Access()\
			{\
				return OrgClass::_member;\
			}\
			static MemberType Access( MemberType* pSetData )\
			{\
				if( pSetData ) OrgClass::_member = (MemberType&)*pSetData;\
				return OrgClass::_member;\
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, false, true> : public TypeFetch \
		{ \
			static constexpr bool s_bAddressable = true; \
			static constexpr auto s_eQualifier = _qualifier|::XReflect::eMBQ_DisableWrite; \
			typedef _ClassType ClassType;\
			typedef _member_type& MemberType;\
			static _member_type& Access()\
			{\
				return OrgClass::_member;\
			}\
			static _member_type& Access( _member_type* )\
			{\
				return OrgClass::_member;\
			}\
		};\
		template<typename _ClassType> \
		struct MemberWrap<_ClassType, false, false> : public TypeFetch \
		{ \
			static constexpr bool s_bAddressable = false; \
			static constexpr auto s_eQualifier = _qualifier|::XReflect::eMBQ_DisableWrite; \
			typedef _ClassType ClassType;\
			typedef _member_type MemberType;\
			static MemberType Access()\
			{\
				return OrgClass::_member;\
			}\
		};\
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				constexpr bool bAssignable = std::is_copy_assignable<MemberWrapType>::value && !std::is_const<MemberWrapType>::value;\
				constexpr bool bSameType = std::is_same<MemberWrapType, _member_type>::value;\
				constexpr bool bAddressable = bSameType; \
				typedef MemberWrap<OrgClass, bAssignable, bAddressable> MemberWrapClass; \
				::XReflect::CreateStaticMemberWrap<MemberWrapClass>( #_member_name, FieldTags, ::XReflect::eFDT_StaticMember );\
			} \
		}; \
	};  \
	typedef _member_name##_class 


#define REGIST_STATICPROPERTY_IMPLEMENT( _qualifier, _member_name, _get_fun, _set_fun, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		struct PropertyWrap : public OrgClass\
		{ \
			typedef decltype ((_get_fun)()) OrgReturnType;\
			typedef std::remove_reference<OrgReturnType>::type PropertyType;\
			static constexpr auto get_fun = _get_fun; \
			static constexpr auto set_fun = _set_fun; \
			typedef OrgClass ClassType;\
			typedef typename std::remove_reference<OrgReturnType>::type MemberType;\
			static constexpr bool s_bAddressable = std::is_reference<OrgReturnType>::value; \
			static constexpr auto s_eQualifier = _qualifier; \
			static OrgReturnType Access( MemberType* pSetData )\
			{\
				if( pSetData )\
					set_fun( *pSetData );\
				return get_fun();\
			}\
		}; \
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateStaticMemberWrap<PropertyWrap>( #_member_name, FieldTags, ::XReflect::eFDT_StaticMember );\
			} \
		}; \
	};  \
	typedef _member_name##_class 


#define REGIST_READONLY_STATICPROPERTY_IMPLEMENT( _member_name, _get_fun, ... ) \
	_member_name##_Base_Class; \
	struct _member_name##_class : public _member_name##_Base_Class\
	{ \
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _member_name##_class( T...p ) : _member_name##_Base_Class( p... ){} \
		struct PropertyWrap : public OrgClass\
		{ \
			static constexpr auto get_fun = _get_fun; \
			typedef decltype (get_fun()) OrgReturnType;\
			typedef std::remove_reference<OrgReturnType>::type PropertyType;\
			typedef OrgClass ClassType;\
			typedef typename std::remove_reference<OrgReturnType>::type MemberType;\
			static constexpr bool s_bAddressable = std::is_reference<OrgReturnType>::value; \
			static constexpr auto s_eQualifier = ::XReflect::eMBQ_DisableWrite; \
			static OrgReturnType Access( MemberType* )\
			{\
				return get_fun();\
			}\
		}; \
		struct Register : public _member_name##_Base_Class::Register \
		{ \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_member_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				::XReflect::CreateStaticMemberWrap<PropertyWrap>( #_member_name, FieldTags, ::XReflect::eFDT_StaticMember );\
			} \
		}; \
	};  \
	typedef _member_name##_class 


#define REGIST_STATICMETHOD_IMPLEMENT( _function_type, _function, _function_name, ... ) \
	_function_name##_Base_Class; \
	struct _function_name##_class : public _function_name##_Base_Class \
	{ \
        struct SHackClass : public OrgClass { \
        using FunctionType = _function_type; \
        static FunctionType GetOrgFunction() { return (FunctionType)&OrgClass::_function; } \
        template<typename RetType, typename ... Param> struct FunCaller \
        { RetType operator()( Param ... p ){ return OrgClass::_function( p... ); } }; }; \
        using FunctionType = SHackClass::FunctionType;\
		static constexpr bool AllowRegisterConstruct = false; \
		template<typename ...T> _function_name##_class( T...p ) : _function_name##_Base_Class( p... ){} \
		struct Register : public _function_name##_Base_Class::Register \
		{ \
			template< typename RetType, typename... Param > \
			static auto GetCaller( RetType(*)( Param... ) ) \
			{ return typename SHackClass::template FunCaller<RetType, Param...>(); } \
			Register()\
			{ \
				static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
				static const ::XReflect::CFieldTags FieldTags( \
					_function_name##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
				typedef decltype ( (FunctionType)nullptr ) _fun_type;\
				::XReflect::CreateGlobalFunWrap( SHackClass::GetOrgFunction(), \
					GetCaller( SHackClass::GetOrgFunction() ), \
					"", ::XReflect::TypeName<OrgClass>(), #_function_name, FieldTags );\
			} \
		}; \
	};  \
	typedef _function_name##_class 


#define REGIST_GLOBALCMETHOD_IMPLEMENT( _namespace, _fun_type, _function, _fun_name_lua, ... ) \
    namespace _fun_name_lua##_scope { \
	static constexpr const char* aryGroupTags[] = { #_namespace }; \
	static constexpr ::XReflect::STagArray GroupTag = { aryGroupTags, std::size( aryGroupTags ) }; \
	static constexpr const char* aryFieldTags[] = { "" __VA_ARGS__ }; \
	static const ::XReflect::CFieldTags FieldTags = { GroupTag, { aryFieldTags, std::size( aryFieldTags ) } }; \
	template<typename RetType, typename ... Param> struct FunCaller \
	{ RetType operator()( Param ... p ){ return _function( p... ); } }; \
	template< typename RetType, typename... Param > \
	auto GetCaller( RetType(*)( Param... ) ) { return FunCaller<RetType, Param...>(); }\
    static bool _fun_name_lua##_register( ( ::XReflect::CreateGlobalFunWrap( \
		(_fun_type)(&_function), GetCaller((_fun_type)(&_function) ), \
		#_namespace, nullptr, #_fun_name_lua, FieldTags ), true ) ); \
	}


#define DEFINE_ENUM_BEGIN_IMPLEMENT( _namespace, _enum_type, _enum_name ) \
    namespace _enum_type##_scope { \
	typedef _namespace::_enum_type _internal_enumType;\
	struct _begin_class \
	{\
		static constexpr const char* Tags[] = { #_enum_name }; \
		static constexpr ::XReflect::STagArray GroupTag = { Tags, std::size( Tags ) }; \
		static constexpr uint32 ClassID = 0; \
		static constexpr uint32 nSize = (int32)sizeof( _internal_enumType ); \
		_begin_class( uint32 nClassID, bool bMask ){ \
			::XReflect::CReflection::GetInstance().RegisterEnumType( #_namespace, #_enum_name, \
			nClassID, ::XReflect::TypeName<_internal_enumType>(), nSize, bMask ); }\
	};\
	typedef _begin_class 


#define REGIST_MASK_ENUM() \
	_register_enum_mask_Base_Class; \
	struct _register_enum_mask : public _register_enum_mask_Base_Class \
	{\
		_register_enum_mask( uint32 nClassID, bool bMask )\
			: _register_enum_mask_Base_Class( nClassID, true ) {}\
	};\
	typedef _register_enum_mask 


#define REGIST_ENUMVALUE_IMPLEMENT( _enum_value, _enum_name, ... ) \
	_enum_value##_Base_Class; \
	struct _enum_value##_class : public _enum_value##_Base_Class\
	{ \
		_enum_value##_class( uint32 nClassID, bool bMask )\
			: _enum_value##_Base_Class( nClassID, bMask ) { \
			static constexpr const char* Tags[] = { "" __VA_ARGS__ }; \
			static const ::XReflect::CFieldTags FieldTags( \
				_enum_value##_Base_Class::GroupTag, { Tags, std::size( Tags ) } ); \
			::XReflect::CReflection::GetInstance().RegisterEnumValue( \
			FieldTags, ::XReflect::TypeName<_internal_enumType>(), \
			#_enum_name, (int32)( _internal_enumType::_enum_value ) ); \
		}\
	};\
	typedef _enum_value##_class 


#define DEFINE_ENUM_END_IMPLEMENT() _last;\
	struct _end_enum : public _last \
	{\
		_end_enum() : _last( _last::ClassID, false ) {}\
	};\
	static _end_enum Register; }


#define REGIST_GROUP_TAG_BEGIN_IMPLEMENT( GroupTagName, ... )\
	GroupTagName##_Base_Class; \
	struct GroupTagName##_class : public GroupTagName##_Base_Class \
	{\
		template<typename ...T> GroupTagName##_class( T...p ) : GroupTagName##_Base_Class( p... ){} \
		static constexpr const char* Tags[] = { #GroupTagName, __VA_ARGS__ }; \
		static constexpr ::XReflect::STagArray GroupTag = { Tags, std::size( Tags ) }; \
	};\
	typedef GroupTagName##_class


#define REGIST_GROUP_TAG_END_IMPLEMENT()\
	DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _Base_Class); \
	struct DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _class) \
	: public DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _Base_Class) \
	{\
		template<typename ...T> DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _class)( T...p ) \
			: DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _Base_Class)( p... ){} \
		static constexpr ::XReflect::STagArray GroupTag = _begin_class::GroupTag; \
	};\
	typedef DEFINE_UNIQUE_IDENTIFY(GroupTagEnd, __LINE__, _class)
