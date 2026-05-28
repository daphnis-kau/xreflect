/**@file  		ReflectionHelp.h
* @brief		Define virtual function structure and interface
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <functional>
#include "ReflectionBase.h"

namespace XReflect
{
#ifdef _WIN32
	inline void* ReserveVirtualMemory( size_t nSize )
	{
		return VirtualAlloc( nullptr, nSize, MEM_RESERVE, PAGE_NOACCESS );
	}

	inline void CommitVirtualMemory( void* pAddr, size_t nSize )
	{
		VirtualAlloc( pAddr, nSize, MEM_COMMIT, PAGE_READWRITE );
	}

	inline void ProtectVirtualMemory( void* pAddr, size_t nSize, bool bReadOnly )
	{
		DWORD flOld;
		VirtualProtect( pAddr, nSize, bReadOnly ? PAGE_READONLY : PAGE_READWRITE, &flOld );
	}

	inline void DecommitVirtualMemory( void* pAddr, size_t nSize )
	{
		VirtualFree( pAddr, nSize, MEM_DECOMMIT );
	}

	inline void FreeVirtualMemory( void* pAddr, size_t nSize )
	{
		VirtualFree( pAddr, 0, MEM_RELEASE );
	}

	inline uint32 GetVirtualMemoryPageSize()
	{
		SYSTEM_INFO si; GetSystemInfo( &si ); return (uint32_t)si.dwPageSize;
	}
#else
	inline void* ReserveVirtualMemory( size_t nSize )
	{
		void* p = mmap( nullptr, nSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
		return p == MAP_FAILED ? nullptr : p;
	}

	inline void CommitVirtualMemory( void* pAddr, size_t nSize )
	{
		mprotect( pAddr, nSize, PROT_READ | PROT_WRITE );
	}

	inline void ProtectVirtualMemory( void* pAddr, size_t nSize, bool bReadOnly )
	{
		mprotect( pAddr, nSize, bReadOnly ? PROT_READ : ( PROT_READ | PROT_WRITE ) );
	}

	inline void DecommitVirtualMemory( void* pAddr, size_t nSize )
	{
		mmap( pAddr, nSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0 );
	}

	inline void FreeVirtualMemory( void* pAddr, size_t nSize )
	{
		munmap( pAddr, nSize );
	}

	inline uint32 GetVirtualMemoryPageSize()
	{
		return (uint32_t)sysconf( _SC_PAGESIZE );
	}
#endif

	template<typename T, typename U>
	constexpr T AlignUp( T n, U align )
	{
		return (T)((n + (T)align - 1) / (T)align * (T)align);
	}

	template<typename ElemType, typename SizeType>
	inline void	AppendArray( ElemType*& aryElems,
		SizeType& nCount, const ElemType& Elem )
	{
		auto aryNew = new ElemType[nCount + 1];
		memcpy( aryNew, aryElems, sizeof(ElemType) * nCount );
		aryNew[nCount++] = Elem;
		delete[] aryElems;
		aryElems = aryNew;
	}

	inline uint32_t StringHash( const char* s )
	{
		uint32_t h = 2166136261u;
		while (*s) h = (h ^ (uint8_t)*s++) * 16777619u;
		return h;
	}

	void InitFunctionTable( void* pFun[], uint32 nCount );
	int32 CalculateFunctionCount( const SFunctionTable* pTable );

	class CScopeExit
	{
		std::function<void()> m_fun;
	public:
		template<typename FunType>
		CScopeExit( FunType fun ) : m_fun( fun ) {}
		~CScopeExit() { m_fun(); }
	};

	inline void* AlignedAlloc( size_t nSize, size_t nAlign = 16 )
	{
		return ::operator new( nSize, std::align_val_t( nAlign ) );
	}

	inline void AlignedFree( void* p, size_t nAlign = 16 )
	{
		::operator delete( p, std::align_val_t( nAlign ) );
	}
}
