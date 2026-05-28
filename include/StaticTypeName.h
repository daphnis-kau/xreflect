/**@file  		StaticTypeName.h
* @brief		Get static type name with rtti
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#pragma once

namespace XReflect
{
	#ifdef _MSC_VER
		template<typename T>
		constexpr const char* TypeName()
		{
			const char* sig = __FUNCSIG__;
			const char* p = sig;
			while( *p && *p != '<' ) ++p;
			return *p ? p : sig;
		}
	#elif defined( __GNUC__ )
		template<typename T>
		constexpr const char* TypeName()
		{
			return __PRETTY_FUNCTION__;
		}
	#else
		#error "Unsupported compiler"
	#endif
}
