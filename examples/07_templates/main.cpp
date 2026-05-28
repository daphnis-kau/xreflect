/** @file 07_templates/main.cpp
 * @brief Template class reflection — custom template + STL templates.
 *
 * Template classes registered via DEFINE_TEMPLATE_BEGIN_WITH_N_PARAMS
 * are pulled into the reflection system when:
 *   a) used as a data member of a registered non-template class
 *   b) used as a function parameter/return value of a registered class
 *   c) explicitly via TRegisterTypes<T>()
 *
 * Additionally, you can look up any template instance directly with
 * FindClassByTypeID<T>(), which automatically triggers TRegisterTypes<T>()
 * internally — no manual registration needed.
 */
#include "Reflection.h"
#include <cstdio>
#include <string>
#include <vector>

// ═══ Custom template class ═══

template<typename T>
struct MyBox
{
	T m_Value;
	MyBox() : m_Value{} {}
	T Unbox() const { return m_Value; }
};

DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM( MyBox, typename )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_Value )
	REGIST_CLASSMETHOD( Unbox )
DEFINE_TEMPLATE_END()

// ═══ Non-template class with template-typed members ═══
// MyBox<float> is pulled via DataStore::m_Box member registration

struct DataStore
{
	MyBox<float>          m_Box;       // template member (pulls MyBox<float>)
	std::string           m_Name;      // STL template member
	std::vector<float>    m_Numbers;   // STL template member

	DataStore() {}

	const std::vector<float>& GetNumbers() const { return m_Numbers; }
	void SetNumbers( const std::vector<float>& v ) { m_Numbers = v; }
	float Sum() const
	{
		float s = 0;
		for( float x : m_Numbers )
			s += x;
		return s;
	}
};

DEFINE_CLASS_BEGIN( DataStore )
	REGIST_DEFAULT_CONSTRUCTOR()
	REGIST_CLASSMEMBER( m_Box )
	REGIST_CLASSMEMBER( m_Name )
	REGIST_CLASSMEMBER( m_Numbers )
	REGIST_CLASSMETHOD( GetNumbers )
	REGIST_CLASSMETHOD( SetNumbers )
	REGIST_CLASSMETHOD( Sum )
DEFINE_CLASS_END()

int main()
{
	using namespace XReflect;
	auto& R = CReflection::GetInstance();

	printf( "=== Template Class Reflection ===\n\n" );

	// -- 1. Custom template via member: MyBox<float> --
	{
		const CClass* pCls = R.FindClassByTypeID( TypeName<MyBox<float>>() );
		printf( "-- MyBox<float> (via member) --\n" );
		printf( "  Class name : %s\n", pCls->GetClassName().c_str() );
		printf( "  Template   : %s\n", pCls->GetTemplateName().c_str() );
		printf( "  Size       : %u (expected %zu)\n", pCls->GetClassSize(), sizeof(MyBox<float>) );

		MyBox<float> box;
		box.m_Value = 3.14f;
		float v = pCls->FindField( "m_Value" )->Get<MyBox<float>, float>( &box );
		printf( "  m_Value    = %.2f\n\n", v );
	}

	// -- 2. Custom template via TRegisterTypes: MyBox<int> --
	{
		// Pull MyBox<int> via explicit TRegisterTypes
		XReflect::TRegisterTypes<MyBox<int>>();

		const CClass* pCls = R.FindClassByTypeID( TypeName<MyBox<int>>() );
		printf( "-- MyBox<int> (via TRegisterTypes) --\n" );
		printf( "  Class name : %s\n", pCls->GetClassName().c_str() );
		printf( "  Template   : %s\n", pCls->GetTemplateName().c_str() );

		MyBox<int> box;
		box.m_Value = 42;
		int v = pCls->FindField( "m_Value" )->Get<MyBox<int>, int>( &box );
		printf( "  m_Value    = %d\n\n", v );
	}

	// -- 3. Custom template via FindClassByTypeID<T>: MyBox<double> --
	// FindClassByTypeID<T>() automatically triggers TRegisterTypes<T>()
	// internally — no manual registration needed.
	{
		const CClass* pCls = R.FindClassByTypeID<MyBox<double>>();
		printf( "-- MyBox<double> (via FindClassByTypeID<T>) --\n" );
		printf( "  Class name : %s\n", pCls->GetClassName().c_str() );
		printf( "  Template   : %s\n", pCls->GetTemplateName().c_str() );
		printf( "  Size       : %u (expected %zu)\n", pCls->GetClassSize(), sizeof(MyBox<double>) );

		MyBox<double> box;
		box.m_Value = 2.718;
		double v = pCls->FindField( "m_Value" )->Get<MyBox<double>, double>( &box );
		printf( "  m_Value    = %.3f\n\n", v );
	}

	// -- 4. Template identity check --
	{
		const CClass* pF = R.FindClassByTypeID( TypeName<MyBox<float>>() );
		const CClass* pI = R.FindClassByTypeID( TypeName<MyBox<int>>() );
		printf( "-- Template identity --\n" );
		printf( "  Same base? %s\n\n",
			pF->GetTemplateName() == pI->GetTemplateName() ? "yes" : "no" );
	}

	// -- 5. STL template members + method call --
	{
		const CClass* pCls = R.FindClassByTypeID( TypeName<DataStore>() );
		printf( "-- DataStore --\n" );
		printf( "  Class : %s\n", pCls->GetClassName().c_str() );
		printf( "  Fields: " );

		for( auto& fieldPair : pCls->GetRegistFields() )
		{
			const CField* pF = fieldPair.second;
			printf( "%s ", pF->GetName().c_str() );

			UPakDataType t = pF->GetResultType();
			if( t.IsClass() && pF->GetType() == eFDT_Member )
			{
				const CClass* pEC = t.GetClass();
				printf( "[%s", pEC->GetClassName().c_str() );
				if( !pEC->GetContainerElemType().IsVoid() )
				{
					const CClass* pET = pEC->GetContainerElemType().GetClass();
					printf( "<%s>", pET ? pET->GetClassName().c_str() : "?" );
				}
				printf( "] " );
			}
		}
		printf( "\n" );

		DataStore ds;
		ds.m_Numbers = { 1.0f, 2.0f, 3.0f };
		float  result = 0;
		void*  pThis  = &ds;
		void*  aSum[] = { &pThis };
		pCls->FindField( "Sum" )->Call( &result, aSum );
		printf( "  Sum({1,2,3}) = %.0f\n", result );
	}

	return 0;
}
