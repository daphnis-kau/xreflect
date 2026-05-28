/** @file 04_enum/main.cpp
 * @brief Enum type registration and value introspection.
 */
#include "Reflection.h"
#include <cstdio>

enum class Color { Red, Green, Blue };
enum class Flag { None = 0, Read = 1, Write = 2, Execute = 4 };

DEFINE_ENUM_BEGIN( Color )
    REGIST_ENUMVALUE( Red )
    REGIST_ENUMVALUE( Green )
    REGIST_ENUMVALUE_WITH_NAME( Blue, Azure )
DEFINE_ENUM_END()

DEFINE_ENUM_BEGIN( Flag )
    REGIST_ENUMVALUE( None )
    REGIST_ENUMVALUE( Read )
    REGIST_ENUMVALUE( Write )
    REGIST_ENUMVALUE_WITH_NAME( Execute, CanExecute )
DEFINE_ENUM_END()

int main()
{
    using namespace XReflect;
    auto& R = CReflection::GetInstance();
    printf( "=== Enum Reflection ===\n\n" );

    const CClass* pColor = R.FindClassByTypeID( TypeName<Color>() );
    printf( "Enum: %s\n", pColor->GetClassName().c_str() );
    printf( "IsEnum: %s\n", pColor->IsEnum() ? "yes" : "no" );

    printf( "Values:\n" );
    for( auto& pair : pColor->GetRegistFields() )
    {
        auto f = pair.second;
        if( f->GetType() != eFDT_Enumeration ) continue;
        printf( "  %s = %d\n", f->GetName().c_str(), f->GetEnumValue() );
    }

    const CClass* pFlag = R.FindClassByTypeID( TypeName<Flag>() );
    printf( "\nFlag enum: %s\n", pFlag->GetClassName().c_str() );
    printf( "Is mask-enable: %s\n", pFlag->IsEnumMaskEnable() ? "yes" : "no" );
    for( auto& pair : pFlag->GetRegistFields() )
    {
        auto f = pair.second;
        if( f->GetType() != eFDT_Enumeration ) continue;
        printf( "  %s = %d\n", f->GetName().c_str(), f->GetEnumValue() );
    }

    const CField* pAzure = pColor->FindField( "Azure" );
    printf( "\nColor::Blue -> Azure = %d (REGIST_ENUMVALUE_WITH_NAME)\n",
        pAzure ? pAzure->GetEnumValue() : -1 );
    const CField* pCE = pFlag->FindField( "CanExecute" );
    printf( "Flag::Execute -> CanExecute = %d (REGIST_ENUMVALUE_WITH_NAME)\n",
        pCE ? pCE->GetEnumValue() : -1 );

    return 0;
}
