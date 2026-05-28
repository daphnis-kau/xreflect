#include "CName.h"
#include "ReflectionHelp.h"
#include <cassert>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace XReflect
{
	#define NAME_BLOCK_COUNT     4096
	#define NAME_COUNT_PER_BLOCK 65536
	#define NAME_BUFFER_SIZE     ( XR_MAX_NAME_LENGTH * 8 )

	struct SName
	{
		uint32_t m_nLen;
		uint32_t m_nIndex;
		const char* GetString() const { return (const char*)( this + 1 ); }
	};

	struct CNameArray { SName* aryName[ NAME_COUNT_PER_BLOCK ]; };

	static std::mutex      s_NameLock;
	static uint32_t        s_nNameCount = 0;
	static CNameArray*     g_aryNames[ NAME_BLOCK_COUNT ] = {};

	struct SNameMap
	{
		// ── Hash table ──
		std::vector<uint64_t> m_vHashes;
		std::vector<uint32_t> m_vIndices;

		// ── Bump allocator ──
		struct SNameBuffer { SNameBuffer* pNextBuffer; };
		SNameBuffer* m_pCurNameBuffer = nullptr;
		char*        m_pNameBufCur    = (char*)nullptr + NAME_BUFFER_SIZE;

		~SNameMap()
		{
			for( uint32_t i = 0; i < s_nNameCount; i++ )
			{
				SName* pName = GetSNameByIndex( i );
				if( pName ) pName->~SName();
			}
			uint32_t nMaxBlock = ( s_nNameCount - 1 ) / NAME_COUNT_PER_BLOCK;
			for( uint32_t i = 0; i <= nMaxBlock; i++ )
				delete g_aryNames[ i ];

			while( m_pCurNameBuffer )
			{
				SNameBuffer* pBuf = m_pCurNameBuffer;
				m_pCurNameBuffer = m_pCurNameBuffer->pNextBuffer;
				FreeVirtualMemory( pBuf, NAME_BUFFER_SIZE );
			}
		}

		SNameMap()
		{
			m_vHashes.resize( 16, 0 );
			m_vIndices.resize( 16, UINT32_MAX );

			SName* pName = AllocName( 0 );
			pName->m_nLen = 0;
			pName->m_nIndex = s_nNameCount++;
			((char*)( pName + 1 ))[0] = 0;
			g_aryNames[0] = new CNameArray;
			g_aryNames[0]->aryName[0] = pName;
			Insert( pName, 0 );
		}

		static SName* GetSNameByIndex( uint32_t nIndex )
		{
			uint32_t nBlock = nIndex / NAME_COUNT_PER_BLOCK;
			if( nBlock >= NAME_BLOCK_COUNT || !g_aryNames[ nBlock ] )
				return nullptr;
			return g_aryNames[ nBlock ]->aryName[ nIndex % NAME_COUNT_PER_BLOCK ];
		}

		static uint64_t Hash( const char* p, uint32_t n )
		{
			uint64_t h = 14695981039346656037ULL;
			for( uint32_t i = 0; i < n; i++ )
			{
				h ^= (uint8_t)p[ i ];
				h *= 1099511628211ULL;
			}
			return h;
		}

		SName* Find( const char* szName, uint32_t nLen )
		{
			if( m_vHashes.empty() )
				return nullptr;
			uint64_t nHash = Hash( szName, nLen );
			size_t   nMask = m_vHashes.size() - 1;
			size_t   nIdx  = nHash & nMask;
			while( m_vIndices[ nIdx ] != UINT32_MAX )
			{
				if( m_vHashes[ nIdx ] == nHash )
				{
					SName* pName = GetSNameByIndex( m_vIndices[ nIdx ] );
					if( pName && nLen == ( pName->m_nLen & 0x7fffffff ) &&
						!memcmp( szName, pName->GetString(), nLen ) )
						return pName;
				}
				nIdx = ( nIdx + 1 ) & nMask;
			}
			return nullptr;
		}

		void Insert( SName* pName, uint32_t nStrLen )
		{
			uint64_t nHash = Hash( pName->GetString(), nStrLen );
			size_t   nMask = m_vHashes.size() - 1;
			size_t   nIdx  = nHash & nMask;
			while( m_vIndices[ nIdx ] != UINT32_MAX )
				nIdx = ( nIdx + 1 ) & nMask;
			m_vHashes[ nIdx ]  = nHash;
			m_vIndices[ nIdx ] = pName->m_nIndex;

			if( s_nNameCount * 2 >= m_vHashes.size() )
			{
				size_t nOld = m_vHashes.size();
				auto   vOldHashes  = std::move( m_vHashes );
				auto   vOldIndices = std::move( m_vIndices );
				size_t nNew = nOld * 2;
				m_vHashes.assign( nNew, 0 );
				m_vIndices.assign( nNew, UINT32_MAX );
				size_t nNewMask = nNew - 1;
				for( size_t i = 0; i < nOld; i++ )
				{
					if( vOldIndices[ i ] == UINT32_MAX )
						continue;
					size_t nReIdx = vOldHashes[ i ] & nNewMask;
					while( m_vIndices[ nReIdx ] != UINT32_MAX )
						nReIdx = ( nReIdx + 1 ) & nNewMask;
					m_vHashes[ nReIdx ]  = vOldHashes[ i ];
					m_vIndices[ nReIdx ] = vOldIndices[ i ];
				}
			}
		}

		SName* AllocName( uint32_t nStrLen )
		{
			uint32_t nTotalSize = (uint32_t)AlignUp(
				sizeof( SName ) + nStrLen + 1, (size_t)8 );

			if( m_pNameBufCur + nTotalSize > (char*)m_pCurNameBuffer + NAME_BUFFER_SIZE )
			{
				if( m_pCurNameBuffer )
					ProtectVirtualMemory( m_pCurNameBuffer, NAME_BUFFER_SIZE, true );
				void* pNew = ReserveVirtualMemory( NAME_BUFFER_SIZE );
				CommitVirtualMemory( pNew, NAME_BUFFER_SIZE );
				SNameBuffer* pBuf = (SNameBuffer*)pNew;
				pBuf->pNextBuffer = m_pCurNameBuffer;
				m_pCurNameBuffer = pBuf;
				m_pNameBufCur = (char*)pNew + sizeof( SNameBuffer );
			}

			void* pBuf = m_pNameBufCur;
			m_pNameBufCur += nTotalSize;
			return new( pBuf ) SName;
		}

		static SNameMap& GetNameMap()
		{
			static SNameMap s_mapName;
			return s_mapName;
		}
	};

	uint32_t CName::GetNameIndex( const char* szName, uint32_t nLen )
	{
		if( !szName || !szName[0] )
			return 0;

		uint32_t nValidLen = (uint32_t)strnlen( szName, nLen );
		if( nValidLen >= XR_MAX_NAME_LENGTH )
			throw std::runtime_error( "String too long for CName" );

		std::lock_guard<std::mutex> lock( s_NameLock );
		SNameMap& map = SNameMap::GetNameMap();
		SName* pName = map.Find( szName, nValidLen );
		if( pName )
			return pName->m_nIndex;

		assert( s_nNameCount < NAME_BLOCK_COUNT * NAME_COUNT_PER_BLOCK );

		pName = map.AllocName( nValidLen );
		pName->m_nLen = nValidLen;
		pName->m_nIndex = s_nNameCount++;
		char* szTarget = (char*)( pName + 1 );
		memcpy( szTarget, szName, nValidLen );
		szTarget[ nValidLen ] = 0;

		uint32_t nBlockIndex = pName->m_nIndex / NAME_COUNT_PER_BLOCK;
		uint32_t nIndexInBlock = pName->m_nIndex % NAME_COUNT_PER_BLOCK;
		if( g_aryNames[ nBlockIndex ] == nullptr )
			g_aryNames[ nBlockIndex ] = new CNameArray;
		g_aryNames[ nBlockIndex ]->aryName[ nIndexInBlock ] = pName;
		map.Insert( pName, nValidLen );
		return pName->m_nIndex;
	}

	const char* CName::GetNameCString( uint32_t nIndex )
	{
		SName* pName = SNameMap::GetSNameByIndex( nIndex );
		return pName ? (const char*)( pName + 1 ) : "";
	}

	CName CName::GetNameByIndex( uint32_t nIndex )
	{
		CName Result;
		if( nIndex >= s_nNameCount )
			return Result;
		Result.m_nIndex = nIndex;
		return Result;
	}
}
