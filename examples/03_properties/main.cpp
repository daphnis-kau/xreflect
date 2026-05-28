/** @file 03_properties/main.cpp */
#include "Reflection.h"
#include <cstdio>
#include <string>

class Player
{
public:
	std::string m_Name;
	int         m_Health, m_MaxHealth, m_InternalId;
	Player() : m_Name( "Unnamed" ), m_Health( 100 ), m_MaxHealth( 100 ), m_InternalId( 0 ) {}
	const std::string& GetName() const    { return m_Name; }
	void  SetName( const std::string& v ) { m_Name = v; }
	int   GetHealth()  const { return m_Health; }
	void  SetHealth( int v ) { m_Health = v; }
	float HealthRatio() const { return (float)m_Health / m_MaxHealth; }
};

DEFINE_CLASS_BEGIN( Player )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSPROPERTY( Name, GetName, SetName )
	REGIST_CLASSPROPERTY( Health, GetHealth, SetHealth )
	REGIST_READONLY_CLASSPROPERTY( HealthRatio, HealthRatio )
	REGIST_READONLY_CLASSPROPERTY_LAMBDA( MaxHealth,
		[]( const Player* p ) { return p->m_MaxHealth; } )
	REGIST_EDIT_DISABLE_CLASSMEMBER( m_InternalId )
	REGIST_SERIALIZE_DISABLE_CLASSMEMBER( m_MaxHealth )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	const CClass* pCls = CReflection::GetInstance().FindClassByTypeID( TypeName<Player>() );
	printf( "=== Properties & Qualifiers ===\n" );
	Player p;
	const CField* pName = pCls->FindField( "Name" );
	std::string name = pName->Get<Player, std::string>( &p );
	printf( "Name: %s\n", name.c_str() );
	const CField* pMH = pCls->FindField( "MaxHealth" );
	printf( "MaxHealth (lambda read-only): %d\n",
		pMH->Get<Player, int>( &p ) );
	const CField* pId = pCls->FindField( "m_InternalId" );
	printf( "m_InternalId edit-disabled: %s\n",
		( pId->GetMemberQualifier() & eMBQ_DisableEdit ) ? "yes" : "no" );
	return 0;
}
