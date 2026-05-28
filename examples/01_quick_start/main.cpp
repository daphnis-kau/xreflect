/** @file 01_quick_start/main.cpp
 * @brief Minimal example: register a struct and read its fields.
 *
 * Build:  cmake --build . --target 01_quick_start
 * Run:    ./01_quick_start
 */
#include "Reflection.h"
#include <cstdio>

struct Vec3 { float x, y, z; };

DEFINE_CLASS_BEGIN( Vec3 )
	REGIST_CLASSMEMBER( x )
	REGIST_CLASSMEMBER_WITH_NAME( x, pos_x )
	REGIST_CLASSMEMBER( y )
	REGIST_CLASSMEMBER( z )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();

	const CClass* pCls = R.FindClassByTypeID( TypeName<Vec3>() );
	printf( "Class: %s  (size: %u)\n", pCls->GetClassName().c_str(), pCls->GetClassSize() );

	Vec3 v = { 1.0f, 2.0f, 3.0f };
	for( auto& pair : pCls->GetRegistFields() )
	{
		auto f = pair.second;
		float val = f->Get<Vec3, float>( &v );
		printf( "  %s = %.1f\n", f->GetName().c_str(), val );
	}

	// x is also registered as "pos_x" via REGIST_CLASSMEMBER_WITH_NAME
	const CField* pX2 = pCls->FindField( "pos_x" );
	if( pX2 )
		printf( "\n  pos_x (renamed) = %.1f\n", pX2->Get<Vec3, float>( &v ) );
	return 0;
}
