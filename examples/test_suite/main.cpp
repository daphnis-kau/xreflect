// xreflect feature test suite
// Build: cmake --build . --target test_basic
// Run:   ./test_basic

#include "Reflection.h"
#include <cstdio>
#include <cmath>
#include <string>
#include "Template/std.h"
#include <vector>
#include <cstring>

// ─── Type definitions ───

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    void Normalize() { float inv = 1.0f/Length(); x*=inv; y*=inv; z*=inv; }
};
DEFINE_CLASS_BEGIN( Vec3 )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( x )
    REGIST_CLASSMEMBER( y )
    REGIST_CLASSMEMBER( z )
    REGIST_CLASSMETHOD( Length )
    REGIST_CLASSMETHOD( Normalize )
DEFINE_CLASS_END()

enum class Color { Red, Green, Blue };
DEFINE_ENUM_BEGIN( Color )
    REGIST_ENUMVALUE( Red )
    REGIST_ENUMVALUE( Green )
    REGIST_ENUMVALUE( Blue )
DEFINE_ENUM_END()

class Player {
public:
    std::string m_Name;
    int m_Health, m_MaxHealth, m_InternalId;
    Player() : m_Name("Unnamed"), m_Health(100), m_MaxHealth(100), m_InternalId(0) {}
    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& v) { m_Name = v; }
    int GetHealth() const { return m_Health; } void SetHealth(int v) { m_Health = v; }
    float HealthRatio() const { return (float)m_Health / m_MaxHealth; }
    void TakeDamage(int dmg) { m_Health = (m_Health > dmg) ? m_Health - dmg : 0; }
};
DEFINE_CLASS_BEGIN( Player )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSPROPERTY( Name, GetName, SetName )
    REGIST_CLASSPROPERTY( Health, GetHealth, SetHealth )
    REGIST_READONLY_CLASSPROPERTY( HealthRatio, HealthRatio )
    REGIST_EDIT_DISABLE_CLASSMEMBER( m_InternalId )
    REGIST_SERIALIZE_DISABLE_CLASSMEMBER( m_MaxHealth )
    REGIST_CLASSMETHOD( TakeDamage )
DEFINE_CLASS_END()

struct Entity { int m_Id; Entity() : m_Id(0) {} };
DEFINE_CLASS_BEGIN( Entity )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_Id )
DEFINE_CLASS_END()

struct Enemy : Entity {
    int m_Damage;
    Enemy() : m_Damage(10) {}
};
DEFINE_CLASS_BEGIN( Enemy )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_BASE_CLASS( Entity )
    REGIST_CLASSMEMBER( m_Damage )
DEFINE_CLASS_END()

// ═══════════════════════════════════════════
// -- Field path / Modify types --

struct Pos { float m_X, m_Y; Pos() : m_X(0), m_Y(0) {} };
DEFINE_CLASS_BEGIN( Pos )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_X )
    REGIST_CLASSMEMBER( m_Y )
DEFINE_CLASS_END()

struct Unit { Pos m_Pos; int m_HP; Unit() : m_HP(100) { m_Pos.m_X = 1; m_Pos.m_Y = 2; } };
DEFINE_CLASS_BEGIN( Unit )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_Pos )
    REGIST_CLASSMEMBER( m_HP )
DEFINE_CLASS_END()

// --- Container types ---
struct Item { int m_ID; Item() : m_ID(0) {} };
DEFINE_CLASS_BEGIN( Item )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_ID )
DEFINE_CLASS_END()

struct Inv { std::vector<Item> m_Items; Inv() { m_Items.push_back(Item()); } };
DEFINE_CLASS_BEGIN( Inv )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_Items )
DEFINE_CLASS_END()

// Test runner
// ═══════════════════════════════════════════

