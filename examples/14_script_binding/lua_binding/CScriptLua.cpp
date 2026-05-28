#ifdef _WIN32
#include <excpt.h>
#include <windows.h>
#elif( defined _ANDROID )
#include <alloca.h>
#endif
#include <locale>
#include <set>

#undef min
#undef max

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
	#include "lstate.h"
}

#include "CField.h"
#include "CClass.h"
#include "CReflection.h"
#include "CScriptLua.h"
#include "CTypeLua.h"

namespace ScriptBinding
{
	//=====================================================================
	/// All Lua data types
	//=====================================================================
	static CLuaTypeBase* s_aryLuaTypes[] =
	{
		nullptr,
		&TLuaValue<char>::GetInst(),
		&TLuaValue<int8>::GetInst(),
		&TLuaValue<int16>::GetInst(),
		&TLuaValue<int32>::GetInst(),
		&TLuaValue<int64>::GetInst(),
		&TLuaValue<long>::GetInst(),
		&TLuaValue<uint8>::GetInst(),
		&TLuaValue<uint16>::GetInst(),
		&TLuaValue<uint32>::GetInst(),
		&TLuaValue<uint64>::GetInst(),
		&TLuaValue<ulong>::GetInst(),
		&TLuaValue<wchar_t>::GetInst(),
		&TLuaValue<char16_t>::GetInst(),
		&TLuaValue<char32_t>::GetInst(),
		&TLuaValue<bool>::GetInst(),
		&TLuaValue<float>::GetInst(),
		&TLuaValue<double>::GetInst(),
		&TLuaValue<const char*>::GetInst(),
	};

	inline CLuaTypeBase* GetLuaTypeBase( UPakDataType Type )
	{
		uint32 nPoints = Type.GetMulPointer() + Type.IsReference();
		if( nPoints >= 2 )
			return &TLuaValue<void*>::GetInst();

		if( Type.IsBaseType() )
		{
			if( nPoints )
				return &TLuaValue<void*>::GetInst();
			return s_aryLuaTypes[Type.GetDataType()];
		}

		auto pClass = Type.GetClass();
		if( pClass->IsEnum() )
		{
			if( nPoints )
				return &TLuaValue<void*>::GetInst();
			uint32 nSize = pClass->GetClassAlignSize();
			if( nSize == 1 )
				return s_aryLuaTypes[eDT_int8];
			if( nSize == 2 )
				return s_aryLuaTypes[eDT_int16];
			return s_aryLuaTypes[eDT_int32];
		}
		else
		{
			if( !nPoints )
				return &CLuaValueObject::GetInst();
			return &CLuaObject::GetInst();
		}
	}

	//====================================================================================
	// CScriptLua::SRaiiLua
	//====================================================================================
	CScriptLua::SRaiiLua::SRaiiLua( lua_State* pL, CScriptLua* pScript )
		: m_pScript( pScript ? pScript : CScriptLua::ToScriptLua( pL ) )
	{
		m_pScript->m_vecLuaState.push_back( pL );
	}

	CScriptLua::SRaiiLua::~SRaiiLua()
	{
		m_pScript->m_vecLuaState.pop_back();
	}

	//====================================================================================
	// CScriptLua
	//====================================================================================
	char* const CScriptLua::ms_pPointerStart = (char*)
		"(global_weakvalue)\0"
		"(reference_table)\0"
		"(script_lua)\0"
		"(first_class_info)\0"
		"(error_handler)\0"
		"(CppObject)\0"
		"(CClass)\0";

	char* NextPointer( const char* szStr )
	{
		return (char*)( szStr + strlen( szStr ) + 1 );
	}

	char* const CScriptLua::ms_pWeakTable = ms_pPointerStart;
	char* const CScriptLua::ms_pReferenceTable = NextPointer( ms_pWeakTable );
	char* const CScriptLua::ms_pScriptLua = NextPointer( ms_pReferenceTable );
	char* const CScriptLua::ms_pFirstClassInfo = NextPointer( ms_pScriptLua );
	char* const CScriptLua::ms_pErrorHandler = NextPointer( ms_pFirstClassInfo );
	char* const CScriptLua::ms_pCppObject = NextPointer( ms_pErrorHandler );
	char* const CScriptLua::ms_pCppClass = NextPointer( ms_pCppObject );
	char* const CScriptLua::ms_pPointerEnd = NextPointer( ms_pCppClass );

