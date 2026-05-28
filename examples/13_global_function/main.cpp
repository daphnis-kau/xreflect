/** @file 13_global_function/main.cpp
 * @brief Global function registration via REGIST_GLOBALCMETHOD.
 */
#include "Reflection.h"
#include <cstdio>

static float Global_Add( float a, float b )
{
	return a + b;
}

static const char* Global_Version()
{
	return "xreflect v1.0";
}

REGIST_GLOBALCMETHOD( Global_Add )
REGIST_GLOBALCMETHOD_WITH_NAME( Global_Add, Add )
REGIST_GLOBALCMETHOD( Global_Version )

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();

	printf( "=== Global Function Registration ===\n\n" );

	// REGIST_GLOBALCMETHOD registers into a global scope.
	// With empty namespace the scope type name is "".
	const CClass* pScope = R.FindClassByTypeID( CName( "" ) );

	if( pScope )
	{
		printf( "Global scope fields:\n" );
		for( auto& pair : pScope->GetRegistFields() )
		{
			const CField* pF = pair.second;
			printf( "  %s (type=%d)\n", pF->GetName().c_str(), pF->GetType() );
		}

		const CField* pAdd = pScope->FindField( "Global_Add" );
		if( pAdd )
		{
			float a = 3.0f, b = 4.0f, result = 0;
			void* args[] = { &a, &b };
			pAdd->Call( &result, args );
			printf( "\nGlobal_Add(3, 4) = %.0f\n", result );
		}

		const CField* pVer = pScope->FindField( "Global_Version" );
		if( pVer )
		{
			const char* szVer = nullptr;
			pVer->Call( &szVer, nullptr );
			printf( "Global_Version() = %s\n", szVer );
		}
	}
	else
	{
		printf( "Global scope not found.\n" );
	}

	return 0;
}
