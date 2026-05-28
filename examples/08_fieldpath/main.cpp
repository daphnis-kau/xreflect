/** @file 08_fieldpath/main.cpp */
#include "Reflection.h"
#include <cstdio>

struct Pos { float m_X, m_Y; };
DEFINE_CLASS_BEGIN( Pos )
	REGIST_CLASSMEMBER( m_X )
	REGIST_CLASSMEMBER( m_Y )
DEFINE_CLASS_END()

struct Enemy { Pos m_Pos; int m_HP; };
DEFINE_CLASS_BEGIN( Enemy )
	REGIST_CLASSMEMBER( m_Pos )
	REGIST_CLASSMEMBER( m_HP )
DEFINE_CLASS_END()

static void PrintPath( XReflect::SFieldPath Path, const char* szLabel )
{
	printf( "%s (%u nodes): [", szLabel, Path.nNodeCount );
	for( uint32 i = 0; i < Path.nNodeCount; i++ )
	{
		if( i ) printf( ", " );
		switch( Path.aryPath[i].GetType() )
		{
		case XReflect::eFNT_NormalField:
			printf( "field:%s", Path.aryPath[i].GetField()->GetName().c_str() ); break;
		case XReflect::eFNT_TypeCast:
			printf( "cast:%s", Path.aryPath[i].GetTargetClass()->GetClassName().c_str() ); break;
		case XReflect::eFNT_ElementIndex:
			printf( "idx:%llu", (unsigned long long)Path.aryPath[i].GetIndex() ); break;
		default: break;
		}
	}
	printf( "]\n" );
}

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();
	const CClass* pEnemy = R.FindClassByTypeID( TypeName<Enemy>() );
	const CClass* pPos   = R.FindClassByTypeID( TypeName<Pos>() );

	printf( "=== FieldPath ===\n" );

	// Single field: m_HP
	const CField* pHP = pEnemy->FindField( "m_HP" );
	SFieldNode ary1[] = { SFieldNode( pHP ) };
	SFieldPath p1 = { ary1, 1 };
	PrintPath( p1, "m_HP" );

	// Chained: m_Pos.m_X
	const CField* pPP = pEnemy->FindField( "m_Pos" );
	const CField* pPX = pPos->FindField( "m_X" );
	SFieldNode ary2[] = { SFieldNode( pPP ), SFieldNode( pPX ) };
	SFieldPath p2 = { ary2, 2 };
	PrintPath( p2, "m_Pos.m_X" );

	Enemy e;
	e.m_Pos.m_X = 7.5f;
	float x = 0;
	pEnemy->Fetch( &e, p2, &x, []( void* d, void* c ) { *(float*)c = *(float*)d; } );
	printf( "Fetch m_Pos.m_X = %.1f\n", x );

	// Fetch via std::function overload
	printf( "\n--- EnumFields + Fetch(function) ---\n" );
	pEnemy->EnumFields( (uint32)eFDT_Member, []( const CField* pF ) {
		printf( "  EnumFields: %s\n", pF->GetName().c_str() );
		return true;
	} );

	// Fetch via std::function
	Enemy e2; e2.m_Pos.m_Y = 99.0f;
	const CField* pPY = pPos->FindField( "m_Y" );
	SFieldNode ay3[] = { SFieldNode(pPP), SFieldNode(pPY) };
	SFieldPath p3 = { ay3, 2 };
	pEnemy->Fetch( &e2, p3, []( void* d ) {
		printf( "  Fetch(function) m_Pos.m_Y = %.1f\n", *(float*)d );
	} );

	return 0;
}
