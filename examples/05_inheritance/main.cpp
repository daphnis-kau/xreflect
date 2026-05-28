/** @file 05_inheritance/main.cpp */
#include "Reflection.h"
#include <cstdio>

struct Entity { int m_Id; Entity() : m_Id(0) {} };
DEFINE_CLASS_BEGIN( Entity )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_Id )
DEFINE_CLASS_END()

struct Enemy : Entity { int m_Damage; Enemy() : m_Damage(10) {} };
DEFINE_CLASS_BEGIN( Enemy )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_BASE_CLASS( Entity )
	REGIST_CLASSMEMBER( m_Damage )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	const CClass* pE = CReflection::GetInstance().FindClassByTypeID( TypeName<Enemy>() );
	printf( "=== Inheritance ===\n" );
	printf( "Enemy base count: %u\n", pE->GetBaseClassCount() );
	printf( "Enemy inherits depth: %u\n", pE->GetInheritDepth() );
	printf( "Enemy isA Entity: %s\n", pE->IsA( CReflection::GetInstance().FindClassByTypeID( TypeName<Entity>() ) ) ? "yes" : "no" );
	printf( "Enemy isA Entity (CName): %s\n",
		pE->IsA( CName( TypeName<Entity>() ) ) ? "yes" : "no" );
	printf( "m_Damage via Enemy: %s\n", pE->FindField("m_Damage") ? "found" : "not found" );
	printf( "m_Id via Enemy+base: %s\n", pE->FindField("m_Id", true) ? "found" : "not found" );
	// Calculate base offset
	const CClass* pEnt = CReflection::GetInstance().FindClassByTypeID( TypeName<Entity>() );
	printf( "Entity offset in Enemy: %td\n", pE->CalculateBaseOffset( pEnt ) );
	// Subclass walk
	printf( "Entity subclass count: %u\n", pEnt->GetSubClassCount() );
	for( uint32 i = 0; i < pEnt->GetSubClassCount(); ++i )
		printf( "  sub[%u]: %s (off=%td)\n",
			i, pEnt->GetSubClass(i)->GetClassName().c_str(), pEnt->GetSubClassOff(i) );
	return 0;
}
