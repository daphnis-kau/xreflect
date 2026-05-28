/**@file  		TemplateWrap.h
* @brief		Template reflection support
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once
#include "CName.h"
#include <cstdio>
#include "ReflectionBase.h"

namespace XReflect
{
	// TypeRegister
	template<typename T>	
	struct TTypeRegister { static void Register() {} };

	template<typename T>	
	void RegisterType() { TTypeRegister<T>::Register(); };

	template<char n>		void RegisterType() {};
	template<int8_t n>		void RegisterType() {};
	template<int16_t n>		void RegisterType() {};
	template<int32_t n>		void RegisterType() {};
	template<long n>		void RegisterType() {};
	template<ulong n>		void RegisterType() {};
	template<int64_t n>		void RegisterType() {};
	template<uint8_t n>		void RegisterType() {};
	template<uint16_t n>	void RegisterType() {};
	template<uint32_t n>	void RegisterType() {};
	template<uint64_t n>	void RegisterType() {};
	template<wchar_t n>		void RegisterType() {};
	template<char16_t n>	void RegisterType() {};
	template<char32_t n>	void RegisterType() {};
	template<bool n>		void RegisterType() {};
	template<float n>		void RegisterType() {};
	template<double n>		void RegisterType() {};

	template<typename ...T> struct TRegisterTypes;

	template<typename T>
	struct TRegisterTypes<T>
	{
		typedef typename TTypeInfo<
			typename std::remove_reference<
			typename std::remove_pointer<
			typename std::remove_const<T>
			::type>::type>::type>::Type Type;
		TRegisterTypes() { RegisterType<Type>(); };
	};

	template<typename FirstType, typename ...T>
	struct TRegisterTypes<FirstType, T...>
		: public TRegisterTypes<T...>
	{
		typedef typename TTypeInfo<
			typename std::remove_reference<
			typename std::remove_pointer<
			typename std::remove_const<typename TTypeInfo<FirstType>::Type>
			::type>::type>::type>::Type Type;
		TRegisterTypes() { RegisterType<Type>(); };
	};

	template<typename Type>
	const char* Value2String( Type n )
	{
		thread_local static char szBuffer[128];
		snprintf( szBuffer, sizeof(szBuffer), "%lld", (long long)n );
		return szBuffer;
	}

	template<typename T> 
	const char* MakeTypeName( bool bReadableName )
	{
		TRegisterTypes<T>();
		if( bReadableName )
			return GetTypeReadableName( GetTypeInfo<T>() );
		return TypeName<T>();
	};

#ifdef _MSC_VER
    template<size_t n>      const char* MakeTypeName(bool) { return Value2String( n ); };
#else
	template<char n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<int8_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<int16_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<int32_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<long n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<ulong n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<int64_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<uint8_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<uint16_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<uint32_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<uint64_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<size_t n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<wchar_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<char16_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<char32_t n>	const char* MakeTypeName(bool) { return Value2String( n ); };
	template<bool n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<float n>		const char* MakeTypeName(bool) { return Value2String( n ); };
	template<double n>	const char* MakeTypeName(bool) { return Value2String( n ); };
#endif
}
