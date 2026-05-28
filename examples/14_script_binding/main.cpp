#include "lua_binding/CScriptLua.h"
#include "js_binding/CScriptJS.h"
#include <functional>
#include <chrono>
#include <set>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

#include "CApplication.h"

DEFINE_NS_CLASS_BEGIN( XReflect, CName )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_STATICMETHOD( GetNameByIndex )
	REGIST_CLASSMETHOD( GetIndex )
	REGIST_CLASSMETHOD( c_str )
DEFINE_CLASS_END()

DEFINE_NS_CLASS_BEGIN( Base, CApp )
	REGIST_STATICMETHOD( GetInstance )
DEFINE_CLASS_END();

DEFINE_ENUM_BEGIN( ETestEnum )
DEFINE_ENUM_END();

DEFINE_CLASS_BEGIN( SAddress )
	REGIST_CONSTRUCTOR_BEGIN()
		REGIST_CUSTOM_CONSTRUCTOR( uint32, uint16 )
	REGIST_CONSTRUCTOR_END()
	REGIST_CLASSMEMBER( nIP )
	REGIST_CLASSMEMBER( nPort )
	REGIST_CLASSPROPERTY( port, GetPort, SetPort )
	REGIST_CLASSCALLBACK( OnTestVirtualWithConstruct )
DEFINE_CLASS_END();

DEFINE_CLASS_BEGIN( SApplicationConfig )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSPROPERTY_LAMBDA( szName,
		[]( OrgClass* pConfig ) { return (const char*)pConfig->szName; },
		[]( OrgClass* pConfig, const char* szName )
		{ strcpy( pConfig->szName, szName ); } )
	REGIST_CLASSMETHOD( SetName )
	REGIST_CLASSMETHOD( GetName )
	REGIST_CLASSMEMBER( nID )
	REGIST_CLASSMEMBER( Address )
	REGIST_CLASSMEMBER( aryInt )
DEFINE_CLASS_END();

DEFINE_CLASS_BEGIN( IApplicationHandler )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSCALLBACK( OnTestPureVirtual )
	REGIST_CLASSCALLBACK( OnTestNoParamPureVirtual )
	REGIST_CLASSCALLBACK( TestBuffer )
DEFINE_CLASS_END();

DEFINE_CLASS_BEGIN( CApplication )
	REGIST_BASE_CLASS( Base::CApp )
	REGIST_DESTRUCTOR()
	REGIST_CLASSMETHOD( TestTemplateMap )
	REGIST_CLASSMETHOD( TestCallObjectPointer )
	REGIST_CLASSMETHOD( TestCallObjectReference )
	REGIST_CLASSMETHOD( TestCallObjectValue )
	REGIST_CLASSCALLBACK( TestVirtualObjectPointer )
	REGIST_CLASSCALLBACK( TestVirtualObjectReference )
	REGIST_CLASSCALLBACK( TestVirtualObjectValue )
	REGIST_CLASSMETHOD( TestCallPOD )
	REGIST_CLASSMETHOD( TestNoParamFunction )
	REGIST_CLASSMETHOD( TestBuffer )
	REGIST_STATICMETHOD( GetInst )
DEFINE_CLASS_END();

static bool ReadFileContent( const char* szPath, std::vector<char>& Buffer )
{
	FILE* f = fopen( szPath, "rb" );
	if( !f ) return false;
	fseek( f, 0, SEEK_END );
	long nSize = ftell( f );
	fseek( f, 0, SEEK_SET );
	Buffer.resize( nSize + 1 );
	size_t nRead = fread( Buffer.data(), 1, nSize, f );
	fclose( f );
	Buffer[nRead] = 0;
	return true;
}

int main()
{
	printf( "=== Script Binding Test ===\n" );
	CApplication::GetInst();

	// Derive script directory from __FILE__
	std::string strDir( __FILE__ );
	auto nPos = strDir.find_last_of( "\\/" );
	if( nPos != std::string::npos )
		strDir = strDir.substr( 0, nPos + 1 );

	std::vector<char> Buffer;

	// Lua test
	std::string strLua = strDir + "test.lua";
	if( ReadFileContent( strLua.c_str(), Buffer ) )
	{
		ScriptBinding::CScriptLua LuaScript;
		LuaScript.RunString( Buffer.data(), Buffer.size() - 1, "test.lua", true );
		LuaScript.RunFunction( nullptr, "StartApplication", "sampler", 12345 );
	}

	// JS test
	std::string strJS = strDir + "test.js";
	if( ReadFileContent( strJS.c_str(), Buffer ) )
	{
		ScriptBinding::CScriptJS JSScript( nullptr, 0 );
		JSScript.RunString( Buffer.data(), Buffer.size() - 1, "test.js", true );
		JSScript.RunFunction( nullptr, "StartApplication", "sampler", 12345 );
	}

	return 0;
}
