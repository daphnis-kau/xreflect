/** @file 02_fields_and_methods/main.cpp */
#include "Reflection.h"
#include <cstdio>
#include <cmath>

struct Vec3 { float x, y, z; float Length() const { return sqrtf(x*x+y*y+z*z); } };

DEFINE_CLASS_BEGIN( Vec3 )
	REGIST_CLASSMEMBER( x )
	REGIST_CLASSMEMBER( y )
	REGIST_CLASSMEMBER( z )
	REGIST_CLASSMETHOD( Length )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	const CClass* pCls = CReflection::GetInstance().FindClassByTypeID( TypeName<Vec3>() );
	printf( "=== Fields & Methods ===\n" );
	Vec3 v = { 3, 4, 0 };

	const CField* pX = pCls->FindField( "x" );
	float fx = pX->Get<Vec3, float>( &v );
	printf( "x = %.0f\n", fx );

	// Method invocation via Call
	const CField* pLen = pCls->FindField( "Length" );
	float  len   = 0;
	void*  pThis = &v;
	void*  aLen[] = { &pThis };
	pLen->Call( &len, aLen );
	printf( "Length = %.0f\n\n", len );

	// Two field access styles:
	//
	// Get<T>() / Set<T>() -- type-safe (recommended).
	// Template deduces addressable vs value semantics at compile time.
	printf( "--- Access methods ---\n" );
	printf( "  Get<Vec3,float>(x) = %.0f\n",
		pCls->FindField( "x" )->Get<Vec3, float>( &v ) );
	pCls->FindField( "y" )->Set<Vec3, float>( &v, 99.0f );
	printf( "  After Set<Vec3,float>(y,99):  y = %.0f\n",
		pCls->FindField( "y" )->Get<Vec3, float>( &v ) );

	// GetData() / SetData() -- raw void* (low-level).
	// For addressable members (direct fields), pass type** (pointer-to-pointer).
	// For non-addressable members (properties, bitfields), pass type* buffer.
	// Prefer Get<T>() / Set<T>() unless you need the raw interface.
	float* py = nullptr;
	pCls->FindField( "y" )->GetData( &v, &py );
	printf( "  GetData(y) via float**: y = %.0f\n", *py );
	return 0;
}
