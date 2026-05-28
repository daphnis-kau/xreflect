#pragma once

#include <cassert>
#include "CReflection.h"

namespace XReflect
{	
	template<typename Class>
	class TClass
	{
		const CClass* m_pClass;
	public:
		TClass( const CClass* pClass )
			: m_pClass( nullptr )
		{
			auto pDefaultClass = TClass<Class>::GetClass();
			assert( pDefaultClass != nullptr );
			if( pClass == nullptr )
				pClass = pDefaultClass;
			if( !pClass->IsA( pDefaultClass ) )
				return;
			m_pClass = pClass;
		}

		TClass( uint32 nClassID = 0 ) : TClass( nClassID == 0 ? nullptr 
			: CReflection::GetInstance().FindClassByClassID( nClassID ) )
		{
		}

		operator const CClass*() const
		{
			return m_pClass;
		}

		const CClass* operator ->() const
		{
			const CClass* pClass = *this;
			assert( pClass );
			return pClass;
		}

		const CClass& operator *() const
		{
			const CClass* pClass = *this;
			assert( pClass );
			return *pClass;
		}

		bool operator< ( const TClass& rhs ) const
		{
			uint32 nClassID1 = m_pClass ? m_pClass->GetClassID() : 0;
			uint32 nClassID2 = *rhs ? rhs->GetClassID() : 0;
			return nClassID1 < nClassID2;
		}

		static const CClass* GetClass()
		{
			struct RegisterTypeHelper
			{
				const CClass* m_pClass;
				RegisterTypeHelper()
				{
					::XReflect::RegisterType<Class>();
					m_pClass = CReflection::GetInstance().
						FindClassByTypeID( XReflect::TypeName<Class>() );
				}
			};
			static RegisterTypeHelper s_RegisterTypeHelper;
			return s_RegisterTypeHelper.m_pClass;
		}

		static uint32 GetClassID()
		{
			auto pClass = GetClass();
			return pClass ? pClass->GetClassID() : 0;
		}

		static bool IsA( const CClass* pClass )
		{
			auto pDefaultClass = TClass<Class>::GetClass();
			return pDefaultClass && pClass && pDefaultClass->IsA( pClass );
		}

		static bool IsA( uint32 nClassID )
		{
			auto& Reflection = CReflection::GetInstance();
			return IsA( Reflection.FindClassByClassID( nClassID ) );
		}

		template<typename T>
		static bool IsA()
		{
			return IsA( TClass<T>::GetClass() );
		}

		static bool IsBaseof( const CClass* pClass )
		{
			auto pDefaultClass = TClass<Class>::GetClass();
			return pDefaultClass && pClass && pClass->IsA( pDefaultClass );
		}

		static bool IsBaseof( uint32 nClassID )
		{
			auto& Reflection = CReflection::GetInstance();
			return IsBaseof( Reflection.FindClassByClassID( nClassID ) );
		}

		template<typename T>
		static bool IsBaseof()
		{
			return IsBaseof( TClass<T>::GetClass() );
		}
	};
}