static int g_Passed = 0, g_Failed = 0;
#define CHECK(expr) do { \
    if (expr) { g_Passed++; } \
    else { fprintf(stderr, "FAIL [%d]: %s\n", __LINE__, #expr); g_Failed++; } \
} while(0)
#define CHECKF(expr, fmt, ...) do { \
    if (expr) { g_Passed++; } \
    else { fprintf(stderr, "FAIL [%d]: " fmt "\n", __LINE__, __VA_ARGS__); g_Failed++; } \
} while(0)

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    using namespace XReflect;
    auto& R = CReflection::GetInstance();

    printf("=== xreflect test suite ===\n\n");

    // ═══ 1. Class registration ═══
    {
        printf("  Test 1: Class registration... ");
        const CClass* pV = R.FindClassByTypeID(TypeName<Vec3>());
        CHECK(pV != nullptr);
        CHECK(strcmp(pV->GetClassName().c_str(), "Vec3") == 0);
        CHECK(pV->GetClassSize() == sizeof(Vec3));
        CHECK(pV->IsDefaultConstructible());
        CHECK(pV->IsCopyConstructible());
        printf("OK\n");
    }

    // ═══ 2. Member field get/set ═══
    {
        printf("  Test 2: Member field access... ");
        const CClass* pV = R.FindClassByTypeID(TypeName<Vec3>());
        const CField* pX = pV->FindField("x");
        const CField* pY = pV->FindField("y");
        const CField* pZ = pV->FindField("z");
        CHECK(pX != nullptr && pY != nullptr && pZ != nullptr);
        CHECK(pX->GetType() == eFDT_Member);
        Vec3 v(1, 2, 3);
        float fx = pX->Get<Vec3, float>(&v);
        CHECK(fx == 1.0f);
        pY->Set<Vec3, float>(&v, 42.0f);
        CHECK(v.y == 42.0f);
        printf("OK\n");
    }

    // ═══ 3. Method invocation via Call ═══
    {
        printf("  Test 3: Method invocation... ");
        const CClass* pV = R.FindClassByTypeID(TypeName<Vec3>());
        const CField* pLen = pV->FindField("Length");
        CHECK(pLen != nullptr);
        CHECK(pLen->GetType() == eFDT_ClassFunction);
        Vec3 v(3, 4, 0);
        float len = 0;
        void* pThis = &v;
        void* aLen[] = { &pThis };
        pLen->Call(&len, aLen);
        CHECKF(fabs(len - 5.0f) < 0.001f, "len=%.3f", len);

        // void method
        const CField* pNorm = pV->FindField("Normalize");
        Vec3 v2(3, 4, 0);
        void* pThis2 = &v2;
        void* aNorm[] = { &pThis2 };
        pNorm->Call(nullptr, aNorm);
        CHECKF(fabs(v2.Length() - 1.0f) < 0.001f, "normLen=%.3f", v2.Length());
        printf("OK\n");
    }

    // ═══ 4. Object construction/destruction ═══
    {
        printf("  Test 4: Construction/destruction... ");
        const CClass* pV = R.FindClassByTypeID(TypeName<Vec3>());
        void* pObj = malloc(pV->GetClassSize());
        pV->Construct(eBCI_DefaultConstructor, pObj, nullptr);
        CHECK(((Vec3*)pObj)->x == 0);
        pV->Destruct(pObj);
        free(pObj);
        printf("OK\n");
    }

    // ═══ 5. Enum registration ═══
    {
        printf("  Test 5: Enum registration... ");
        const CClass* pC = R.FindClassByTypeID(TypeName<Color>());
        CHECK(pC != nullptr);
        CHECK(pC->IsEnum());
        const CField* pR = pC->FindField("Red");
        const CField* pB = pC->FindField("Blue");
        CHECK(pR != nullptr && pB != nullptr);
        CHECK(pR->GetEnumValue() == 0);
        CHECK(pB->GetEnumValue() == 2);
        printf("OK\n");
    }

    // ═══ 6. Property get/set ═══
    {
        printf("  Test 6: Property access... ");
        const CClass* pP = R.FindClassByTypeID(TypeName<Player>());
        const CField* pName = pP->FindField("Name");
        Player player;
        std::string name = pName->Get<Player, std::string>(&player);
        CHECK(name == "Unnamed");
        std::string nn = "Hero";
        pName->Set<Player, std::string>(&player, nn);
        CHECK(player.GetName() == "Hero");
        printf("OK\n");
    }

    // ═══ 7. Readonly property ═══
    {
        printf("  Test 7: Readonly property... ");
        const CClass* pP = R.FindClassByTypeID(TypeName<Player>());
        const CField* pR = pP->FindField("HealthRatio");
        Player player;
        float r = pR->Get<Player, float>(&player);
        CHECKF(r == 1.0f, "ratio=%.3f", r);
        printf("OK\n");
    }

    // ═══ 8. Qualifier flags ═══
    {
        printf("  Test 8: Qualifier flags... ");
        const CClass* pP = R.FindClassByTypeID(TypeName<Player>());
        CHECK(pP->FindField("m_InternalId")->GetMemberQualifier() & eMBQ_DisableEdit);
        CHECK(pP->FindField("m_MaxHealth")->GetMemberQualifier() & eMBQ_DisableSerialize);
        printf("OK\n");
    }

    // ═══ 9. Field iteration ═══
    {
        printf("  Test 9: Field iteration... ");
        const CClass* pV = R.FindClassByTypeID(TypeName<Vec3>());
        int n = 0;
        for( auto& it : pV->GetRegistFields() )
            n++;
        CHECK(n >= 5);
        printf("OK (%d fields)\n", n);
    }

    // ═══ 10. Inheritance ═══
    {
        printf("  Test 10: Inheritance... ");
        const CClass* pE = R.FindClassByTypeID(TypeName<Enemy>());
        CHECK(pE != nullptr);
        CHECK(pE->GetBaseClassCount() == 1);
        CHECK(pE->IsA(R.FindClassByTypeID(TypeName<Entity>())));
        CHECK(pE->FindField("m_Id", true) != nullptr);
        printf("OK\n");
    }

    // ═══ 11. UPakDataType ═══
    {
        printf("  Test 11: UPakDataType... ");
        CHECK(UPakDataType(eDT_int32).IsBaseType());
        CHECK(UPakDataType(eDT_void, 1, false, false).GetMulPointer() == 1);
        CHECK(UPakDataType(R.FindClassByTypeID(TypeName<Vec3>())).IsClass());
        printf("OK\n");
    }

    // --- 12. Container introspection ---
    {
        printf("  Test 12: Container introspection... ");
        const CClass* pInv = R.FindClassByTypeID(TypeName<Inv>());
        const CField* pF = pInv->FindField("m_Items");
        const CClass* pVec = pF->GetResultType().GetClass();
        UPakDataType elem = pVec->GetContainerElemType();
        CHECK(!elem.IsVoid() && strcmp(elem.GetClass()->GetClassName().c_str(), "Item") == 0);
        Inv inv;
        CHECK(pVec->GetContainerSize(&inv.m_Items) == 1);
        printf("OK\n");
    }
    
    // --- 13. Field path Fetch ---
    {
        printf("  Test 13: Field path Fetch... ");
        const CClass* pU = R.FindClassByTypeID(TypeName<Unit>());
        const CField* pPos = pU->FindField("m_Pos");
        const CField* pX = R.FindClassByTypeID(TypeName<Pos>())->FindField("m_X");
        SFieldNode a[] = { SFieldNode(pPos), SFieldNode(pX) };
        SFieldPath pt = { a, 2 };
        Unit u;
        float fx = pU->Fetch<float>(&u, pt);
        CHECKF(fx == 1.0f, "fx=%.1f", fx);
        const CField* pHP = pU->FindField("m_HP");
        SFieldNode a2[] = { SFieldNode(pHP) };
        SFieldPath pt2 = { a2, 1 };
        CHECK(pU->Fetch<int>(&u, pt2) == 100);
        puts("OK");
    }

    // --- 14. Modify via field path ---
    {
        printf("  Test 14: Modify... ");
        const CClass* pU = R.FindClassByTypeID(TypeName<Unit>());
        SFieldNode a1[] = { SFieldNode(pU->FindField("m_HP")) };
        SFieldPath p1 = { a1, 1 };
        Unit u; int hp = 75;
        CHECK(pU->Modify(&u, &hp, false, p1, eFOT_Modify) && u.m_HP == 75);
        const CField* pX = R.FindClassByTypeID(TypeName<Pos>())->FindField("m_X");
        SFieldNode a2[] = { SFieldNode(pU->FindField("m_Pos")), SFieldNode(pX) };
        SFieldPath p2 = { a2, 2 };
        float nx = 9.0f;
        CHECK(pU->Modify(&u, &nx, false, p2, eFOT_Modify) && u.m_Pos.m_X == 9.0f);
        puts("OK");
    }

    printf("\n=== Results: %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed ? 1 : 0;
}
