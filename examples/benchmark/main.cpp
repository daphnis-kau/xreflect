// xreflect performance benchmark
// Compares xreflect and RTTR against native C++ baseline.
#include "Reflection.h"
#include <rttr/registration>
#include <cstdio>
#include <chrono>
#include <cmath>
#include <cstring>

// -- Shared test type --
struct Vec3 {
    float x = 0, y = 0, z = 0;
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    void Normalize() { float inv = 1.0f/Length(); x*=inv; y*=inv; z*=inv; }
};

DEFINE_CLASS_BEGIN( Vec3 )
    REGIST_CLASSMEMBER( x )
    REGIST_CLASSMEMBER( y )
    REGIST_CLASSMEMBER( z )
    REGIST_CLASSMETHOD( Length )
    REGIST_CLASSMETHOD( Normalize )
DEFINE_CLASS_END()

RTTR_REGISTRATION
{
    rttr::registration::class_<Vec3>("Vec3")
        .constructor<>()
        .property("x", &Vec3::x)
        .property("y", &Vec3::y)
        .property("z", &Vec3::z)
        .method("Length", &Vec3::Length)
        .method("Normalize", &Vec3::Normalize);
}

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

static double ToNs( ns d ) { return (double)d.count(); }

template<typename F>
static double Bench( const char* label, int iters, F&& fn, int warmup = 0 )
{
    if( warmup ) for( int i = 0; i < warmup; ++i ) fn();
    auto t0 = Clock::now();
    for( int i = 0; i < iters; ++i ) fn();
    auto t1 = Clock::now();
    double total = ToNs( std::chrono::duration_cast<ns>( t1 - t0 ) );
    double perOp = total / iters;
    printf( "  %-55s %8.2f ns/op  (total %8.2f ms)\n", label, perOp, total / 1e6 );
    return perOp;
}

int main()
{
    const int N = 10'000'000;
    using namespace XReflect;
    auto& R = CReflection::GetInstance();
    Vec3 v;

    printf( "=== Reflection Performance Benchmark ===\n" );
#ifdef _MSC_VER
    printf( "Iterations: %d  |  Compiler: MSVC %d  |  Build: Release\n\n", N, _MSC_VER );
#elif defined(__GNUC__)
    printf( "Iterations: %d  |  Compiler: GCC %d  |  Build: Release\n\n", N, __GNUC__ );
#elif defined(__clang__)
    printf( "Iterations: %d  |  Compiler: Clang %d  |  Build: Release\n\n", N, __clang_major__ );
#else
    printf( "Iterations: %d  |  Compiler: unknown  |  Build: Release\n\n", N );
#endif

    // -- Field read --
    printf( "-- Field read (x) --\n" );
    v.x = 7.0f;
    double nativeRead = Bench( "native       v.x", N, [&] { volatile float a = v.x; (void)a; } );

    const CField* pX = R.FindClassByTypeID(TypeName<Vec3>())->FindField("x");
    double xrRead = Bench( "xreflect     Get<Vec3,float>( x )", N,
        [&] { float a = pX->Get<Vec3, float>(&v); (void)a; } );

    auto rType  = rttr::type::get<Vec3>();
    auto rPropX = rType.get_property("x");
    double rttrRead = Bench( "RTTR         property::get_value(x)", N,
        [&] { float a = rPropX.get_value(v).to_float(); (void)a; } );

    // -- Field write --
    printf( "\n-- Field write (y) --\n" );
    double nativeWrite = Bench( "native       v.y = val", N, [&] { v.y = 3.0f; } );

    const CField* pY = R.FindClassByTypeID(TypeName<Vec3>())->FindField("y");
    double xrWrite = Bench( "xreflect     Set<Vec3,float>( y )", N,
        [&] { pY->Set<Vec3, float>(&v, 3.0f); } );

    auto rPropY = rType.get_property("y");
    double rttrWrite = Bench( "RTTR         property::set_value(y)", N,
        [&] { rPropY.set_value(v, 3.0f); } );

    // -- Method call --
    printf( "\n-- Method call (Length) --\n" );
    v.x = 3; v.y = 4; v.z = 0;
    double nativeMethod = Bench( "native       v.Length()", N,
        [&] { volatile float a = v.Length(); (void)a; } );

    const CField* pLen = R.FindClassByTypeID(TypeName<Vec3>())->FindField("Length");
    double xrMethod = Bench( "xreflect     FindField->Call(Length)", N, [&] {
        float r = 0; void* pT = &v; void* a[] = { &pT };
        pLen->Call(&r, a); (void)r;
    } );

    auto rMethodLen = rType.get_method("Length");
    double rttrMethod = Bench( "RTTR         method::invoke(Length)", N, [&] {
        float a = rMethodLen.invoke(v).to_float(); (void)a;
    } );

    // -- Void method call --
    printf( "\n-- Void method call (Normalize) --\n" );
    double nativeVoid = Bench( "native       v.Normalize()", N, [&] {
        Vec3 v2(3,4,0); v2.Normalize();
    } );

    const CField* pNorm = R.FindClassByTypeID(TypeName<Vec3>())->FindField("Normalize");
    double xrVoid = Bench( "xreflect     FindField->Call(Normalize)", N, [&] {
        Vec3 v2(3,4,0); void* pT = &v2; void* a[] = { &pT };
        pNorm->Call(nullptr, a);
    } );

    auto rMethodNorm = rType.get_method("Normalize");
    double rttrVoid = Bench( "RTTR         method::invoke(Normalize)", N, [&] {
        Vec3 v2(3,4,0); rMethodNorm.invoke(v2);
    } );

    // -- Lookup + read (worst case) --
    printf( "\n-- Lookup + read (worst case) --\n" );
    const int M = N / 10;
    CName nameZ("z");
    double xrLookup = Bench( "xreflect     FindField+Get (no cache)", M, [&] {
        auto f = R.FindClassByTypeID(TypeName<Vec3>())->FindField(nameZ);
        float a = f->Get<Vec3, float>(&v); (void)a;
    } );

    double rttrLookup = Bench( "RTTR         type::get_by_name+get_value", M, [&] {
        auto t = rttr::type::get_by_name("Vec3");
        auto p = t.get_property("z");
        float a = p.get_value(v).to_float(); (void)a;
    } );

    // -- Summary --
    printf( "\n=== Summary (overhead vs native) ===\n" );
    printf( "  Field read:   native %.2f ns | xreflect %.2f ns (%.1fx) | RTTR %.2f ns (%.1fx)\n",
            nativeRead, xrRead, xrRead/nativeRead, rttrRead, rttrRead/nativeRead );
    printf( "  Field write:  native %.2f ns | xreflect %.2f ns (%.1fx) | RTTR %.2f ns (%.1fx)\n",
            nativeWrite, xrWrite, xrWrite/nativeWrite, rttrWrite, rttrWrite/nativeWrite );
    printf( "  Method call:  native %.2f ns | xreflect %.2f ns (%.1fx) | RTTR %.2f ns (%.1fx)\n",
            nativeMethod, xrMethod, xrMethod/nativeMethod, rttrMethod, rttrMethod/nativeMethod );
    printf( "  Void call:    native %.2f ns | xreflect %.2f ns (%.1fx) | RTTR %.2f ns (%.1fx)\n",
            nativeVoid, xrVoid, xrVoid/nativeVoid, rttrVoid, rttrVoid/nativeVoid );
    printf( "  Lookup+read:  xreflect %.2f ns | RTTR %.2f ns (%.1fx slower)\n",
            xrLookup, rttrLookup, rttrLookup/xrLookup );

    printf( "\n=== Done ===\n" );
    return 0;
}
