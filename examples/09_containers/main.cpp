#include "Reflection.h"
#include "Template/std.h"
#include <cstdio>
#include <vector>
#include <set>

// ============================================================================
// Part 1: Custom container via REGIST_ARRAY.
//         REGIST_ARRAY registers the class as a random-access container.
//         xreflect can then query element type and container size.
// ============================================================================

struct FloatStack
{
	static const int MAX = 8;
	float m_Buf[ MAX ];
	int   m_Count;

	FloatStack() : m_Count( 3 )
	{
		m_Buf[0] = 10.f; m_Buf[1] = 20.f; m_Buf[2] = 30.f;
	}
};

DEFINE_CLASS_BEGIN( FloatStack )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_ARRAY_LAMBDA( float,
		[this]() { return m_Count; },
		[this]( size_t i ) -> float& { return m_Buf[i]; },
		[this]( size_t i, const float& v ) { m_Buf[i] = v; } )
DEFINE_CLASS_END()

// Non-lambda REGIST_ARRAY: uses operator[] and a named size method
struct IntStack
{
	int m_Buf[ 8 ];
	int m_Count;
	IntStack() : m_Count( 2 ) { m_Buf[0] = 100; m_Buf[1] = 200; }
	int& operator[]( size_t i )       { return m_Buf[i]; }
	int  SizeImp() const               { return m_Count; }
};
DEFINE_CLASS_BEGIN( IntStack )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_ARRAY( int, SizeImp )
DEFINE_CLASS_END()

// ============================================================================
// Part 2: Custom container via REGIST_ITERATOR.
//         REGIST_ITERATOR registers a class as an iterable container.
//         The iterator must provide: default/copy/move ctor, ++, --, ==, !=,
//         =, and operator* returning the element type.
//         Then register begin()/end() with REGIST_CLASSMETHOD_OVERLOAD.
// ============================================================================

struct TokenList
{
	struct Iter
	{
		const char** m_p;
		Iter() : m_p( nullptr ) {}
		Iter( const char** p ) : m_p( p ) {}
		Iter( const Iter& r ) : m_p( r.m_p ) {}
		Iter( Iter&& r ) : m_p( r.m_p ) { r.m_p = nullptr; }
		Iter& operator=( const Iter& r ) { m_p = r.m_p; return *this; }

		const char*& operator*() { return *m_p; }
		Iter& operator++()       { ++m_p; return *this; }
		Iter& operator--()       { --m_p; return *this; }
		bool  operator==( const Iter& r ) const { return m_p == r.m_p; }
		bool  operator!=( const Iter& r ) const { return m_p != r.m_p; }
	};

	const char* m_Tokens[ 8 ];
	int         m_Count;

	TokenList() : m_Count( 2 )
	{
		m_Tokens[0] = "hello";
		m_Tokens[1] = "world";
	}

	int   Size() const              { return m_Count; }
	Iter  begin()                   { return Iter( (const char**)m_Tokens ); }
	Iter  end()                     { return Iter( (const char**)(m_Tokens + m_Count) ); }
};

DEFINE_CLASS_BEGIN( TokenList )
	REGIST_DEFAULT_CONSTRUCTOR()
	// Array interface: element type + indexed access
	REGIST_ARRAY_LAMBDA( const char*,
		[this]() { return m_Count; },
		[this]( size_t i ) -> const char* { return m_Tokens[i]; },
		[this]( size_t i, const char* v ) { m_Tokens[i] = v; } )
	// Iterator interface: the iterator type + begin/end
	REGIST_ITERATOR( Iter )
	REGIST_CLASSMETHOD( begin )
	REGIST_CLASSMETHOD( end )
DEFINE_CLASS_END()

// ============================================================================
// Part 3: STL containers (pre-registered in Template/std.h).
//         std::vector<T> is an array container, std::set<T> is an iterator
//         container. Both expose GetContainerElemType() and GetContainerSize().
// ============================================================================

