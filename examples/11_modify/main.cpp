/** @file 11_modify/main.cpp
 * @brief Write path via field path (Modify).
 *
 * Demonstrates CClass::Modify as the complement to Fetch (08_fieldpath).
 * Together they form the reflection read/write pair used by serialization
 * and undo/redo systems.
 */
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

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();
	const CClass* pEnemy = R.FindClassByTypeID( TypeName<Enemy>() );
	const CClass* pPos   = R.FindClassByTypeID( TypeName<Pos>() );

	printf( "=== Modify (Write via Field Path) ===\n\n" );

	Enemy e;
	e.m_Pos.m_X = 1.0f;
	e.m_Pos.m_Y = 2.0f;
	e.m_HP = 100;

	printf( "Before: Pos=(%.0f,%.0f) HP=%d\n", e.m_Pos.m_X, e.m_Pos.m_Y, e.m_HP );

	// ── Modify a direct member: m_HP ──
	{
		const CField* pHP = pEnemy->FindField( "m_HP" );
		SFieldNode ary[] = { SFieldNode( pHP ) };
		SFieldPath path  = { ary, 1 };

		int newHP = 50;
		pEnemy->Modify( &e, &newHP, false, path, eFOT_Modify );
		printf( "After Modify(m_HP, 50): HP=%d\n", e.m_HP );
	}

	// ── Modify a nested member: m_Pos.m_X ──
	{
		const CField* pPP = pEnemy->FindField( "m_Pos" );
		const CField* pPX = pPos->FindField( "m_X" );
		SFieldNode ary[] = { SFieldNode( pPP ), SFieldNode( pPX ) };
		SFieldPath path  = { ary, 2 };

		float newX = 9.0f;
		pEnemy->Modify( &e, &newX, false, path, eFOT_Modify );
		printf( "After Modify(m_Pos.m_X, 9): Pos=(%.0f,%.0f)\n",
			e.m_Pos.m_X, e.m_Pos.m_Y );
	}

	// ── Verify with Fetch ──
	{
		const CField* pPP = pEnemy->FindField( "m_Pos" );
		const CField* pPY = pPos->FindField( "m_Y" );
		SFieldNode ary[] = { SFieldNode( pPP ), SFieldNode( pPY ) };
		SFieldPath path  = { ary, 2 };

		float val = 0;
		pEnemy->Fetch( &e, path, &val,
			[]( void* d, void* c ) { *(float*)c = *(float*)d; } );
		printf( "Fetch(m_Pos.m_Y) = %.0f (expected 2.0)\n", val );
	}

	// ── Offset checks ──
	printf( "\n--- Offset checks ---\n" );
	printf( "IsBaseOffset(0) for Enemy: %s\n",
		pEnemy->IsBaseOffset( 0 ) ? "yes" : "no" );

	return 0;
}