	CScriptLua::CScriptLua()
		: m_bPreventExeInRunString( false )
	{
		lua_State* pL = lua_newstate( &CScriptLua::Realloc, this, 0 );

		m_vecLuaState.push_back( pL );
		luaL_openlibs( pL );

		// Avoid using ipairs or pairs, LuaJIT optimizes them poorly
		const char* szDefClass =
			// Newly added class derives to child class
			"local function __derive_to_child( child, key, value, orgFun )\n"
			"	local child_vtable = rawget( child, \"__virtual_table\" )\n"
			"	if rawget( child_vtable, key ) == orgFun then\n"
			"		rawset( child_vtable, key, value )\n"
			"	end\n"
			"	local derive_list = child.__derive_list"
			"	for i = 1, #derive_list do\n"
			"		__derive_to_child( derive_list[i], key, value, orgFun )\n"
			"	end\n"
			"end\n"

			// Child inherits functions from base
			"local function __inherit_from_base( child, base )\n"
			"	local base_vtable = rawget( base, \"__virtual_table\" )\n"
			"	local child_vtable = rawget( child, \"__virtual_table\" )\n"
			"	local k, v = next( base_vtable )\n"
			"	while k do\n"
			"		if rawget( child_vtable, k ) == nil then\n"
			"			rawset( child_vtable, k, v )\n"
			"		end\n"
			"		k, v = next( base_vtable, k )\n"
			"	end\n"
			"	local base_list = base.__base_list\n"
			"	for i = 1, #base_list do\n"
			"		__inherit_from_base( child, base_list[i] )\n"
			"	end\n"
			"end\n"

			// Check on the inheritance tree whether check_node is a base of cur_node
			"local function SearchClassNode( cur_node, check_node )\n"
			"	if( cur_node == check_node )then\n"
			"	    return true\n"
			"	end\n"

			"	local base_list = rawget( cur_node, \"__base_list\")\n"
			"	for i = 1, #base_list do\n"
			"	    if( SearchClassNode( base_list[i], check_node ) ) then\n"
			"	        return true\n"
			"	    end\n"
			"	end\n"

			"	return false\n"
			"end\n"

			// Create a class
			"local GetIndexClosure = select( 1, ... )\n"
			"local GetNewIndexClosure = select( 2, ... )\n"
			"function class( ... )\n"
			"	local NewClass = {}\n"
			"	local VirtualTable = {}\n"
			"	local BaseList = { ... }\n"
			"	NewClass.__base_list = BaseList\n"
			"	NewClass.__derive_list = {}\n"
			"	NewClass.__virtual_table = VirtualTable\n"
			"	NewClass.__index = GetIndexClosure(VirtualTable)\n"
			"	NewClass.__newindex = GetNewIndexClosure(VirtualTable)\n"

			"	NewClass.__GetSuperClass = function( n )\n"
			"		return BaseList[n or 1];\n"
			"	end\n"
			"	function VirtualTable.__IsInheritFrom( self, class )\n"
			"	    return SearchClassNode( NewClass, class )\n"
			"	end\n"
			"	function VirtualTable.__GetClass( self )\n"
			"	    return NewClass\n"
			"	end\n"

			"	VirtualTable.class = NewClass\n"

			"	local nBaseCount = #BaseList\n"
			"	for i = 1, nBaseCount do\n"
			"	    table.insert( BaseList[i].__derive_list, NewClass )\n"
			"		__inherit_from_base( NewClass, BaseList[i] )\n"
			"	end\n"

			// Class metatable
			"	local class_metatable = {}\n"
			"	class_metatable.__index = VirtualTable\n"
			"	class_metatable.__newindex = function( class, key, value )\n"
			"		local vtable = rawget( class, \"__virtual_table\" )"
			"		__derive_to_child( class, key, value, rawget( vtable, key ) )\n"
			"	end\n"
			"	class_metatable.__call = function( _, ... )\n"
			"	    local NewInstance = {}\n"
			"	    setmetatable( NewInstance, NewClass )\n"
			"	    if( NewClass.construction )then\n"
			"			NewClass.construction( NewInstance, ... )\n"
			"	    end\n"
			"	    return NewInstance\n"
			"	end\n"

			"	setmetatable( NewClass, class_metatable )\n"
			"	return NewClass\n"
			"end\n"

			// Check on the inheritance tree whether check_node is a base of cur_node; if not, type conversion is not allowed
			// If so, also check whether there is a base class on cur_node's inheritance tree with discontiguous memory from check_node
			// If there is, type conversion is also not allowed
			// Return results:
			// first: >0 means convertible, other values mean not convertible
			// second: indicates the closest C++ base class
			// Note: discontiguous memory means the Lua class inherits from two or more C++ classes,
			//    and the memory blocks used by its instance are not contiguous
			"local function CheckClassNode( cur_node, check_node )\n"
			"    if( cur_node == check_node ) then\n"
			"        return 1, nil\n"
			"    end\n"

			"    local IsCurCppClass = rawget( cur_node, \"__class_info\")\n"
			"    local BaseList = rawget( cur_node, \"__base_list\")\n"
			"    local FoundCount = 0\n"
			"    local BaseCppClass = nil\n"

			"	for i = 1, #BaseList do\n"
			"		local IsBaseCppClass = rawget( BaseList[i], \"_info\")\n"
			"		local re, base = CheckClassNode( BaseList[i], check_node )\n"
			// Cannot perform type conversion
			"		if( re == -1 ) then\n"
			"			return -1, nil\n"
			"		elseif( re == 0 ) then\n"
			// Discontiguous memory exists
			"		if( not IsCurCppClass and IsBaseCppClass ) then\n"
			"			return -1, nil\n"
			"		end\n"
			"		else\n"
			// Not the unique base class
			"		if( FoundCount > 0 ) then\n"
			"			return -1, nil\n"
			"			end\n"
			"			FoundCount = 1\n"
			"			BaseCppClass = IsCurCppClass and cur_node or base\n"
			"		end\n"
			"	end\n"

			"	return FoundCount, BaseCppClass\n"
			"end\n"

			// Type conversion
			"local __cpp_cast = select( 3, ... )\n"
			"function ClassCast( class, obj, ... )\n"
			"	local obj_class = getmetatable( obj )\n"
			"	if( obj_class == nil )then\n"
			"		return nil\n"
			"	end\n"

			"	if( SearchClassNode( obj_class, class ) ) then\n"
			"		return obj\n"
			"	end\n"

			"	local re, base = CheckClassNode( class, obj_class )\n"
			"	if( re > 0 ) then\n"
			// If there is a C++ base class, perform C++ conversion first
			"		if( base ) then\n"
			"			__cpp_cast( obj, base )\n"
			"		end\n"
			"		setmetatable( obj, class )\n"
			"		if( class.construction )then\n"
			"			class.construction( obj, ... )\n"
			"	    end\n"
			"		return obj\n"
			"	end\n"

			"	return nil\n"
			"end\n";

		lua_pushlightuserdata( pL, ms_pScriptLua );
		lua_pushlightuserdata( pL, this );
		lua_rawset( pL, LUA_REGISTRYINDEX );

		// Generate CScriptLua::ms_pGlobObjectWeakValue
		lua_pushlightuserdata( pL, CScriptLua::ms_pWeakTable );
		lua_newtable( pL );
		lua_newtable( pL );
		lua_pushstring( pL, "v" );
		lua_setfield( pL, -2, "__mode" );
		lua_setmetatable( pL, -2 );
		lua_rawset( pL, LUA_REGISTRYINDEX );

		// Generate CScriptLua::ms_pGlobObjectTableKey
		lua_pushlightuserdata( pL, CScriptLua::ms_pReferenceTable );
		lua_newtable( pL );
		lua_rawset( pL, LUA_REGISTRYINDEX );
		
		auto funRead = []( lua_State* L, void* pContext, size_t* pSize )
			{
				auto szStr = *(const char**)pContext;
				*(const char**)pContext = nullptr;
				*pSize = szStr ? strlen( szStr ) : 0;
				return szStr;
			};

		if( !lua_load( pL, funRead, &szDefClass, "@#DefClass", "text" ) )
		{
			PushCppObjWeakTable( pL );
			lua_pushcclosure( pL, &CScriptLua::GetIndexClosure, 1 );

			PushCppObjWeakTable( pL );
			lua_pushcclosure( pL, &CScriptLua::GetNewIndexClosure, 1 );

			PushCppObjWeakTable( pL );
			lua_pushstring( pL, "__class_info" );
			lua_pushcclosure( pL, CScriptLua::ClassCast, 2 );
			lua_call( pL, 3, 0 );
		}

		RegisterPointerClass( this );
		lua_register( pL, "tostring", &CScriptLua::ToString );

		BuildRegisterInfo();
	}

	CScriptLua::~CScriptLua(void)
	{
		lua_close( GetLuaState() );
	}
	
	//==============================================================================
	// aux function
	//==============================================================================
	void CScriptLua::PushCppObjWeakTable( lua_State* pL )
	{
		lua_pushlightuserdata( pL, CScriptLua::ms_pWeakTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );
	}

	int32 CScriptLua::GetIndexClosure( lua_State* pL )
	{
		lua_pushvalue( pL, lua_upvalueindex( 1 ) );
		lua_pushcclosure( pL, CScriptLua::GetInstanceField, 2 );
		return 1;
	}

	int32 CScriptLua::GetNewIndexClosure( lua_State* pL )
	{
		lua_pushvalue( pL, lua_upvalueindex( 1 ) );
		lua_pushcclosure( pL, CScriptLua::SetInstanceField, 2 );
		return 1;
	}

