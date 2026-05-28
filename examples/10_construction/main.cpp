/** @file 10_construction/main.cpp
 * @brief Object construction/destruction via reflection.
 */
#include "Reflection.h"
#include <cstdio>

class Player
{
public:
	std::string m_Name;
	int  m_HP;

	Player() : m_Name( "Unnamed" ), m_HP( 100 ) {}
	Player( const Player& o ) : m_Name( o.m_Name ), m_HP( o.m_HP ) {}
	virtual ~Player() {}
};

DEFINE_CLASS_BEGIN( Player )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_Name )
	REGIST_CLASSMEMBER( m_HP )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();
	const CClass* pCls = R.FindClassByTypeID( TypeName<Player>() );

	printf( "=== Construction / Destruction ===\n\n" );
	printf( "Class: %s (size=%u)\n", pCls->GetClassName().c_str(), pCls->GetClassSize() );

	// ── malloc + placement construct ──
	void* pObj = malloc( pCls->GetClassSize() );
	pCls->Construct( eBCI_DefaultConstructor, pObj, nullptr );

	std::string name = pCls->FindField( "m_Name" )->Get<Player, std::string>( (Player*)pObj );
	int hp = pCls->FindField( "m_HP" )->Get<Player, int>( (Player*)pObj );
	printf( "Default ctor: name=%s, HP=%d\n", name.c_str(), hp );

	// ── Modify and read back ──
	int newHP = 50;
	pCls->FindField( "m_HP" )->SetData( pObj, &newHP );
	hp = pCls->FindField( "m_HP" )->Get<Player, int>( (Player*)pObj );
	printf( "After Set HP=50: HP=%d\n", hp );

	// ── Destruct and free ──
	pCls->Destruct( pObj );
	free( pObj );

	// ── New/Delete (heap allocation via reflection) ──
	void* pObj2 = pCls->New( eBCI_DefaultConstructor, nullptr );
	hp = pCls->FindField( "m_HP" )->Get<Player, int>( (Player*)pObj2 );
	printf( "New/Delete: HP=%d\n", hp );
	pCls->Delete( pObj2 );

	// ── Assign / Move + copy/move traits ──
	printf( "\n--- Object Semantics ---\n" );
	printf( "IsCopyConstructible: %s\n", pCls->IsCopyConstructible() ? "yes" : "no" );
	printf( "IsMoveConstructible: %s\n", pCls->IsMoveConstructible() ? "yes" : "no" );
	printf( "IsCopyAssignable:    %s\n", pCls->IsCopyAssignable() ? "yes" : "no" );
	printf( "IsMoveAssignable:    %s\n", pCls->IsMoveAssignable() ? "yes" : "no" );

	Player* pA = (Player*)pCls->New( eBCI_DefaultConstructor, nullptr );
	Player* pB = (Player*)pCls->New( eBCI_DefaultConstructor, nullptr );
	pA->m_Name = "Alice"; pA->m_HP = 100;
	pB->m_Name = "Bob";   pB->m_HP = 200;
	pCls->Assign( pA, pB );
	printf( "After Assign(Bob->Alice): %s HP=%d\n", pA->m_Name.c_str(), pA->m_HP );
	pCls->Delete( pA );
	pCls->Delete( pB );

	return 0;
}
