#pragma once

#include <cstdint>
#include <cstring>
#include <ostream>

#define XR_MAX_NAME_LENGTH	8192

namespace XReflect
{
	class CName
	{
		uint32_t m_nIndex;

		static uint32_t GetNameIndex( const char* szName, uint32_t nLen );
		static const char* GetNameCString( uint32_t nIndex );

	public:
		CName() : m_nIndex( 0 ) {}
		CName( const char* szString, uint32_t nLen = UINT32_MAX )
			: m_nIndex( GetNameIndex( szString, nLen ) ) {}
		static CName GetNameByIndex( uint32_t nIndex );

		operator bool() const { return m_nIndex != 0; }
		uint32_t GetIndex() const { return m_nIndex; }
		const char* c_str() const { return GetNameCString( m_nIndex ); }

		bool operator==( CName Name ) const { return m_nIndex == Name.m_nIndex; }
		bool operator!=( CName Name ) const { return !( *this == Name ); }
		bool operator==( const char* szName ) const { return !strcmp( c_str(), szName ); }
		bool operator!=( const char* szName ) const { return !( *this == szName ); }
		bool operator<( CName Name ) const { return m_nIndex < Name.m_nIndex; }
	};
}