	int32 CScriptLua::GetInstanceField( lua_State* pL )
	{
		// GetInstanceField( instance, key )
		lua_pushvalue( pL, 2 );
		lua_rawget( pL, lua_upvalueindex( 1 ) );
		if( lua_tocfunction( pL, 3 ) != &CScriptLua::CallByLua )
			return 1;
		lua_getupvalue( pL, -1, 1 );
		CField* pField = (CField*)lua_touserdata( pL, -1 );
		if( pField->GetType() != eFDT_Member )
		{
			lua_pop( pL, 1 );
			return 1;
		}

		// Auto save/restore state 
		SRaiiLua Auto( pL );

		int32 nWeakTable = lua_upvalueindex( 2 );
		CScriptLua* pScript = Auto.m_pScript;
		UPakDataType DataType = pField->GetResultType();
		assert( !DataType.IsVoid() );
		size_t nDataSize = DataType.IsReference() 
			? sizeof(void*) : pField->GetResultSize();
		char* pDataBuf = (char*)alloca( nDataSize );

		// Get this pointer
		void* pObject = nullptr;
		UPakDataType ObjectType = pField->GetParamType( 0 );
		assert( ObjectType.GetMulPointer() == 1 && !ObjectType.IsReference() );
		CLuaTypeBase* pObjectType = GetLuaTypeBase( ObjectType );
		pObjectType->GetFromVM( ObjectType, pL, (char*)&pObject, 1, nWeakTable );
		if( pObject == nullptr )
		{
			lua_settop( pL, 0 );
			lua_pushnil( pL );
			return 1;
		}

		pField->GetData( pObject, pDataBuf );

		if( DataType.IsClassValue() )
		{
			CLuaTypeBase* pDataType = GetLuaTypeBase( DataType );
			pDataType->PushToVM( DataType, pL, pDataBuf, nWeakTable );
			auto pClass = DataType.GetClass();
			pClass->Destruct( pDataBuf );
		}
		else 
		{ 
			if( DataType.IsReference() )
			{
				if( DataType.IsClass() )
				{
					CLuaTypeBase* pDataType = GetLuaTypeBase( DataType );
					pDataType->PushToVM( DataType, pL, pDataBuf, nWeakTable );
					return 1;
				}
				pDataBuf = *(char**)pDataBuf;
				DataType = UPakDataType( DataType, DataType.GetMulPointer() );
			}
			CLuaTypeBase* pDataType = GetLuaTypeBase( DataType );
			pDataType->PushToVM( DataType, pL, pDataBuf, nWeakTable );
		}

		return 1;
	}

	int32 CScriptLua::SetInstanceField( lua_State* pL )
	{
		// SetInstanceField( instance, key, value )
		lua_pushvalue( pL, 2 );
		lua_rawget( pL, lua_upvalueindex( 1 ) );

		if( lua_tocfunction( pL, 4 ) != &CScriptLua::CallByLua )
		{
			lua_settop( pL, 3 );
			lua_rawset( pL, 1 );
			return 0;
		}

		lua_getupvalue( pL, 4, 1 );
		CField* pField = (CField*)lua_touserdata( pL, -1 );
		lua_settop( pL, 3 );
		if( pField->GetType() != eFDT_Member )
		{
			lua_rawset( pL, 1 );
			return 0;
		}

		// Auto save/restore state 
		SRaiiLua Auto( pL );

		lua_remove( pL, 2 );
		int32 nWeakTable = lua_upvalueindex( 2 );
		CScriptLua* pScript = Auto.m_pScript;
		UPakDataType DataType = pField->GetResultType();
		bool bIsClass = DataType.IsClass();
		auto nMulPointer = DataType.GetMulPointer();
		auto bClassValue = !nMulPointer && bIsClass;
		// If class member is a value-type class object, add reference to avoid construction overhead
		DataType.SetReference( bClassValue );

		assert( !DataType.IsVoid() );
		size_t nDataSize = DataType.IsClassValue() ?
			sizeof( void* ) : pField->GetResultSize();
		char* pDataBuf = (char*)alloca( nDataSize );

		// Get this pointer
		void* pObject = nullptr;
		UPakDataType ObjectType = pField->GetParamType( 0 );
		assert( ObjectType.GetMulPointer() == 1 && !ObjectType.IsReference() );
		CLuaTypeBase* pObjectType = GetLuaTypeBase( ObjectType );
		pObjectType->GetFromVM( ObjectType, pL, (char*)&pObject, 1, nWeakTable );
		if( pObject == nullptr )
		{
			lua_settop( pL, 0 );
			return 0;
		}

		if( !bClassValue )
		{
			CLuaTypeBase* pDataType = GetLuaTypeBase( DataType );
			pDataType->GetFromVM( DataType, pL, pDataBuf, 2, nWeakTable );
			pField->SetData( pObject, pDataBuf );
		}
		else
		{
			DataType = UPakDataType( DataType, 1 );
			CLuaTypeBase* pDataType = GetLuaTypeBase( DataType );
			pDataType->GetFromVM( DataType, pL, pDataBuf, 2, nWeakTable );
			pField->SetData( pObject, *(void**)pDataBuf );
		}

		lua_settop( pL, 0 );
		return 0;
	}

	//=========================================================================
	// Construction and destruction                                            
	//=========================================================================
	int32 CScriptLua::ObjectGC(lua_State* pL)
	{
		void* pUpValue = lua_touserdata( pL, lua_upvalueindex( 1 ) );
		const CClass* pClass = (const CClass*)pUpValue;
		lua_pushvalue( pL, lua_upvalueindex( 2 ) );
		lua_rawget( pL, -2 );
		void* pObject = (void*)lua_touserdata( pL, -1 );
		// No need to call UnRegisterObject, just restore the vtable,
		// because it has already been collected, so no one will reference this object
		// Calling UnRegisterObject would instead cause GC issues (table[obj] = nil would crash)
		CScriptLua* pScriptLua = ToScriptLua( pL );
		pClass->UnhookVirtualTable( pObject );
		if( !lua_islightuserdata( pL, -1 ) )
			pClass->Destruct( pObject );
		lua_pop( pL, 1 );
		return 0;
	}

