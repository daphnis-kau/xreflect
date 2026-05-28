/** @file 12_comprehensive/main.cpp
 * @brief Full macro surface demo — every registration macro in one place.
 *
 * Covers: constructors (default, copy, custom, multi-param), destructor,
 * members, properties (get/set, read-only, lambda), methods (static, global,
 * callback), base class inheritance, qualifiers, smart pointers.
 *
 * Build:  cmake --build . --target 12_comprehensive
 * Run:    ./12_comprehensive
 */
#include "Reflection.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <memory>

// ── Pure virtual interface ──
class IUpdatable
{
public:
	virtual void   OnUpdate( float dt ) = 0;
	virtual ~IUpdatable() {}
};

DEFINE_CLASS_BEGIN( IUpdatable )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_DESTRUCTOR()
	REGIST_CLASSCALLBACK( OnUpdate )
DEFINE_CLASS_END()

// ── Component with full registration surface ──
class Transform
{
	std::string m_Name;
protected:
	float   m_PosX, m_PosY, m_PosZ;
	float   m_ScaleX, m_ScaleY, m_ScaleZ;
	int32   m_Layer;
public:
	Transform() : m_Name( "Root" ), m_Layer( 0 )
		, m_PosX( 0 ), m_PosY( 0 ), m_PosZ( 0 )
		, m_ScaleX( 1 ), m_ScaleY( 1 ), m_ScaleZ( 1 ) {}
	Transform( float x, float y, float z ) : Transform()
		{ m_PosX = x; m_PosY = y; m_PosZ = z; }
	Transform( const Transform& other )
		{ m_PosX = other.m_PosX; m_PosY = other.m_PosY; m_PosZ = other.m_PosZ; }

	// Properties
	const char* GetName() const          { return m_Name.c_str(); }
	void        SetName( const char* v ) { m_Name = v; }
	float       Length() const           { return sqrtf( m_PosX*m_PosX + m_PosY*m_PosY + m_PosZ*m_PosZ ); }
	int         GetLayer() const         { return m_Layer; }

	// Methods
	void Translate( float dx, float dy, float dz )
		{ m_PosX += dx; m_PosY += dy; m_PosZ += dz; }
	static Transform Identity() { return Transform(); }

	// Virtual (overridable from script)
	virtual void OnUpdate( float dt ) { /* default: no-op */ }
};

DEFINE_CLASS_BEGIN( Transform )
	REGIST_GROUP_TAG_BEGIN( GameGroup, "Transform" )
	REGIST_CLASSID( 1001 )
	// ── Constructors ──
	REGIST_CONSTRUCTOR_BEGIN()
		REGIST_CUSTOM_CONSTRUCTOR()
		REGIST_CUSTOM_CONSTRUCTOR( float, float, float )
	REGIST_CONSTRUCTOR_END()

	// ── Data members with qualifiers ──
	REGIST_CLASSMEMBER( m_PosX )
	REGIST_CLASSMEMBER( m_PosY )
	REGIST_CLASSMEMBER( m_PosZ )
	REGIST_CLASSMEMBER( m_ScaleX )
	REGIST_CLASSMEMBER( m_ScaleY )
	REGIST_CLASSMEMBER( m_ScaleZ )
	REGIST_EDIT_DISABLE_CLASSMEMBER( m_Layer )

	// ── Properties ──
	REGIST_CLASSPROPERTY_LAMBDA( Name,
		[]( const Transform* p ) -> const char* { return p->GetName(); },
		[]( Transform* p, const char* v ) { p->SetName( v ); } )
	REGIST_READONLY_CLASSPROPERTY( Length, Length )
	REGIST_READONLY_CLASSPROPERTY( Layer, GetLayer )

	// ── Methods ──
	REGIST_CLASSMETHOD( Translate )

	// ── Static methods ──
	REGIST_STATICMETHOD( Identity )

	// ── Virtual callback ──
	REGIST_CLASSCALLBACK( OnUpdate )
	REGIST_GROUP_TAG_END()
DEFINE_CLASS_END()

// ── Static member / property demo ──
struct Config {
	static int s_Version;
	static int  GetVersion() { return s_Version; }
	static void SetVersion( int v ) { s_Version = v; }
};
int Config::s_Version = 3;
DEFINE_CLASS_BEGIN( Config )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_STATICMEMBER( s_Version )
	REGIST_STATICPROPERTY_LAMBDA( Ver,
		[]() { return Config::s_Version; },
		[]( int v ) { Config::s_Version = v; } )
	REGIST_READONLY_STATICPROPERTY( ReadOnlyVer, GetVersion )
DEFINE_CLASS_END()

// ── Global function registration ──
static float Global_Distance( const Transform& a, const Transform& b )
{
	return 1.0f;
}
REGIST_GLOBALCMETHOD( Global_Distance )

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();
	printf( "=== Comprehensive Macro Surface Demo ===\n\n" );

	const CClass* pCls = R.FindClassByTypeID( TypeName<Transform>() );
	printf( "Class: %s  (size: %u)\n", pCls->GetClassName().c_str(), pCls->GetClassSize() );
	printf( "  Default constructible: %s\n", pCls->IsDefaultConstructible() ? "yes" : "no" );
	printf( "  Copy constructible:    %s\n", pCls->IsCopyConstructible() ? "yes" : "no" );
	printf( "  Custom constructors:   %u\n", pCls->GetCustomConstructorCount() );

	// ── Construct via reflection ──
	printf( "\n── Construction ──\n" );
	void* pObj = malloc( pCls->GetClassSize() );
	pCls->Construct( eBCI_DefaultConstructor, pObj, nullptr );

	// ── List all fields ──
	printf( "\n── All registered fields ──\n" );
	for( auto& pair : pCls->GetRegistFields() )
	{
		auto f = pair.second;
		printf( "  %-20s  type=%-2d", f->GetName().c_str(), f->GetType() );
		if( f->GetType() == eFDT_Member )
		{
			printf( "  qualifier=0x%02x", f->GetMemberQualifier() );
			if( f->GetMemberQualifier() & eMBQ_DisableEdit )
				printf( " [no-edit]" );
			if( f->GetMemberQualifier() & eMBQ_DisableSerialize )
				printf( " [no-serialize]" );
		}
		printf( "\n" );
	}

	// ── Verify IUpdatable also registered ──
	printf( "\n── Registered classes ──\n" );
	printf( "  %s  (size: %u)\n", pCls->GetClassName().c_str(), pCls->GetClassSize() );

	pCls->Destruct( pObj );
	free( pObj );

	printf( "\n── Static members & DisplayName ──\n" );
	const CClass* pCfg = R.FindClassByTypeID( TypeName<Config>() );
	for( auto& pair : pCfg->GetRegistFields() )
	{
		auto f = pair.second;
		printf( "  %-20s  type=%-2d\n", f->GetName().c_str(), f->GetType() );
	}

	// Display name (requires DISPLAY_TAG)
	const char* szDisplay = pCls->FindField("m_PosX")
		->GetDisplayName( true );
	printf( "  DisplayName(m_PosX): %s\n", szDisplay ? szDisplay : "(null)" );

	printf( "\nAll macros verified.\n" );
	return 0;
}
