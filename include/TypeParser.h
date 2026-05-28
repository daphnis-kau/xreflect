/**@file  		TypeParser.h
* @brief		Fetch type information of C++ base type
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#pragma once

#include <cstdint>
#include <cstring>

typedef int8_t    int8;
typedef int16_t   int16;
typedef int32_t   int32;
typedef int64_t   int64;
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef unsigned char  tbyte;
typedef unsigned long  ulong;
#include "StaticTypeName.h"
#include <array>

namespace XReflect
{
	///< type of c++
	enum EDataType
	{
		eDT_void			= 0,
		eDT_char			= 1,
		eDT_int8			= 2,
		eDT_int16			= 3,
		eDT_int32			= 4,
		eDT_int64			= 5,
		eDT_long			= 6,
		eDT_uint8			= 7,
		eDT_uint16			= 8,
		eDT_uint32			= 9,
		eDT_uint64			= 10,
		eDT_ulong			= 11,
		eDT_wchar			= 12,
		eDT_char16			= 13,
		eDT_char32			= 14,
		eDT_bool			= 15,
		eDT_float			= 16,
		eDT_double			= 17,
		eDT_const_char_str	= 18,
		eDT_enum			= 19,
		eDT_class			= 20,
		eDT_count			= 21,
	};

	template<typename T>
	struct TTypeID
	{
		enum
		{
			eTypeID = std::is_enum<T>::value ? eDT_enum : eDT_class,
			eSize = sizeof( T )
		};
	};

	///< Builtin type of c++
	template<> struct TTypeID<void>		{ enum{ eTypeID = eDT_void	, eSize = 0					}; };
	template<> struct TTypeID<char>		{ enum{ eTypeID = eDT_char	, eSize = sizeof(char)		}; };
	template<> struct TTypeID<int8>		{ enum{ eTypeID = eDT_int8	, eSize = sizeof(int8)		}; };
	template<> struct TTypeID<int16>	{ enum{ eTypeID = eDT_int16	, eSize = sizeof(int16)		}; };
	template<> struct TTypeID<int32>	{ enum{ eTypeID = eDT_int32	, eSize = sizeof(int32)		}; };
	template<> struct TTypeID<long>		{ enum{ eTypeID = eDT_long	, eSize = sizeof(long)		}; };
	template<> struct TTypeID<int64>	{ enum{ eTypeID = eDT_int64	, eSize = sizeof(int64)		}; };
	template<> struct TTypeID<uint8>	{ enum{ eTypeID = eDT_uint8	, eSize = sizeof(uint8)		}; };
	template<> struct TTypeID<uint16>	{ enum{ eTypeID = eDT_uint16, eSize = sizeof(uint16)	}; };
	template<> struct TTypeID<uint32>	{ enum{ eTypeID = eDT_uint32, eSize = sizeof(uint32)	}; };
	template<> struct TTypeID<uint64>	{ enum{ eTypeID = eDT_uint64, eSize = sizeof(uint64)	}; };
	template<> struct TTypeID<ulong>	{ enum{ eTypeID = eDT_ulong	, eSize = sizeof(ulong)		}; };
	template<> struct TTypeID<wchar_t>	{ enum{ eTypeID = eDT_wchar	, eSize = sizeof(wchar_t)	}; };
	template<> struct TTypeID<char16_t>	{ enum{ eTypeID = eDT_wchar	, eSize = sizeof(char16_t)	}; };
	template<> struct TTypeID<char32_t>	{ enum{ eTypeID = eDT_wchar	, eSize = sizeof(char32_t)	}; };
	template<> struct TTypeID<bool>		{ enum{ eTypeID = eDT_bool	, eSize = sizeof(bool)		}; };
	template<> struct TTypeID<float>	{ enum{ eTypeID = eDT_float	, eSize = sizeof(float)		}; };
	template<> struct TTypeID<double>	{ enum{ eTypeID = eDT_double, eSize = sizeof(double)	}; };

	// make constant flag mask
	template<uint32 n>
	constexpr std::array<bool, n + 1> AppendConstMask( std::array<bool, n> ary, bool bConst )
	{
		std::array<bool, n + 1> result;
		for( uint32 i = 0; i < n; i++ )
			result[i] = ary[i];
		result[n] = bConst;
		return result;
	}

	/**@struct template of type information
	* @brief Fetch typeinfo by template, default type is class, \n
	*		other types' information is fetched by template specialization
	*/
	template<typename T>
	struct TTypeInfo
	{
		using Type = T;
		constexpr static const uint32 nSize = TTypeID<Type>::eSize;
		constexpr static const EDataType eType = (EDataType)TTypeID<Type>::eTypeID;
		constexpr static const uint32 nConstMaskCount = 1;
		constexpr static const uint32 nConstMask = 0;
	};

	///< Constant type
	template<typename T>
	struct TTypeInfo<T const>
	{
		using Type = typename TTypeInfo<T>::Type;
		constexpr static const uint32 nSize = TTypeID<Type>::eSize;
		constexpr static const EDataType eType = (EDataType)TTypeID<Type>::eTypeID;
		constexpr static const uint32 nConstMaskCount = 1;
		constexpr static const uint32 nConstMask = 1;
	};

	///< Pointer type
	template<typename T>
	struct TTypeInfo<T*>
	{
		using Type = typename TTypeInfo<T>::Type;
		constexpr static const uint32 nSize = TTypeInfo<T>::nSize;
		constexpr static const EDataType eType = (EDataType)TTypeInfo<T>::eType;
		constexpr static const uint32 nConstMaskCount = TTypeInfo<T>::nConstMaskCount + 1;
		constexpr static const uint32 nConstMask = TTypeInfo<T>::nConstMask;
	};

	///< Constant pointer type
	template<typename T>
	struct TTypeInfo<T* const>
	{
		using Type = typename TTypeInfo<T>::Type;
		constexpr static const uint32 nSize = TTypeInfo<T>::nSize;
		constexpr static const EDataType eType = (EDataType)TTypeInfo<T>::eType;
		constexpr static const uint32 nConstMaskCount = TTypeInfo<T>::nConstMaskCount + 1;
		constexpr static const uint32 nConstMask = TTypeInfo<T>::nConstMask|( 1 << TTypeInfo<T>::nConstMaskCount);
	};

	///< C string type
	template<>
	struct TTypeInfo<const char*>
	{
		using Type = const char*;
		constexpr static const uint32 nSize = sizeof( const char* );
		constexpr static const EDataType eType = eDT_const_char_str;
		constexpr static const uint32 nConstMaskCount = 1;
		constexpr static const uint32 nConstMask = 0;
	};

	///< Void type
	template<>
	struct TTypeInfo<void>
	{
		using Type = void;
		constexpr static const uint32 nSize = 0;
		constexpr static const EDataType eType = eDT_void;
		constexpr static const uint32 nConstMaskCount = 1;
		constexpr static const uint32 nConstMask = 0;
	};

	///< nullptr type
	template<> struct TTypeInfo<decltype(nullptr)>
	{
		using Type = void;
		constexpr static const uint32 nSize = sizeof( decltype(nullptr) );
		constexpr static const EDataType eType = eDT_void;
		constexpr static const uint32 nConstMaskCount = 2;
		constexpr static const uint32 nConstMask = 0;
	};

	struct STypeInfo
	{
		EDataType	eType;
		uint32		nSize;
		uint32		nPointerConstMask;
		uint16		nMultiPointers;
		uint8		nReference;
		bool		bConstType;
		const char* szTypeName;
	};

	// compare two type information
	inline bool operator == ( const STypeInfo& lhs, const STypeInfo& rhs )
	{
		return lhs.eType == rhs.eType && lhs.nSize == rhs.nSize &&
			lhs.nPointerConstMask == rhs.nPointerConstMask &&
			lhs.nMultiPointers == rhs.nMultiPointers &&
			lhs.nReference == rhs.nReference && lhs.bConstType == rhs.bConstType &&
			( lhs.szTypeName == rhs.szTypeName || !strcmp( lhs.szTypeName, rhs.szTypeName ) );
	}

	///< compare two type information
	inline bool operator != ( const STypeInfo& lhs, const STypeInfo& rhs )
	{
		return !( lhs == rhs );
	}

	/**
	* @brief Get information of the giving type
	* @return information of the giving type
	*/
	template<typename T>
	constexpr STypeInfo GetTypeInfo()
	{
		using UnRefType = typename std::remove_reference<T>::type;
		STypeInfo Info;
		Info.eType = TTypeInfo<UnRefType>::eType;
		Info.nSize = TTypeInfo<UnRefType>::nSize;
		Info.nReference = std::is_rvalue_reference_v<T> ? 2 : ( std::is_reference_v<T> ? 1 : 0 );
		Info.szTypeName = TypeName<typename TTypeInfo<UnRefType>::Type>();
		Info.bConstType = TTypeInfo<UnRefType>::nConstMask&1;
		Info.nMultiPointers = TTypeInfo<UnRefType>::nConstMaskCount - 1;
		Info.nPointerConstMask = TTypeInfo<UnRefType>::nConstMask >> 1;
		return Info;
	}

	/**
	* @brief Get information of the giving type
	* @return information of the giving type
	*/
	template<size_t N>
	constexpr STypeInfo GetTypeInfo()
	{
		STypeInfo Info;
		Info.eType = eDT_void;
		Info.nSize = N;
		Info.nReference = 0;
		Info.szTypeName = TypeName<size_t>();
		Info.bConstType = true;
		Info.nMultiPointers = false;
		Info.nPointerConstMask = 0;
		return Info;
	}

	///< Convert function pointer to uintptr_t
	template<typename FunctionType>
	uintptr_t FunctionTypeToIntValue( FunctionType fun )
	{
		uintptr_t funValue = *(uintptr_t*)&fun;
		FunctionType result = nullptr;
		*(uintptr_t*)&result = funValue;
		if( result != fun )
			throw "the function can not covert to uintptr_t";
		return funValue;
	};
}
