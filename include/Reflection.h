#pragma once

#include "ReflectionMacro.h"

/**
* @brief  Register class
* @note	Sampler: \n
*	DEFINE_CLASS_BEGIN( _class )\n
*		.... \n
*	DEFINE_CLASS_END()\n
*   or
*	DEFINE_CLASS( _class )
*/
#define DEFINE_CLASS_BEGIN( _class ) \
	DEFINE_CLASS_BEGIN_IMPLEMENT( , _class )
#define DEFINE_NS_CLASS_BEGIN( _namespace, _class ) \
	DEFINE_CLASS_BEGIN_IMPLEMENT( _namespace, _class )
#define DEFINE_CLASS_END() \
	DEFINE_CLASS_END_IMPLEMENT()

#define DEFINE_CLASS( _class ) \
	DEFINE_CLASS_BEGIN( _class ) \
	DEFINE_CLASS_END()

#define DEFINE_NS_CLASS( _namespace, _class ) \
	DEFINE_NS_CLASS_BEGIN( _namespace, _class ) \
	DEFINE_CLASS_END()

/**
* @brief  Register normal template class with 1 template param
* @note	Sampler: \n
*	DEFINE_TEMPLATE_BEGIN_WITH_1_PARAMS( _class, typename )\n
*		.... \n
*	DEFINE_TEMPLATE_END()\n
*/
#define DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM( _class, T0 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM_IMPLEMENT( , _class, T0 )
#define DEFINE_NS_TEMPLATE_BEGIN_WITH_1_PARAM( _namespace, _class, T0 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM_IMPLEMENT( _namespace, _class, T0 )

/**
* @brief  Register normal template class with 2 template params
* @note	Sampler: \n
*	DEFINE_TEMPLATE_BEGIN_WITH_2_PARAMS( _class, typename, typename )\n
*		.... \n
*	DEFINE_TEMPLATE_END()\n
*/
#define DEFINE_TEMPLATE_BEGIN_WITH_2_PARAMS( _class, T0, T1 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_2_PARAMS_IMPLEMENT( , _class, T0, T1 )
#define DEFINE_NS_TEMPLATE_BEGIN_WITH_2_PARAMS( _namespace, _class, T0, T1 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_2_PARAMS_IMPLEMENT( _namespace, _class, T0, T1 )

/**
* @brief  Register normal template class with 3 template params
* @note	Sampler: \n
*	DEFINE_TEMPLATE_BEGIN_WITH_3_PARAMS( _class, typename, typename, typename )\n
*		.... \n
*	DEFINE_TEMPLATE_END()\n
*/
#define DEFINE_TEMPLATE_BEGIN_WITH_3_PARAMS( _class, T0, T1, T2 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_3_PARAMS_IMPLEMENT( , _class, T0, T1, T2 )
#define DEFINE_NS_TEMPLATE_BEGIN_WITH_3_PARAMS( _namespace, _class, T0, T1, T2 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_3_PARAMS_IMPLEMENT( _namespace, _class, T0, T1, T2 )

/**
* @brief  Register normal template class with 4 template params
* @note	Sampler: \n
*	DEFINE_TEMPLATE_BEGIN_WITH_4_PARAMS( _class, typename, typename, typename, typename )\n
*		.... \n
*	DEFINE_TEMPLATE_END()\n
*/
#define DEFINE_TEMPLATE_BEGIN_WITH_4_PARAMS( _class, T0, T1, T2, T3 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_4_PARAMS_IMPLEMENT( , _class, T0, T1, T2, T3 )
#define DEFINE_NS_TEMPLATE_BEGIN_WITH_4_PARAMS( _namespace, _class, T0, T1, T2, T3 ) \
	DEFINE_TEMPLATE_BEGIN_WITH_4_PARAMS_IMPLEMENT( _namespace, _class, T0, T1, T2, T3 )