struct Item
{
	int   m_ID;
	float m_Price;
	Item() : m_ID( 0 ), m_Price( 0 ) {}
	Item( int id, float p ) : m_ID( id ), m_Price( p ) {}
};

DEFINE_CLASS_BEGIN( Item )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_ID )
	REGIST_CLASSMEMBER( m_Price )
DEFINE_CLASS_END()

struct Stock
{
	std::vector<Item> m_Items;
	std::set<int>     m_IDs;
	Stock()
	{
		m_Items.push_back( Item( 1, 9.9f ) );
		m_Items.push_back( Item( 2, 19.9f ) );
		m_Items.push_back( Item( 3, 29.9f ) );
		m_IDs.insert( 100 );
		m_IDs.insert( 200 );
		m_IDs.insert( 300 );
	}
};

DEFINE_CLASS_BEGIN( Stock )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_Items )
	REGIST_CLASSMEMBER( m_IDs )
DEFINE_CLASS_END()

// ============================================================================

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();

	// --- Custom array container ---
	printf( "=== REGIST_ARRAY (FloatStack) ===\n" );
	const CClass* pFS = R.FindClassByTypeID( TypeName<FloatStack>() );
	FloatStack s;
	UPakDataType fsElem = pFS->GetContainerElemType();
	printf( "  container<%s>  size=%lld  (expected float, 3)\n",
		GetTypeReadableName( STypeInfo{ fsElem.GetDataType() } ),
		(long long)pFS->GetContainerSize( &s ) );

	// --- Custom iterator container ---
	printf( "\n=== REGIST_ITERATOR (TokenList) ===\n" );
	const CClass* pTS = R.FindClassByTypeID( TypeName<TokenList>() );
	TokenList ts;
	UPakDataType tsElem = pTS->GetContainerElemType();
	printf( "  container<%s>  size=%lld  (expected const char*, 2)\n",
		GetTypeReadableName( STypeInfo{ tsElem.GetDataType() } ),
		(long long)pTS->GetContainerSize( &ts ) );

	// --- STL: vector<T> (array container) ---
	printf( "\n=== std::vector<Item> (STL, pre-registered) ===\n" );
	const CClass* pStk = R.FindClassByTypeID( TypeName<Stock>() );
	Stock stock;
	const CField* pFv = pStk->FindField( "m_Items" );
	const CClass* pVec = pFv->GetResultType().GetClass();
	UPakDataType ve = pVec->GetContainerElemType();
	printf( "  container<%s>  size=%lld  (expected Item, 3)\n",
		ve.GetClass()->GetClassName().c_str(),
		(long long)pVec->GetContainerSize( &stock.m_Items ) );

	// Field path into vector element
	const CField* pF  = pStk->FindField( "m_Items" );
	SFieldNode   ary[] = { SFieldNode( pF ), SFieldNode( (uint64)1 ) };
	SFieldPath   path  = { ary, 2 };
	Item item = pStk->Fetch<Item>( &stock, path );
	printf( "  Fetch[1] = Item{ id=%d, price=%.1f }  (expected id=2)\n",
		item.m_ID, item.m_Price );

	// --- STL: set<int> (iterator container) ---
	printf( "\n=== std::set<int> (STL, pre-registered) ===\n" );
	const CField* pFs = pStk->FindField( "m_IDs" );
	const CClass* pSet = pFs->GetResultType().GetClass();
	UPakDataType se = pSet->GetContainerElemType();
	printf( "  container<%s>  size=%lld  (expected int, 3)\n",
		GetTypeReadableName( STypeInfo{ se.GetDataType() } ),
		(long long)pSet->GetContainerSize( &stock.m_IDs ) );

	// std::set supports iteration via REGIST_ITERATOR in Template/std.h
	// so you can iterate with begin/end, find, erase etc. at runtime
	printf( "  begin/end/find/erase available via REGIST_ITERATOR\n" );

	return 0;
}