	int32 CScriptLua::ObjectConstruct( lua_State* pL )
	{
		int32 nParamCount = lua_gettop( pL ) - 1;
		int32 nCallBase = lua_upvalueindex( 1 );
		int32 nWeakTable = lua_upvalueindex( 2 );		
		auto pClass = (const CClass*)lua_touserdata( pL, nCallBase );

		// Skip if C++ object is already constructed (happens when casting a C++ object to a Lua object)
		lua_pushlightuserdata( pL, ms_pCppObject );
		lua_rawget( pL, 1 );
		if( !lua_isnil( pL, -1 ) )
		{
			lua_settop( pL, 1 );
			return 1;
		}
		lua_pop( pL, 1 );

		uint32 nEvaluaError = UINT32_MAX;
		int32 nConstructor = UINT16_MAX;
		int32 nCount = pClass->GetCustomConstructorCount();
		for( int32 i = eBCI_DefaultConstructor; i < nCount; i++ )
		{
			// Ignore move constructor when copy constructor exists; script objects are GC-based, no rvalue reference concept
			if( pClass->IsCopyConstructible() && i == eBCI_MoveConstructor )
				continue;
			auto pParamInfo = pClass->GetConstructorParamsInfo( i );
			if( !pParamInfo || pParamInfo->nParamCount != nParamCount )
				continue;
			uint32 nCurError = 0;
			for( int32 j = 0; j < nParamCount; j++ )
			{
				int32 nIndex = j + 2;
				auto Type = pParamInfo->GetParamType()[j];
				auto DataType = Type.GetDataType();
				auto nPointers = Type.GetMulPointer();
				if( DataType == eDT_const_char_str && !nPointers )
				{
					if( lua_isstring( pL, nIndex ) )
						continue;
					nCurError += lua_isnumber( pL, nIndex ) ? 0x0001 : 0x0002;
				}
				else if( DataType <= eDT_enum && !nPointers )
				{
					if( lua_isnumber( pL, nIndex ) )
						continue;
					nCurError += lua_isstring( pL, nIndex ) ? 0x0001 : 0x10000;
				}
				else
				{
					if( lua_istable( pL, nIndex ) )
						continue;
					nCurError += 0x10000;
				}
			}
			if( nCurError >= nEvaluaError )
				continue;
			nConstructor = i;
			nEvaluaError = nCurError;
		}

		if( nConstructor == UINT16_MAX )
		{
			lua_settop( pL, 0 );
			return 1;
		}

		// Auto save/restore state 
		SRaiiLua Auto( pL );

		auto pParamInfo = pClass->GetConstructorParamsInfo( nConstructor );
		uint32 nParamSize = pParamInfo->nTotalParamSize;
		size_t nArgSize = nParamCount * sizeof( void* );
		char* pDataBuf = (char*)alloca( nParamSize + nArgSize );
		void** pArgArray = (void**)( pDataBuf + nParamSize );

		// Skip the first parameter
		int32 nStkId = 2;

		// The rightmost parameter of the Lua function is at the top of the Lua stack,         
		// placed in the first member of m_listParam
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
		{
			UPakDataType Type = pParamInfo->GetParamType()[nArgIndex];
			CLuaTypeBase* pParamType = GetLuaTypeBase( Type );
			pParamType->GetFromVM( Type, pL, pDataBuf, nStkId++, nWeakTable );
			pArgArray[nArgIndex] = Type.IsClassValue() ? *(void**)pDataBuf : pDataBuf;
			pDataBuf += pParamInfo->GetParamSize()[nArgIndex];
		}

		// Clear parameters other than table
		lua_settop( pL, 1 );

		void* pNewObj = NewLuaObj( pL, pClass );
		CScriptLua* pScriptLua = Auto.m_pScript;
		pClass->Construct( nConstructor, pNewObj, pArgArray );
		RegisterObject( pL, pClass, pNewObj, nWeakTable );
		return 1;
	}

	//=========================================================================
	// Type conversion                                                
	//=========================================================================
	int32 CScriptLua::ClassCast( lua_State* pL )
	{
		// arg1:obj, arg2:newClass
		int32 nWeakTable = lua_upvalueindex( 1 );	
		int32 nClassInfoStr = lua_upvalueindex( 2 );

		lua_pushvalue( pL, nClassInfoStr );
		lua_rawget( pL, -2 );					// obj, newClass, newCppClass
		auto pNewClass = (const CClass*)lua_touserdata( pL, -1 );
		lua_pop( pL, 1 );						// obj, newClass

		lua_pushlightuserdata( pL, ms_pCppClass );
		lua_rawget( pL, -3 );					// obj, newClass, orgClass
		auto pOrgClass = (const CClass*)lua_touserdata( pL, -1 );
		lua_pop( pL, 1 );						// obj, newClass

		int32 nOffset = pOrgClass->CalculateBaseOffset( pNewClass );
		if( nOffset >= 0 )
		{
			lua_pop( pL, 1 );
			return 1;
		}

		nOffset = pNewClass->CalculateBaseOffset( pOrgClass );
		if( nOffset < 0 )
		{
			lua_pop( pL, 1 );
			lua_pushnil( pL );
			return 1;
		}

		lua_setmetatable( pL, -2 );				// obj
		lua_pushlightuserdata( pL, ms_pCppObject );
		lua_rawget( pL, -2 );					// obj, CppObj
		void* pObj = (void*)lua_touserdata( pL, -1 );
		lua_pop( pL, 1 );						// obj

		pOrgClass->UnhookVirtualTable( pObj );
		if( nOffset )
		{
			pObj = ( (char*)pObj ) - nOffset;
			lua_pushlightuserdata( pL, ms_pCppObject );
			lua_pushlightuserdata( pL, pObj );
			lua_rawset( pL, -3 );
		}

		lua_pushlightuserdata( pL, ms_pCppClass );
		lua_pushlightuserdata( pL, (void*)pNewClass );
		lua_rawset( pL, -3 );
		RegisterObject( pL, pNewClass, pObj, nWeakTable );
		return 1;
	}

	//=====================================================================
	// Interface for Lua script calling C++
	//=====================================================================
	int32 CScriptLua::CallByLua(lua_State* pL)
	{
		int32 nCallBase = lua_upvalueindex( 1 );
		int32 nWeakTable = lua_upvalueindex( 2 );
		CField* pField = (CField*)lua_touserdata( pL, nCallBase );
		uint32 nTop = lua_gettop( pL );

		// Auto save/restore state 
		SRaiiLua Auto( pL );
		CScriptLua* pScript = Auto.m_pScript;

		try
		{
			uint32 nParamSize = pField->GetParamTotalSize();
			uint32 nParamCount = pField->GetParamCount();
			UPakDataType ResultType = pField->GetResultType();
			size_t nReturnSize = !ResultType.IsVoid() ?
				pField->GetResultSize() : sizeof( int64 );
			size_t nArgSize = nParamCount * sizeof( void* );
			char* pDataBuf = (char*)alloca( nParamSize + nArgSize + nReturnSize );
			void** pArgArray = (void**)( pDataBuf + nParamSize );
			char* pResultBuf = pDataBuf + nParamSize + nArgSize;
			memset( pResultBuf, 0, nReturnSize );

			int32 nStkId = 1;

			// The rightmost parameter of the Lua function is at the top of the Lua stack,         
			// placed in the first member of m_listParam
			for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
			{
				UPakDataType Type = pField->GetParamType( nArgIndex );
				CLuaTypeBase* pParamType = GetLuaTypeBase( Type );
				pParamType->GetFromVM( Type, pL, pDataBuf, nStkId++, nWeakTable );
				pArgArray[nArgIndex] = Type.IsClass() && !Type.GetMulPointer()
					? *(void**)pDataBuf : pDataBuf;
				pDataBuf += pField->GetParamSize( nArgIndex );
			}
			lua_settop( pL, 0 );

			assert( pField->GetType() != eFDT_Member );
			pField->Call( pResultBuf, pArgArray );

			if( !ResultType.IsVoid() )
			{
				CLuaTypeBase* pReturnType = GetLuaTypeBase( ResultType );
				pReturnType->PushToVM( ResultType, pL, pResultBuf, nWeakTable );
				if( ResultType.IsClassValue() )
				{
					auto pClass = ResultType.GetClass();
					pClass->Destruct( pResultBuf );
				}
			}
			return ResultType.IsVoid() ? 0 : 1;
		}
		catch( std::exception& exp )
		{
			char szBuf[256];
			sprintf( szBuf, "An unknow exception occur on calling %s\n",
				pField->GetName().c_str() );
			luaL_error( pL, exp.what() );
		}
		catch( ... )
		{
			char szBuf[256];
			sprintf( szBuf, "An unknow exception occur on calling %s\n",
				pField->GetName().c_str() );
			luaL_error( pL, szBuf );
		}

		return 0;
	}