/**
* @brief  Register the template class end
*/
#define DEFINE_TEMPLATE_END() \
	DEFINE_TEMPLATE_END_IMPLEMENT()

/**
* @brief  Register the class's id
*/
#define REGIST_CLASSID( nClassID ) \
	REGIST_CLASSID_IMPLEMENT( nClassID )

/**
* @brief  Register the class's base class
*/
#define REGIST_BASE_CLASS( ... ) \
	REGIST_BASE_CLASS_IMPLEMENT( __VA_ARGS__ )

/**
* @brief  Register the class's constructor
*/
#define REGIST_DEFAULT_CONSTRUCTOR() \
	REGIST_CONSTRUCTOR_BEGIN_IMPLEMENT() \
	REGIST_CONSTRUCTOR_END_IMPLEMENT()
#define REGIST_CONSTRUCTOR_BEGIN() REGIST_CONSTRUCTOR_BEGIN_IMPLEMENT()
#define REGIST_CONSTRUCTOR_END() REGIST_CONSTRUCTOR_END_IMPLEMENT()
#define REGIST_CUSTOM_CONSTRUCTOR( ... ) REGIST_CONSTRUCTOR_IMPLEMENT( __VA_ARGS__ )

/**
* @brief  Register the class's virtual destructor
*/
#define REGIST_DESTRUCTOR()	\
	REGIST_DESTRUCTOR_IMPLEMENT()

/**
* @brief  Register class as strong pointer
*/
#define REGIST_AS_STRONG_POINTER( ... )	\
	REGIST_AS_STRONG_POINTER_IMPLEMENT( __VA_ARGS__ )

/**
* @brief  Register class as weak pointer
*/
#define REGIST_AS_WEAK_POINTER( StrongPtrType, ... )	\
	REGIST_AS_WEAK_POINTER_IMPLEMENT( StrongPtrType, __VA_ARGS__ )

/**
* @brief  Register the class as Array
*/
#define REGIST_ARRAY( ElemType, _size_function, ... ) \
	REGIST_ARRAY_IMPLEMENT( ElemType, \
		[this]()->size_t { return this->_size_function(); }, \
		[this]( size_t n )->ElemType { return (*this)[n]; }, \
		[this]( size_t n, const ElemType& v )->void { (*this)[n] = v; }, \
		__VA_ARGS__ )
#define REGIST_ARRAY_LAMBDA( ElemType, _lambda_size, _lambda_get, _lambda_set, ... ) \
	REGIST_ARRAY_IMPLEMENT( ElemType, _lambda_size, _lambda_get, _lambda_set, __VA_ARGS__ )

/**
* @brief  Register the iterator of the class
*/
#define REGIST_ITERATOR( Iterator, ... ) \
	REGIST_ITERATOR_IMPLEMENT( typename OrgClass::Iterator, Iterator, __VA_ARGS__ )
#define REGIST_ITERATOR_WITH_NAME( IteratorType, IteratorName, ... ) \
	REGIST_ITERATOR_IMPLEMENT( IteratorType, IteratorName, __VA_ARGS__ )

/**
* @brief  Register normal function member
*/
#define REGIST_CLASSMETHOD( _function, ... ) \
	REGIST_CLASSMETHOD_IMPLEMENT( decltype( &OrgClass::_function ), _function, _function, __VA_ARGS__ )
#define REGIST_CLASSMETHOD_WITH_NAME( _function, _fun_name, ... ) \
	REGIST_CLASSMETHOD_IMPLEMENT( decltype( &OrgClass::_function ), _function, _fun_name, __VA_ARGS__ )
#define REGIST_CLASSMETHOD_OVERLOAD( _function_type, _function, _fun_name, ... ) \
	REGIST_CLASSMETHOD_IMPLEMENT( _function_type, _function, _fun_name, __VA_ARGS__ )
#define REGIST_CLASSMETHOD_WITH_LAMBDA( _function_type, _lambda, _fun_name, ... ) \
	REGIST_CLASSMETHOD_WITH_LAMBDA_IMPLEMENT( _function_type, _lambda, _fun_name, __VA_ARGS__ )

/**
* @brief  Register static function member
*/
#define REGIST_STATICMETHOD( _function, ... ) \
	REGIST_STATICMETHOD_IMPLEMENT( decltype( &OrgClass::_function ), _function, _function, __VA_ARGS__ )
#define REGIST_STATICMETHOD_WITH_NAME( _function, _fun_name, ... ) \
	REGIST_STATICMETHOD_IMPLEMENT( decltype( &OrgClass::_function ), _function, _fun_name, __VA_ARGS__ )
#define REGIST_STATICMETHOD_OVERLOAD( _function_type, _function, _fun_name, ... ) \
	REGIST_STATICMETHOD_IMPLEMENT( _function_type, _function, _fun_name, __VA_ARGS__ )

/**
* @brief  Register data member
*/
#define REGIST_CLASSMEMBER( _member, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _member, MemberWrapType, __VA_ARGS__ )
#define REGIST_CLASSMEMBER_WITH_NAME( _member, _new_name, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _new_name, MemberWrapType, __VA_ARGS__ )
#define REGIST_CLASSMEMBER_WITH_TYPE( _member, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _member, _type, __VA_ARGS__ )
#define REGIST_CLASSMEMBER_WITH_NAME_AND_TYPE( _member, _new_name, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _new_name, _type, __VA_ARGS__ )

/**
* @brief  Register data member
*/
#define REGIST_SERIALIZE_DISABLE_CLASSMEMBER( _member, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member, _member, MemberWrapType, __VA_ARGS__ )
#define REGIST_SERIALIZE_DISABLE_CLASSMEMBER_WITH_NAME( _member, _new_name, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member, _new_name, MemberWrapType, __VA_ARGS__ )
#define REGIST_SERIALIZE_DISABLE_CLASSMEMBER_WITH_TYPE( _member, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member, _member, _type, __VA_ARGS__ )
#define REGIST_SERIALIZE_DISABLE_CLASSMEMBER_WITH_NAME_AND_TYPE( _member, _new_name, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member, _new_name, _type, __VA_ARGS__ )

/**
* @brief  Register data member
*/
#define REGIST_EDIT_DISABLE_CLASSMEMBER( _member, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member, _member, MemberWrapType, __VA_ARGS__ )
#define REGIST_EDIT_DISABLE_CLASSMEMBER_WITH_NAME( _member, _new_name, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member, _new_name, MemberWrapType, __VA_ARGS__ )
#define REGIST_EDIT_DISABLE_CLASSMEMBER_WITH_TYPE( _member, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member, _member, _type, __VA_ARGS__ )
#define REGIST_EDIT_DISABLE_CLASSMEMBER_WITH_NAME_AND_TYPE( _member, _new_name, _type, ... ) \
	REGIST_CLASSMEMBER_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member, _new_name, _type, __VA_ARGS__ )

/**
* @brief  Register call get/set as data member
*/
#define REGIST_CLASSPROPERTY( _member_name, _class_get_fun, _class_set_fun, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_None, _member_name, \
	[]( OrgClass* pObject ) { return pObject->_class_get_fun(); }, \
	[]( OrgClass* pObject, const PropertyType& Data ) { pObject->_class_set_fun( Data ); }, __VA_ARGS__ ) 
#define REGIST_CLASSPROPERTY_LAMBDA( _member_name, _get_lambda, _set_lambda, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_None, _member_name, _get_lambda, _set_lambda, __VA_ARGS__ )

/**
* @brief  Register call get/set as data member
*/
#define REGIST_SERIALIZE_DISABLE_CLASSPROPERTY( _member_name, _class_get_fun, _class_set_fun, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member_name, \
	[]( OrgClass* pObject ) { return pObject->_class_get_fun(); }, \
	[]( OrgClass* pObject, const PropertyType& Data ) { pObject->_class_set_fun( Data ); }, __VA_ARGS__ ) 