	int32 CScriptLua::ToString( lua_State* pL )
	{
		luaL_checkany( pL, -1 );
		if( luaL_callmeta( pL, -1, "__tostring" ) )
		{
			lua_remove( pL, -2 );
			return 1;
		}

		char szDouble[256];
		int type = lua_type( pL, -1 );
		const char* s = nullptr;
		if( type == LUA_TNUMBER )
		{

			double n = lua_tonumber( pL, -1 );
			if( n != (double)((int64)n) )
			{
				sprintf( szDouble, "%lf", n );
				s = szDouble;
			}
			else
			{
				sprintf(szDouble, "%lld", (int64)n);
				s = szDouble;
			}
		}
		else if( type == LUA_TSTRING )
			return 1;
		else if( type == LUA_TBOOLEAN )
			s = lua_toboolean( pL, -1 ) ? "true" : "false";
		else if( type == LUA_TNIL )
			s = "nil";

		if( s )
		{
			lua_pop( pL, 1 );
			lua_pushstring( pL, s );
			return 1;
		}

		const void* ptr = lua_topointer( pL, -1 );
		const char* name = luaL_typename( pL, -1 );
		if( type != LUA_TTABLE )
		{
			lua_pop( pL, 1 );
			if( ptr >= ms_pPointerStart && ptr <= ms_pPointerEnd )
				lua_pushstring( pL, (const char*)ptr );
			else
				lua_pushfstring( pL, "%s: %p", name, ptr );
			return 1;
		}

		if( !lua_getmetatable( pL, -1 ) )
		{
			lua_pop( pL, 1 );
			lua_pushfstring( pL, "table: %p", ptr );
			return 1;
		}

		lua_pushstring( pL, "__class_info" );
		lua_rawget( pL, -2 );
		if( lua_isnil( pL, -1 ) )
		{
			lua_pop( pL, 3 );
			lua_pushfstring( pL, "table: %p", ptr );
			return 1;
		}

		auto pClass = (const CClass*)lua_touserdata( pL, -1 );
		lua_pushlightuserdata( pL, CScriptLua::ms_pCppObject );
		lua_rawget( pL, -4 );
		const void* pObject = lua_touserdata( pL, -1 );
		lua_pop( pL, 4 );
		lua_pushfstring( pL, "%s: %p->%p",
			pClass->GetClassName().c_str(), ptr, pObject );
		return 1;
	}

	inline CScriptLua* CScriptLua::ToScriptLua( lua_State* pL )
	{
		CScriptLua* pScriptLua = nullptr;
		lua_getallocf( pL, (void**)&pScriptLua );
		return pScriptLua;
	}

	void* CScriptLua::Realloc( void* pContex,
		void* pPreBuff, size_t nOldSize, size_t nNewSize )
	{
		return realloc( pPreBuff, nNewSize );
	}