#define REGIST_SERIALIZE_DISABLE_CLASSPROPERTY_LAMBDA( _member_name, _get_lambda, _set_lambda, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_DisableSerialize, _member_name, _get_lambda, _set_lambda, __VA_ARGS__ )

/**
* @brief  Register call get/set as data member
*/
#define REGIST_EDIT_DISABLE_CLASSPROPERTY( _member_name, _class_get_fun, _class_set_fun, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member_name, \
	[]( OrgClass* pObject ) { return pObject->_class_get_fun(); }, \
	[]( OrgClass* pObject, const PropertyType& Data ) { pObject->_class_set_fun( Data ); }, __VA_ARGS__ ) 
#define REGIST_EDIT_DISABLE_CLASSPROPERTY_LAMBDA( _member_name, _get_lambda, _set_lambda, ... ) \
	REGIST_CLASSPROPERTY_IMPLEMENT( ::XReflect::eMBQ_DisableEdit, _member_name, _get_lambda, _set_lambda, __VA_ARGS__ )

/**
* @brief  Register call get/set as data member
*/
#define REGIST_READONLY_CLASSPROPERTY( _member_name, _class_get_fun, ... ) \
	REGIST_READONLY_CLASSPROPERTY_IMPLEMENT( _member_name, \
	[]( OrgClass* pObject ) { return pObject->_class_get_fun(); }, __VA_ARGS__ ) 
#define REGIST_READONLY_CLASSPROPERTY_LAMBDA( _member_name, _get_lambda, ... ) \
	REGIST_READONLY_CLASSPROPERTY_IMPLEMENT( _member_name, _get_lambda, __VA_ARGS__ ) 

/**
* @brief  Register static data member
*/
#define REGIST_STATICMEMBER( _member, ... ) \
	REGIST_STATICMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _member, MemberWrapType, __VA_ARGS__ )
#define REGIST_STATICMEMBER_WITH_NAME( _member, _new_name, ... ) \
	REGIST_STATICMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _new_name, MemberWrapType, __VA_ARGS__ )
#define REGIST_STATICMEMBER_WITH_TYPE( _member, _type, ... ) \
	REGIST_STATICMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _member, _type, __VA_ARGS__ )
#define REGIST_STATICMEMBER_WITH_NAME_AND_TYPE( _member, _new_name, _type, ... ) \
	REGIST_STATICMEMBER_IMPLEMENT( ::XReflect::eMBQ_None, _member, _new_name, _type, __VA_ARGS__ )

/**
* @brief  Register static property
*/
#define REGIST_STATICPROPERTY( _member_name, _class_get_fun, _class_set_fun, ... ) \
	REGIST_STATICPROPERTY_IMPLEMENT( ::XReflect::eMBQ_None, _member_name, \
	[]() { return OrgClass::_class_get_fun(); }, \
	[]( const decltype( OrgClass::_class_get_fun() )& Data ) { OrgClass::_class_set_fun( Data ); }, __VA_ARGS__ )
#define REGIST_STATICPROPERTY_LAMBDA( _member_name, _get_lambda, _set_lambda, ... ) \
	REGIST_STATICPROPERTY_IMPLEMENT( ::XReflect::eMBQ_None, _member_name, _get_lambda, _set_lambda, __VA_ARGS__ )

/**
* @brief  Register readonly static property
*/
#define REGIST_READONLY_STATICPROPERTY( _member_name, _class_get_fun, ... ) \
	REGIST_READONLY_STATICPROPERTY_IMPLEMENT( _member_name, \
	[]() { return OrgClass::_class_get_fun(); }, __VA_ARGS__ ) 
#define REGIST_READONLY_STATICPROPERTY_LAMBDA( _member_name, _get_lambda, ... ) \
	REGIST_READONLY_STATICPROPERTY_IMPLEMENT( _member_name, _get_lambda, __VA_ARGS__ ) 