	bool CScriptLua::SetGlobObject(lua_State* pL, const char* szKey)
	{
		lua_pushlightuserdata( pL, CScriptLua::ms_pWeakTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		lua_pushstring( pL, szKey );
		lua_pushvalue( pL, -3 );
		lua_rawset( pL, -3 );
		lua_pop( pL, 1 );
		return true;
	}

	bool CScriptLua::GetGlobObject( lua_State* pL, const char* szKey )
	{
		lua_pushlightuserdata( pL, CScriptLua::ms_pWeakTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		lua_pushstring( pL, szKey );
		lua_rawget( pL, -2 );
		lua_remove( pL, -2 );
		if( !lua_isnil( pL, -1 ) )
			return true;
		lua_pop( pL, 1 );
		return false;
	}

	void CScriptLua::BuildRegisterInfo()
	{
		lua_State* pL = GetLuaState();
		std::set<const CClass*> setRegisted;

		auto GetClassScope = [&]( lua_State* pL, const CClass* pClass )
		{
			lua_getglobal( pL, "_G" );
			for( uint8 i = 0; i < pClass->GetNameSpaceCount(); i++ )
			{
				CName NameSpace = pClass->GetNameSpace( i );
				lua_getfield( pL, -1, pClass->GetNameSpace( i ).c_str() );
				if( lua_isnil( pL, -1 ) )
				{
					lua_pop( pL, 1 );
					lua_newtable( pL );
					lua_pushvalue( pL, -1 );
					lua_setfield( pL, -3, NameSpace.c_str() );
				}
				lua_remove( pL, -2 );
			}
		};

		std::function<void( lua_State*, const CClass* )> RegistClassInfo;
		RegistClassInfo = [&]( lua_State* pL, const CClass* pClass )
		{
			if( setRegisted.find( pClass ) != setRegisted.end() )
				return;
			setRegisted.insert( pClass );
			if( pClass->IsEnum() )
			{
				GetClassScope( pL, pClass );
				lua_newtable( pL );
				lua_pushvalue( pL, -1 );
				lua_setfield( pL, -3, pClass->GetClassName().c_str() );
			}
			else if( pClass->GetRegisterType() == eCRT_NormalClass )
			{
				auto szClass = pClass->GetClassName().c_str();
				GetClassScope( pL, pClass );
				int nClassScopeIdx = lua_gettop( pL );
				lua_getfield( pL, -1, szClass );
				assert( lua_isnil( pL, -1 ) );
				lua_pop( pL, 1 );

				lua_getglobal( pL, "class" );
				assert( !lua_isnil( pL, -1 ) );

				uint32 nBaseCount = pClass->GetBaseClassCount();
				for( uint32 i = 0; i < nBaseCount; i++ )
				{
					auto pBaseClass = pClass->GetBaseClass( i );
					assert( pBaseClass != nullptr );
					GetClassScope( pL, pBaseClass );
					RegistClassInfo( pL, pBaseClass );
					auto szName = pBaseClass->GetClassName().c_str();
					lua_getfield( pL, -1, szName );
					lua_remove( pL, -2 );
					assert( !lua_isnil( pL, -1 ) );
				}

				lua_call( pL, (int32)nBaseCount, 1 );
				lua_pushlightuserdata( pL, (void*)pClass );
				lua_pushvalue( pL, -2 );
				lua_rawset( pL, LUA_REGISTRYINDEX );

				lua_pushvalue( pL, -1 );
				lua_setfield( pL, nClassScopeIdx, szClass );
				lua_remove( pL, nClassScopeIdx );
				int nClassIdx = lua_gettop( pL ); 

				// NewClass.__virtual_table._info = pInfo
				lua_pushstring( pL, "__virtual_table" );
				lua_rawget( pL, -2 );
				lua_pushlightuserdata( pL, ms_pFirstClassInfo );
				lua_pushlightuserdata( pL, (void*)pClass );
				lua_rawset( pL, -3 );
				lua_pop( pL, 1 );

				lua_pushstring( pL, "__class_info" );
				lua_pushlightuserdata( pL, (void*)pClass );
				lua_rawset( pL, nClassIdx );

				lua_pushstring( pL, "__gc" );
				lua_pushlightuserdata( pL, (void*)pClass );
				lua_pushlightuserdata( pL, CScriptLua::ms_pCppObject );
				lua_pushcclosure( pL, CScriptLua::ObjectGC, 2 );
				lua_rawset( pL, nClassIdx );

				lua_pushlightuserdata( pL, (void*)pClass );
				PushCppObjWeakTable( pL );
				lua_pushcclosure( pL, CScriptLua::ObjectConstruct, 2 );
				lua_setfield( pL, nClassIdx, "construction" );
			}
			else
			{
				GetClassScope( pL, pClass );
			}

			assert( !lua_isnil( pL, -1 ) );

			for( auto& pair : pClass->GetRegistFields() )
			{
				auto pField = pair.second;
				if( pField->GetType() == eFDT_Enumeration )
				{
					int32 nEnumValue = pField->GetEnumValue();
					lua_pushinteger( pL, nEnumValue );
				}
				else
				{
					lua_pushlightuserdata( pL, (void*)pField );
					PushCppObjWeakTable( pL );
					lua_pushcclosure( pL, CScriptLua::CallByLua, 2 );
				}
				lua_setfield( pL, -2, pField->GetName().c_str() );
			}
			lua_pop( pL, 1 );
		};

		for( auto& pair : CReflection::GetInstance().GetRegistClassMap() )
		{
			RegistClassInfo( pL, pair.second );
		}
	}

	bool CScriptLua::OnCallBack(
		const CField* pField, void* pRetBuf, void** pArgArray )
	{
		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int32 nTop = lua_gettop( pL );
		lua_pushlightuserdata( pL, CScriptLua::ms_pErrorHandler );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		int32 nErrFunIndex = nTop + 1;			// [ErrFun]

		PushCppObjWeakTable( pL );				// [ErrFun, WeakTable]
		int32 nWeakTable = nErrFunIndex + 1;

		lua_pushlightuserdata( pL, *(void**)pArgArray[0] );
		lua_gettable( pL, -2 );					// [ErrFun, WeakTable, Obj]

		int32 nType = lua_type( pL, -1 );
		if( nType == LUA_TNIL )
		{
			lua_settop( pL, nTop );      //Error occur
			return false;                
		}

		lua_getfield( pL, -1, pField->GetName().c_str() ); 

		if( lua_tocfunction( pL, -1 ) == &CScriptLua::CallByLua )
		{
			lua_getupvalue( pL, -1, 1 );
			if( pField == lua_touserdata( pL, -1 ) )
			{
				// call self
				lua_settop( pL, nTop );
				return false;
			}
		}
		else if( lua_isnil( pL, -1 ) )
		{
			//Error occur
			lua_settop( pL, nTop );
			return false;
		}

		lua_insert( pL, -2 );
		uint32 nParamCount = pField->GetParamCount();
		UPakDataType ResultType = pField->GetResultType();
		for( uint32 nArgIndex = 1; nArgIndex < nParamCount; nArgIndex++ )
		{
			UPakDataType Type = pField->GetParamType( nArgIndex );
			CLuaTypeBase* pParamType = GetLuaTypeBase( Type );
			auto pDataBuf = Type.IsClassReference()
				? (char*)&pArgArray[nArgIndex] : (char*)pArgArray[nArgIndex];
			pParamType->PushToVM( Type, pL, pDataBuf, nWeakTable );
		}

		lua_pcall( pL, (int32)nParamCount, ResultType.IsVoid() ? 0 : 1, nErrFunIndex);
		if( !ResultType.IsVoid() )
		{
			CLuaTypeBase* pReturnType = GetLuaTypeBase( ResultType );
			if( !ResultType.IsClassValue() )
			{
				pReturnType->GetFromVM( 
					ResultType, pL, (char*)pRetBuf, -1, nWeakTable );
			}
			else
			{
				void* pObject = nullptr;
				pReturnType->GetFromVM( 
					ResultType, pL, (char*)&pObject, -1, nWeakTable );
				ResultType.GetClass()->Assign( pRetBuf, pObject  );
			}
		}
		lua_settop( pL, nTop );
		return true;
	}

	void CScriptLua::OnDestruct( const CField* pField, void* pObject )
	{
		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		lua_pushlightuserdata( pL, CScriptLua::ms_pErrorHandler );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		int32 nErrFunIndex = lua_gettop(pL);		// 1

		lua_pushlightuserdata( pL, CScriptLua::ms_pWeakTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );		// 2	

		lua_pushlightuserdata( pL, pObject );
		lua_gettable( pL, -2 );						// 3

		if( lua_isnil( pL, -1 ) )
		{
			lua_pop( pL, 3 );        //Error occur
			return;                
		}

		lua_getfield( pL, -1, "deconstruction" ); // 4

		if( lua_tocfunction( pL, -1 ) == &CScriptLua::CallByLua )
		{
			lua_getupvalue( pL, -1, 1 );
			if( pField == lua_touserdata( pL, -1 ) )
			{
				// call self
				lua_pop( pL, 5 );
				return;
			}
		}
		else if( lua_isnil( pL, -1 ) )
		{
			//Error occur
			lua_pop( pL, 4 );
			return;
		}

		lua_insert( pL, -2 );
		lua_pcall( pL, 1, 0, nErrFunIndex );
		lua_settop( pL, nErrFunIndex - 1 );
	}

	//-------------------------------------------------------------------------------
	// General functions
	//--------------------------------------------------------------------------------
	// Lua stack must contain only one value: the class (table, at bottom).
	// After call, stack top = 1, the object's table is at top
	void* CScriptLua::NewLuaObj( lua_State* pL, const CClass* pClass )
	{
		lua_pushlightuserdata( pL, ms_pCppObject );
		void* pNewObj = lua_newuserdatauv( pL, pClass->GetClassSize(), 0 );
		lua_rawset( pL, -3 );
		lua_pushlightuserdata( pL, ms_pCppClass );
		lua_pushlightuserdata( pL, (void*)pClass );
		lua_rawset( pL, -3 );
		return pNewObj;
	}

	void CScriptLua::RegistToLua( lua_State* pL, const CClass* pClass,
		void* pObj, int32 nObj, int32 nGlobalWeakTable )
	{
		// __addTableOfUserdata, attach the object table,
		// to CScriptLua::ms_pGlobObjectWeakTableKey
		lua_pushlightuserdata( pL, pObj );
		lua_pushvalue( pL, nObj );        // Return Obj at the bottom of the stack
		lua_settable( pL, nGlobalWeakTable );

		uint32 nBaseClassCount = pClass->GetBaseClassCount();
		for( uint32 i = 0; i < nBaseClassCount; i++ )
		{
			void* pBase = ( (char*)pObj ) + pClass->GetBaseClassOff( i );
			const CClass* pBaseClass = pClass->GetBaseClass( i );
			RegistToLua( pL, pBaseClass, pBase, nObj, nGlobalWeakTable );
		}
	}

	void CScriptLua::RegisterObject( lua_State* pL, 
		const CClass* pClass, void* pObj, int32 nWeakTable )
	{
		// In C++, stack top = 1, return Obj, leave it on the stack
		if( pClass->GetOverridableCount() )
			pClass->HookVirtualTable( pObj, ToScriptLua( pL ) );

		int32 nObj = lua_gettop( pL );
		// Set global object table CScriptLua::ms_pGlobObjectWeakTableKey  
		RegistToLua( pL, pClass, pObj, nObj, nWeakTable );
	}

	const char* CScriptLua::PresentValue(
		lua_State* pL, int32 nStkId, size_t* nLen )
	{
		lua_pushvalue( pL, nStkId );
		CScriptLua::ToScriptLua( pL )->ToString( pL );
		const char* szValue = lua_tolstring( pL, -1, nLen );
		lua_pop( pL, 1 );
		return szValue;
	}

	lua_State* CScriptLua::GetLuaState()
	{
		assert(!m_vecLuaState.empty());
		return *m_vecLuaState.rbegin();
	}

	CScriptLua* CScriptLua::GetScript( lua_State* pL )
	{
		return ToScriptLua( pL );
	}

	bool CScriptLua::Evaluate( const char* szExpression, 
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ResultType( TypeInfo );
		if( ResultType.IsVoid() || !pResultBuf )
			return false;

		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		const char* szFun = "return %s";
		char szFuncBuf[256];
		sprintf( szFuncBuf, szFun, szExpression );
		if( !GetGlobObject( pL, szFuncBuf ) &&
			( luaL_loadstring( pL, szFuncBuf ) ||
			 !SetGlobObject( pL, szFuncBuf ) ) )
		{
			lua_settop( pL, nTop );
			return false;
		}
		lua_pcall( pL, 0, LUA_MULTRET, 0 );

		bool IsNil = lua_isnil( pL, -1 );
		GetLuaTypeBase( ResultType )->GetFromVM( 
			ResultType, pL, (char*)pResultBuf, -1, nTop + 1 );
		lua_settop( pL, nTop );
		return !IsNil;
	}

	bool CScriptLua::Set( void* pObject, int32 nIndex,
		void* pArgBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ParamType( TypeInfo );
		if( ParamType.IsVoid() || !pArgBuf || !pObject )
			return false;

		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		if( !PushPointerToLua( pL, pObject, nTop + 1, false ) )
			return lua_settop( pL, nTop ), false;
		lua_pushinteger( pL, nIndex );
		CLuaTypeBase* pParamType = GetLuaTypeBase( ParamType );
		pParamType->PushToVM( ParamType, pL, (char*)pArgBuf, nTop + 1 );
		lua_settable( pL, -3 );
		lua_settop( pL, nTop );
		return true;
	}

	bool CScriptLua::Get( void* pObject, int32 nIndex, 
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ResultType( TypeInfo );
		if( ResultType.IsVoid() || !pResultBuf || !pObject )
			return false;

		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		if( !PushPointerToLua( pL, pObject, nTop + 1, false ) )
			return lua_settop( pL, nTop ), false;

		lua_pushinteger( pL, nIndex );
		lua_gettable( pL, -2 );
		bool IsNil = lua_isnil( pL, -1 );

		GetLuaTypeBase( ResultType )->GetFromVM( 
			ResultType, pL, (char*)pResultBuf, -1, nTop + 1 );
		lua_settop( pL, nTop );
		return !IsNil;
	}

	bool CScriptLua::Set( void* pObject, const char* szName,
		void* pArgBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ParamType( TypeInfo );
		if( ParamType.IsVoid() || !pArgBuf )
			return false;

		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		if( pObject == nullptr )
		{
			CLuaTypeBase* pParamType = GetLuaTypeBase( ParamType );
			pParamType->PushToVM( ParamType, pL, (char*)pArgBuf, nTop + 1 );
			const char* szFun = "%s = select( 1, ... )";
			char szFuncBuf[256];
			sprintf( szFuncBuf, szFun, szName );
			if( !GetGlobObject( pL, szFuncBuf ) &&
				( luaL_loadstring( pL, szFuncBuf ) || 
				 !SetGlobObject( pL, szFuncBuf ) ) )
				return lua_settop( pL, nTop ), false;
			lua_pcall( pL, 1, LUA_MULTRET, 0 );
		}
		else
		{
			if( !PushPointerToLua( pL, pObject, nTop + 1, false ) )
				return lua_settop( pL, nTop ), false;
			CLuaTypeBase* pParamType = GetLuaTypeBase( ParamType );
			pParamType->PushToVM( ParamType, pL, (char*)pArgBuf, nTop + 1 );
			lua_setfield( pL, -2, szName );
		}

		lua_settop( pL, nTop );
		return true;
	}

	bool CScriptLua::Get( void* pObject, const char* szName,
		void* pResultBuf, const STypeInfo& TypeInfo )
	{
		UPakDataType ResultType( TypeInfo );
		if( ResultType.IsVoid() || !pResultBuf )
			return false;

		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		if( pObject == nullptr )
			lua_getglobal( pL, szName );
		else if( !PushPointerToLua( pL, pObject, nTop + 1, false ) )
			return lua_settop( pL, nTop ), false;
		else
			lua_getfield( pL, -1, szName );
		
		bool IsNil = lua_isnil( pL, -1 );
		GetLuaTypeBase( ResultType )->GetFromVM( 
			ResultType, pL, (char*)pResultBuf, -1, nTop + 1 );
		lua_settop( pL, nTop );
		return !IsNil;
	}

	bool CScriptLua::Call( const STypeInfoArray& aryTypeInfo, 
		void* pResultBuf, const char* szFunction, void** aryArg )
	{
		void* pFunction = nullptr;
		static STypeInfo TypeInfo = GetTypeInfo<void*>();
		if( !Get( nullptr, szFunction, &pFunction, TypeInfo ) ||
			pFunction == nullptr )
			return false;
		return Call( aryTypeInfo, pResultBuf, pFunction, aryArg );
	}

	bool CScriptLua::Call( const STypeInfoArray& aryTypeInfo,
		void* pResultBuf, void* pFunction, void** aryArg )
	{
		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		lua_pushlightuserdata( pL, ms_pErrorHandler );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		int32 nErrFunIndex = lua_gettop( pL );

		PushCppObjWeakTable( pL );
		int32 nWeakTable = nErrFunIndex + 1;

		if( !PushPointerToLua( pL, pFunction, nWeakTable, false ) 
			|| !lua_isfunction( pL, -1 ) )
		{
			lua_pop( pL, 4 );
			return false;
		}

		uint32 nParamCount = aryTypeInfo.nInfoCount - 1;
		for( uint32 nArgIndex = 0; nArgIndex < nParamCount; nArgIndex++ )
		{
			auto& TypeInfo = aryTypeInfo.aryInfo[nArgIndex];
			UPakDataType Type( TypeInfo );
			CLuaTypeBase* pParamType = GetLuaTypeBase( Type );
			auto pDataBuf = (char*)aryArg[nArgIndex];
			pParamType->PushToVM( Type, pL, pDataBuf, nWeakTable );
		}

		auto& TypeInfo = aryTypeInfo.aryInfo[nParamCount];
		UPakDataType ResultType( TypeInfo );
		bool bResultValid = !ResultType.IsVoid() && pResultBuf;
		lua_pcall( pL, nParamCount, bResultValid ? 1 : 0, nErrFunIndex );

		if( bResultValid )
		{
			GetLuaTypeBase( ResultType )->GetFromVM( 
				ResultType, pL, (char*)pResultBuf, -1, nWeakTable );
			lua_pop( pL, 2 );
		}

		lua_pop( pL, 2 );
		return true;
	}

	bool CScriptLua::RunString( const void* pBuffer, size_t nSize,
		const char* szFileName, bool bForce/* = false*/ )
	{
		// Auto save/restore state 
		lua_State* pL = GetLuaState();
		SRaiiLua Auto( pL, this );

		int32 nErrFunIndex = -1;
		if( !m_bPreventExeInRunString )
		{
			lua_pushlightuserdata( pL, CScriptLua::ms_pErrorHandler );
			lua_rawget( pL, LUA_REGISTRYINDEX ); //1
			nErrFunIndex = lua_gettop( pL );
		}

		char szBuf[2048];
		sprintf( szBuf, "@%s", szFileName );

		struct SReaderCtx { const char* pData; size_t nSize; };
		SReaderCtx ReaderCtx = { (const char*)pBuffer, nSize };
		lua_Reader funRead = []( lua_State* L, void* pContext, size_t* pSize )
			{
				auto& Ctx = *(SReaderCtx*)pContext;
				if( !Ctx.pData ) { *pSize = 0; return (const char*)nullptr; }
				*pSize = Ctx.nSize;
				auto p = Ctx.pData;
				Ctx.pData = nullptr;
				return p;
			};

		const char* aryMode[] = { "text", "binary" };
		auto szMode = aryMode[*(const int*)pBuffer == LUA_SIGNATURE[0]];
		if( ( !bForce && GetGlobObject( pL, szFileName ) ) ||
			( !lua_load( pL, funRead, &ReaderCtx, szBuf, szMode ) &&
				SetGlobObject( pL, szFileName ) ) )
		{
			if( m_bPreventExeInRunString )
				return true;
			bool re = !lua_pcall( pL, 0, LUA_MULTRET, nErrFunIndex );
			lua_remove( pL, nErrFunIndex );
			return re;
		}

		if( !m_bPreventExeInRunString )
			lua_remove( pL, nErrFunIndex );

		const char* szError = lua_tostring( pL, -1 );
		if( szError )
		{
			printf( "%s\n", szError );
			lua_remove( pL, 1 );
		}

		return false;
	}

	//-------------------------------------------------------------------------------
	// External utility functions
	//--------------------------------------------------------------------------------
	void* CScriptLua::GetVM()
	{
		return GetLuaState();
	}

	int32 CScriptLua::IncRef( void* pObj )
	{
		assert( pObj );
		lua_State* pL = GetLuaState();
		int32 nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		lua_pushlightuserdata( pL, CScriptLua::ms_pReferenceTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		assert( lua_istable( pL, -1 ) );

		if( !PushPointerToLua( pL, pObj, nTop + 1, false ) )
		{
			lua_pop( pL, 3 );
			return 0;
		}

		lua_rawget( pL, -2 );
		int32 nPreRef = lua_isnil( pL, -1 ) ? 0 : (int32)lua_tonumber( pL, -1 );
		lua_pop( pL, 1 );
		lua_pushlightuserdata( pL, pObj );
		lua_pushnumber( pL, ++nPreRef );
		lua_rawset( pL, -3 );
		lua_settop( pL, nTop );
		return nPreRef;
	}

	int32 CScriptLua::DecRef( void* pObj )
	{
		assert( pObj );
		lua_State* pL = GetLuaState();
		int32 nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );

		lua_pushlightuserdata( pL, CScriptLua::ms_pReferenceTable );
		lua_rawget( pL, LUA_REGISTRYINDEX );
		assert( lua_istable( pL, -1 ) );

		if( !PushPointerToLua( pL, pObj, nTop + 1, false ) )
		{
			lua_pop( pL, 4 );
			return 0;
		}

		lua_rawget( pL, -2 );
		int32 nPreRef = lua_isnil( pL, -1 ) ? 0 : (int32)lua_tonumber( pL, -1 );
		lua_pop( pL, 1 );
		assert( nPreRef > 0 );
		lua_pushlightuserdata( pL, pObj );
		--nPreRef ? lua_pushnumber( pL, nPreRef ) : lua_pushnil( pL );
		lua_rawset( pL, -3 );
		lua_settop( pL, nTop );
		return nPreRef;
	}

	void CScriptLua::UnlinkCppObjFromScript( void* pObj )
	{
		lua_State* pL = GetLuaState();
		int32 nTop = lua_gettop( pL );

		PushCppObjWeakTable( pL );
		int32 nWeakTable = nTop + 1;
		assert( !lua_isnil( pL, nWeakTable ) );

		lua_pushlightuserdata( pL, pObj );
		lua_gettable( pL, -2 );						//[WeakTable,tblObj]
		int32 nObjTable = nWeakTable + 1;

		if( lua_isnil( pL, -1 ) )
		{
			lua_settop( pL, nTop );
			return;
		}

		lua_pushlightuserdata( pL, ms_pCppObject );
		lua_rawget( pL, nObjTable );				//[WeakTable,tblObj,cppObj]
		pObj = lua_touserdata( pL, -1 );
		if( pObj == nullptr )
		{
			lua_settop( pL, nTop );
			return;
		}

		lua_getmetatable( pL, nObjTable );
		if( !lua_istable( pL, -1 ) )
		{
			lua_settop( pL, nTop );
			return;
		}

		lua_pushstring( pL, "__class_info" );
		lua_rawget( pL, -2 );
		auto pClass = (const CClass*)lua_touserdata( pL, -1 );
		if( pClass == nullptr )
		{
			lua_settop( pL, nTop );
			return;
		}

		std::function<void( const CClass*, tbyte* )> RemoveFromWeakTable;
		RemoveFromWeakTable = [pL, nWeakTable, &RemoveFromWeakTable]
			( const CClass* pClass, tbyte* pObject )
		{
			lua_pushlightuserdata( pL, pObject );
			lua_pushnil( pL );
			lua_rawset( pL, nWeakTable );
			for( uint32 i = 0; i < pClass->GetBaseClassCount(); i++ )
			{
				auto pBaseClass = pClass->GetBaseClass( i );
				auto nOffset = pClass->GetBaseClassOff( i );
				RemoveFromWeakTable( pBaseClass, pObject + nOffset );
			}
		};

		RemoveFromWeakTable( pClass, (tbyte*)pObj );
		lua_pushlightuserdata( pL, ms_pCppObject );
		lua_pushnil( pL );
		lua_rawset( pL, nObjTable );
		lua_settop( pL, nTop );
	}

	void CScriptLua::GC()
	{
		lua_gc(GetLuaState(), LUA_GCSTEP, 0);
	}

	void CScriptLua::GCAll()
	{
		lua_State* pL = GetLuaState();
		lua_gc(pL, LUA_GCCOLLECT, 0);
		lua_gc(pL, LUA_GCCOLLECT, 0);
	}

	bool CScriptLua::IsValid( void* pObject )
	{
		lua_State* pL = GetLuaState();
		int32 nTop = lua_gettop( pL );
		PushCppObjWeakTable( pL );
		bool bIsValid = PushPointerToLua( pL, pObject, nTop + 1, false );
		lua_pop( pL, 3 );
		return bIsValid;
	}

	SFunctionTable* CScriptLua::FindHookVirtualTable( 
		const CClass* pClass, SFunctionTable* pOldFunTable )
	{
		auto& mapOld2New = m_mapVirtualTableOld2NewOfClass[pClass];
		auto it = mapOld2New.find( pOldFunTable );
		if( it != mapOld2New.end() )
			return it->second;
		return nullptr;
	}

	void CScriptLua::SetHookVirtualTable( const CClass* pClass, 
		SFunctionTable* pOldFunTable, SFunctionTable* pNewFunTable )
	{
		auto& mapOld2New = m_mapVirtualTableOld2NewOfClass[pClass];
		assert( mapOld2New.find( pOldFunTable ) == mapOld2New.end() );
		mapOld2New[pOldFunTable] = pNewFunTable;
	}
};