/**
* @brief  Register global function
*/
#define REGIST_GLOBALCMETHOD( _function, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( , decltype( &_function ), _function, _function, __VA_ARGS__ )
#define REGIST_GLOBALCMETHOD_WITH_NAME( _function, _fun_name, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( , decltype( &_function ), _function, _fun_name, __VA_ARGS__ )
#define REGIST_GLOBALCMETHOD_OVERLOAD( _function_type, _function, _fun_name, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( , _function_type, _function, _fun_name, __VA_ARGS__ )
#define REGIST_NS_GLOBALCMETHOD( _namespace, _function, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( _namespace, decltype( &_function ), _function, _function, __VA_ARGS__ )
#define REGIST_NS_GLOBALCMETHOD_WITH_NAME( _namespace, _function, _fun_name, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( _namespace, decltype( &_function ), _function, _fun_name, __VA_ARGS__ )
#define REGIST_NS_GLOBALCMETHOD_OVERLOAD( _namespace, _function_type, _function, _fun_name, ... ) \
	REGIST_GLOBALCMETHOD_IMPLEMENT( _namespace, _function_type, _function, _fun_name, __VA_ARGS__ )

/**
* @brief  Register normal callback function
* @note	Callback function mean can override by class defined in script
*/
#define REGIST_CLASSCALLBACK( _function, ... ) \
	REGIST_CLASSCALLBACK_IMPLEMENT( void, _function, _function, __VA_ARGS__ )
#define REGIST_CLASSCALLBACK_WITH_NAME( _function, _fun_name, ... ) \
	REGIST_CLASSCALLBACK_IMPLEMENT( void, _function, _fun_name, __VA_ARGS__ )
#define REGIST_CLASSCALLBACK_OVERLOAD( _fun_type, _function, _fun_name, ... ) \
	REGIST_CLASSCALLBACK_IMPLEMENT( _fun_type, _function, _fun_name, __VA_ARGS__ )

/**
* @brief  Register enum type
* @note	Enum type will be register automatically when \n
*	it appear in parameters of registered function
*/
#define DEFINE_ENUM_BEGIN( EnumType ) \
	DEFINE_ENUM_BEGIN_IMPLEMENT( , EnumType, EnumType )
#define DEFINE_ENUM_BEGIN_WITH_NAME( EnumType, EnumName ) \
	DEFINE_ENUM_BEGIN_IMPLEMENT( , EnumType, EnumName )
#define DEFINE_NS_ENUM_BEGIN( _namespace, EnumType ) \
	DEFINE_ENUM_BEGIN_IMPLEMENT( _namespace, EnumType, EnumType )
#define DEFINE_NS_ENUM_BEGIN_WITH_NAME( _namespace, EnumType, EnumName ) \
	DEFINE_ENUM_BEGIN_IMPLEMENT( _namespace, EnumType, EnumName )

/**
* @brief  Register enum type end
*/
#define DEFINE_ENUM_END DEFINE_ENUM_END_IMPLEMENT

/**
* @brief  Register enum value
*/
#define REGIST_ENUMVALUE( EnumValue, ... ) \
	REGIST_ENUMVALUE_IMPLEMENT( EnumValue, EnumValue, __VA_ARGS__ )
#define REGIST_ENUMVALUE_WITH_NAME( EnumValue, EnumName, ... ) \
	REGIST_ENUMVALUE_IMPLEMENT( EnumValue, EnumName, __VA_ARGS__ )

/**
* @brief  Register group tags
*/
#define REGIST_GROUP_TAG_BEGIN( GroupTagName, ... )\
	REGIST_GROUP_TAG_BEGIN_IMPLEMENT(GroupTagName, __VA_ARGS__ )
#define REGIST_GROUP_TAG_END() \
	REGIST_GROUP_TAG_END_IMPLEMENT()

#include "Template/std.h"
