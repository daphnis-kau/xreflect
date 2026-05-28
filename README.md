﻿# xreflect — C++20 Runtime Reflection Library

Zero-dependency C++20 runtime reflection with virtual table hooking, STL container support, and field path traversal. ~15x faster field access than RTTR (2.4 ns vs 35-120 ns). No external code generators required.

## Why xreflect?

| Feature | Native | xreflect | RTTR |
|---------|--------|----------|------|
| Field read (hot path) | 0.3 ns | **1.7 ns** (~5 cycles) | 41.0 ns (~121 cycles) |
| Field write (hot path) | 0.3 ns | **1.5 ns** (~4 cycles) | 14.2 ns (~42 cycles) |
| Method call | 1.1 ns | **4.1 ns** (~12 cycles) | 43.2 ns (~127 cycles) |
| Void method call | — | **5.1 ns** | 18.3 ns |
| Lookup + read (worst case) | — | **78.3 ns** | 73.0 ns |
| Virtual table hooking | — | Yes — scripts inherit C++ classes | No |
| Template class support | — | **Pattern-based** — all instantiations auto-registered | **Instance-based** — each `<T>` registered manually |
| STL container introspection | — | GetContainerElemType + GetContainerSize | Not built-in |
| Field path traversal | — | `obj.a.b[3].c` in one call | Manual per-level |
| 64-bit type descriptor | — | Register-passed, zero alloc | Object-based |
| External code generator | — | Not needed | Not needed |

Performance measured on Intel i7-13700H, MSVC 2022 /O2, 10M iterations.
See [Appendix: Raw Benchmark Data](#appendix-raw-benchmark-data) for Run 1–3 details.
Reproduce: `cmake -B build_rel -DCMAKE_BUILD_TYPE=Release && cmake --build build_rel --config Release --target benchmark`

## Quick Start

```cpp
#include "Reflection.h"

struct Vec3 { float x, y, z; };
DEFINE_CLASS_BEGIN( Vec3 )
    REGIST_CLASSMEMBER( x )
    REGIST_CLASSMEMBER( y )
    REGIST_CLASSMEMBER( z )
DEFINE_CLASS_END()

int main() {
    using namespace XReflect;
    auto& R = CReflection::GetInstance();

    // Find class at runtime
    const CClass* pCls = R.FindClassByTypeID( TypeName<Vec3>() );

    // Read a field by name
    Vec3 v = { 1, 2, 3 };
    const CField* pF = pCls->FindField( "y" );
    float val = pF->Get<Vec3, float>( &v );    // 2.0

    // Write via reflection
    float newVal = 42;
    pF->Set<Vec3, float>( &v, newVal );         // v.y = 42
}
```

## Building

```
cmake -B build && cmake --build build
```

Requires: C++20 (MSVC 2022+, GCC 12+, Clang 16+), CMake 3.20+.

## Features

### Type Registration
```cpp
DEFINE_CLASS_BEGIN( MyClass )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_Position )                    // data member
    REGIST_CLASSMETHOD( DoSomething )                    // method
    REGIST_CLASSPROPERTY( Name, GetName, SetName )       // getter/setter
    REGIST_CLASSCALLBACK( OnEvent )                      // virtual function (hookable)
    REGIST_BASE_CLASS( BaseClass )                       // inheritance
    REGIST_EDIT_DISABLE_CLASSMEMBER( m_Internal )        // hide from editor
    REGIST_SERIALIZE_DISABLE_CLASSMEMBER( m_Cache )      // skip in serialization
DEFINE_CLASS_END()
```

### Enum Registration
```cpp
DEFINE_ENUM_BEGIN( Color )
    REGIST_ENUMVALUE( Red )
    REGIST_ENUMVALUE( Green )
    REGIST_ENUMVALUE( Blue )
DEFINE_ENUM_END()
```

### Virtual Table Hooking
```cpp
class MockScriptHook : public ICallbackHook {
    std::map<std::string, std::function<void(const CField*, void*, void**)>> m_Overrides;
public:
    void AddOverride(const char* name, auto fn) { m_Overrides[name] = fn; }
    bool OnCallBack(const CField* p, void* r, void** a) override {
        auto it = m_Overrides.find(p->GetName().c_str());
        if (it == m_Overrides.end()) return false;
        it->second(p, r, a);
        return true;
    }
    // ... boilerplate FindHook/SetHook/OnDestruct ...
};

IAnimal* p = new IAnimal();
MockScriptHook hook;
hook.AddOverride("GetAge", [](auto, void* r, auto){ *(int*)r = 42; });
pCls->HookVirtualTable(p, &hook);
p->GetAge();  // returns 42 — dispatched through the hook
```

### Field Path Traversal
```cpp
// Navigate obj.m_Pos.m_X in a single operation
SFieldNode path[] = { SFieldNode(pFieldPos), SFieldNode(pFieldX) };
SFieldPath fp = { path, 2 };

// Read
float x;
pCls->Fetch(&obj, fp, &x, [](void* d, void* c){ *(float*)c = *(float*)d; });

// Write
float newX = 9.0f;
pCls->Modify(&obj, &newX, false, fp, eFOT_Modify);
```

### Container Introspection
```cpp
const CField* pF = pCls->FindField("m_Values");
if (pF->GetResultType().IsClass()) {
    const CClass* pCont = pF->GetResultType().GetClass();
    UPakDataType elem = pCont->GetContainerElemType();   // e.g., float
    int64 n = pCont->GetContainerSize(&obj.m_Values);    // element count
}
```

### Template Registration
```cpp
// 1. Register a custom template class — any instantiation is automatically reflected
template<typename T>
struct MyBox { T m_Value; };
DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM( MyBox, typename )
    REGIST_CLASSMEMBER( m_Value )
DEFINE_TEMPLATE_END()

// MyBox<float>, MyBox<int>, etc. are all registered on first use
const CClass* pC = R.FindClassByTypeID( TypeName<MyBox<float>>() );

// 2. STL containers are pre-registered via Template/std.h
#include "Template/std.h"
// std::vector<Item>, std::set<int>, std::string, std::map<K,V>, etc.
// all expose GetContainerElemType() and GetContainerSize()
```

xreflect supports **pattern-based** template registration — define the template once,
and every instantiation is reflected automatically at first use.
Unlike instance-based libraries where each `<int>`, `<float>`, etc. must be registered
manually, xreflect registers the template pattern and resolves instantiations on demand.

`Template/std.h` pre-registers patterns for the entire STL: `std::vector`, `std::set`,
`std::map`, `std::string`, `std::pair`, `std::array`, `std::list`, `std::shared_ptr`,
and more — all with full container introspection.

## Examples

| # | Example | Demonstrates |
|---|---------|-------------|
| 01 | `quick_start` | Minimal struct registration + field read/write |
| 02 | `fields_and_methods` | Method invocation via `CField::Call()` |
| 03 | `properties` | Computed properties, read-only, qualifier flags |
| 04 | `enum` | Enum registration and value lookup |
| 05 | `inheritance` | Base class, `IsA()`, cross-hierarchy `FindField` |
| 06 | `virtual_override` | `ICallbackHook` — scripts override C++ virtuals |
| 07 | `templates` | `TRegisterTypes` to pull template types into reflection |
| 08 | `fieldpath` | `SFieldNode`/`SFieldPath` — chained Fetch |
| 09 | `containers` | `GetContainerElemType()` / `GetContainerSize()` |
| 10 | `construction` | `CClass::Construct()` / `New()` / `Delete()` |
| 11 | `modify` | `CClass::Modify()` — write via field path |
| 12 | `comprehensive` | Full macro surface demo — every registration macro in one place |
| 13 | `global_function` | `REGIST_GLOBALCMETHOD` — global function registration |
| 14 | `script_binding` | Lua & JavaScript runtime binding via xreflect |
| —  | `test_suite` | 38 test cases covering all features |
| —  | `benchmark` | xreflect vs native C++ performance benchmark |

```
cmake --build build --config Release
build/examples/test_suite/Release/test_suite.exe
```

### Example 14 — Script Binding

This example demonstrates automatic C++ → Lua / JavaScript binding via xreflect:

```bash
cmake --build build --config Release --target scriptbinding
build/examples/14_script_binding/Release/scriptbinding.exe
```

Input scripts:
- [`test.lua`](examples/14_script_binding/test.lua) — Lua bindings
- [`test.js`](examples/14_script_binding/test.js) — JavaScript bindings

## API Reference

### Core Types

| Type | Description |
|------|-------------|
| `CReflection` | Singleton type registry |
| `CClass` | Class metadata — size, fields, bases, constructors |
| `CField` | Field, method, property, or callback descriptor |
| `CName` | Interned string, O(1) comparison via integer index |
| `UPakDataType` | 64-bit compact type descriptor (pointer depth + const + ref + data) |
| `SFieldNode` | Single node in a field path (member / cast / index / custom fetcher) |
| `SFieldPath` | Ordered sequence of `SFieldNode` for compound field navigation |
| `CFieldTags` | Tag metadata for grouping and display |
| `IClassDefinition` | Object lifecycle — construct, destruct, assign, move |
| `ICallbackHook` | Virtual function interception — scripts inherit C++ classes |

### Registration Macros

**Class macros:**
`DEFINE_CLASS_BEGIN(T)` / `DEFINE_CLASS_END()`, `REGIST_DEFAULT_CONSTRUCTOR()`, `REGIST_CUSTOM_CONSTRUCTOR(...)`, `REGIST_DESTRUCTOR()`, `REGIST_BASE_CLASS(Base)`,

**Member macros:**
`REGIST_CLASSMEMBER(field)`, `REGIST_CLASSPROPERTY(name,get,set)`, `REGIST_READONLY_CLASSPROPERTY(name,get)`, `REGIST_STATICMEMBER(member)`, `REGIST_STATICPROPERTY(name,get,set)`,

**Method macros:**
`REGIST_CLASSMETHOD(method)`, `REGIST_STATICMETHOD(method)`, `REGIST_CLASSCALLBACK(virtual_fn)`, `REGIST_GLOBALCMETHOD(fn)`,

**Qualifier variants:**
`REGIST_EDIT_DISABLE_CLASSMEMBER`, `REGIST_SERIALIZE_DISABLE_CLASSMEMBER`, `_WITH_NAME`, `_WITH_TYPE`, `_LAMBDA`,

**Enum macros:**
`DEFINE_ENUM_BEGIN(E)` / `DEFINE_ENUM_END()`, `REGIST_ENUMVALUE(V)`,

**Template macros:**
`DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM(T, T0)` / `DEFINE_TEMPLATE_END()`, `REGIST_ARRAY(Elem, size_fn)`, `REGIST_ITERATOR(It)`, `REGIST_AS_STRONG_POINTER()`, `REGIST_AS_WEAK_POINTER(StrongType)`,

### UPakDataType — 64-bit Layout

```
Bit  0–1:  pointer depth  (0–3)
Bit  2:    is reference
Bit  3:    is const
Bit  4–15: reserved
Bit 16–63: CClass* pointer (class types) or EDataType ID (base types)
```

### SFieldNode — 64-bit Tagged Union

| Type | Payload | Use |
|------|---------|-----|
| `eFNT_NormalField` | `const CField*` | Member access |
| `eFNT_TypeCast` | `const CClass*` | Base class cast |
| `eFNT_ElementIndex` | `uint64` | Container element |
| `eFNT_CustomFetcher` | `IFieldNodeFetcher*` | Custom accessor |

## Architecture

xreflect solves four problems with one registration pass:

1. **Serialization** — `CClass::Modify/Fetch` + `SFieldPath` provide read/write for arbitrary nested fields
2. **Editor panels** — `EnumFields()` + qualifier flags auto-generate property widgets
3. **Undo/redo** — `SFieldPath` is a serializable operation key for command systems
4. **Script binding** — iterate all registered classes to auto-export to Lua, JS, etc.

## Requirements & Limitations

- **Compiler:** C++20 (MSVC 2022+, GCC 12+, Clang 16+)
- **Architecture:** 64-bit only (`static_assert(sizeof(void*) == 8)`)
- **Pointer depth:** max 3 levels of indirection
- **Virtual table size:** max 512 entries per class
- **String length:** max 8192 bytes (`XR_MAX_NAME_LENGTH`)
- **Virtual table hooking** depends on compiler ABI (MSVC/GCC/Clang on x86-64). Not guaranteed on ARM/AArch64 or with `-fvisibility=hidden`.
- **Thread safety:** class registration is thread-safe via `std::recursive_mutex`. Field access is lock-free.

## Third-Party Acknowledgements

This project embeds the following open-source libraries:

- **Lua 5.3** — Copyright (c) 1994-2026 Lua.org, PUC-Rio. [MIT License](examples/14_script_binding/third_party/lua/LICENSE)
- **QuickJS** — Copyright (c) 2017-2021 Fabrice Bellard, Charlie Gordon. [MIT License](examples/14_script_binding/third_party/quickjs/LICENSE)
- **RTTR v0.9.6** — Copyright (c) 2014-2018 Axel Menzel. [MIT License](examples/benchmark/third_party/rttr/LICENSE.txt)

## License

Licensed under the [MIT License](LICENSE).


---

# xreflect — C++20 运行时反射库（中文完整版）

零依赖的 C++20 运行时反射库。支持虚表 Hook、STL 容器内省、字段路径遍历。字段访问比 RTTR 快约 15 倍（2.4ns vs 35-120ns）。无需外部代码生成器。

## 为什么选择 xreflect？

| 特性 | 原生 | xreflect | RTTR |
|------|------|----------|------|
| 字段读取（热路径） | 0.3 ns | **1.7 ns** (~5 周期) | 41.0 ns (~121 周期) |
| 字段写入（热路径） | 0.3 ns | **1.5 ns** (~4 周期) | 14.2 ns (~42 周期) |
| 方法调用 | 1.1 ns | **4.1 ns** (~12 周期) | 43.2 ns (~127 周期) |
| 空方法调用 | — | **5.1 ns** | 18.3 ns |
| 查找+读取（最差情况） | — | **78.3 ns** | 73.0 ns |
| 虚表 Hook | — | 脚本可以继承 C++ 类 | 不支持 |
| 模板支持 | — | **模式注册** — 所有实例化自动反射 | **实例注册** — 每个 <T> 需手动注册 |
| STL 容器内省 | — | GetContainerElemType + GetContainerSize | 无内置支持 |
| 字段路径遍历 | — | 一次调用完成 obj.a.b[3].c | 需手动逐级展开 |
| 64 位类型描述符 | — | 寄存器传参，零分配 | 对象型 |
| 外部代码生成器 | — | 不需要 | 不需要 |

性能测试环境：Intel i7-13700H, MSVC 2022 /O2, 1000 万次迭代。
详见[附录：原始测试数据](#附录原始测试数据)。
复现命令：cmake -B build_rel -DCMAKE_BUILD_TYPE=Release && cmake --build build_rel --config Release --target benchmark

## 快速开始

```cpp
#include "Reflection.h"

struct Vec3 { float x, y, z; };
DEFINE_CLASS_BEGIN( Vec3 )
    REGIST_CLASSMEMBER( x )
    REGIST_CLASSMEMBER( y )
    REGIST_CLASSMEMBER( z )
DEFINE_CLASS_END()

int main() {
    using namespace XReflect;
    auto& R = CReflection::GetInstance();

    // 运行时查找类
    const CClass* pCls = R.FindClassByTypeID( TypeName<Vec3>() );

    // 按名称读取字段
    Vec3 v = { 1, 2, 3 };
    const CField* pF = pCls->FindField( "y" );
    float val = pF->Get<Vec3, float>( &v );    // 2.0

    // 通过反射写入
    float newVal = 42;
    pF->Set<Vec3, float>( &v, newVal );         // v.y = 42
}
```

## 编译

```
cmake -B build && cmake --build build
```

要求：C++20（MSVC 2022+, GCC 12+, Clang 16+），CMake 3.20+。

## 功能

### 类型注册
```cpp
DEFINE_CLASS_BEGIN( MyClass )
    REGIST_DEFAULT_CONSTRUCTOR()
    REGIST_CLASSMEMBER( m_Position )                    // 数据成员
    REGIST_CLASSMETHOD( DoSomething )                    // 方法
    REGIST_CLASSPROPERTY( Name, GetName, SetName )       // getter/setter 属性
    REGIST_CLASSCALLBACK( OnEvent )                      // 虚函数（可被脚本 Hook）
    REGIST_BASE_CLASS( BaseClass )                       // 继承
    REGIST_EDIT_DISABLE_CLASSMEMBER( m_Internal )        // 编辑器不可见
    REGIST_SERIALIZE_DISABLE_CLASSMEMBER( m_Cache )      // 序列化跳过
DEFINE_CLASS_END()
```

### 枚举注册
```cpp
DEFINE_ENUM_BEGIN( Color )
    REGIST_ENUMVALUE( Red )
    REGIST_ENUMVALUE( Green )
    REGIST_ENUMVALUE( Blue )
DEFINE_ENUM_END()
```

### 虚表 Hook
```cpp
class MockScriptHook : public ICallbackHook {
    std::map<std::string, std::function<void(const CField*, void*, void**)>> m_Overrides;
public:
    void AddOverride(const char* name, auto fn) { m_Overrides[name] = fn; }
    bool OnCallBack(const CField* p, void* r, void** a) override {
        auto it = m_Overrides.find(p->GetName().c_str());
        if (it == m_Overrides.end()) return false;
        it->second(p, r, a);
        return true;
    }
};

// IAnimal 定义见示例 06_virtual_override
IAnimal* p = new IAnimal();
MockScriptHook hook;
hook.AddOverride("GetAge", [](auto, void* r, auto){ *(int*)r = 42; });
pCls->HookVirtualTable(p, &hook);
p->GetAge();  // 返回 42 — 通过 Hook 分派
```

### 字段路径遍历
```cpp
// 一次操作完成 obj.m_Pos.m_X 的链式访问
SFieldNode path[] = { SFieldNode(pFieldPos), SFieldNode(pFieldX) };
SFieldPath fp = { path, 2 };

// 读取
float x;
pCls->Fetch(&obj, fp, &x, [](void* d, void* c){ *(float*)c = *(float*)d; });

// 写入
float newX = 9.0f;
pCls->Modify(&obj, &newX, false, fp, eFOT_Modify);
```

### 容器内省
```cpp
const CField* pF = pCls->FindField("m_Values");
if (pF->GetResultType().IsClass()) {
    const CClass* pCont = pF->GetResultType().GetClass();
    UPakDataType elem = pCont->GetContainerElemType();   // 元素类型，如 float
    int64 n = pCont->GetContainerSize(&obj.m_Values);    // 元素数量
}
```

### 模板注册
```cpp
// 1. 注册自定义模板类 — 任何实例化自动被反射
template<typename T>
struct MyBox { T m_Value; };
DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM( MyBox, typename )
    REGIST_CLASSMEMBER( m_Value )
DEFINE_TEMPLATE_END()

// MyBox<float>、MyBox<int> 等首次使用时自动注册
const CClass* pC = R.FindClassByTypeID( TypeName<MyBox<float>>() );

// 2. STL 容器已通过 Template/std.h 预注册
#include "Template/std.h"
// std::vector<Item>、std::set<int>、std::string、std::map<K,V> 等
// 均暴露 GetContainerElemType() 和 GetContainerSize()
```

xreflect 采用**模式化**模板注册 — 定义模板一次，每个实例化在首次使用时自动反射。
与实例化库（每个 <int>、<float> 需手动注册）不同，xreflect 注册模板模式并按需解析实例化。

Template/std.h 预注册了完整 STL 的模式：std::vector、std::set、std::map、
std::string、std::pair、std::array、std::list、std::shared_ptr 等——全部支持容器内省。

## 示例

| # | 示例 | 演示内容 |
|---|------|----------|
| 01 | quick_start | 最小结构体注册 + 字段读写 |
| 02 | fields_and_methods | 通过 CField::Call() 调用方法 |
| 03 | properties | 计算属性、只读、限定符标志 |
| 04 | enum | 枚举注册和值查找 |
| 05 | inheritance | 基类、IsA()、跨层级 FindField |
| 06 | virtual_override | ICallbackHook — 脚本覆写 C++ 虚函数 |
| 07 | templates | TRegisterTypes 将模板类型拉入反射 |
| 08 | fieldpath | SFieldNode/SFieldPath — 链式 Fetch |
| 09 | containers | GetContainerElemType() / GetContainerSize() |
| 10 | construction | CClass::Construct() / New() / Delete() |
| 11 | modify | CClass::Modify() — 通过字段路径写入 |
| 12 | comprehensive | 全宏面展示 — 每个注册宏都在一个示例中 |
| 13 | global_function | REGIST_GLOBALCMETHOD — 全局函数注册 |
| 14 | script_binding | 通过 xreflect 进行 Lua 和 JavaScript 运行时绑定 |
| —  | test_suite | 38 个测试用例覆盖所有功能 |
| —  | benchmark | xreflect vs 原生 C++ 性能基准测试 |

```
cmake --build build --config Release
build/examples/test_suite/Release/test_suite.exe
```

### 示例 14 — 脚本绑定

本示例演示通过 xreflect 自动将 C++ 类绑定到 Lua 和 JavaScript：

```bash
cmake --build build --config Release --target scriptbinding
build/examples/14_script_binding/Release/scriptbinding.exe
```

输入脚本：
- [`test.lua`](examples/14_script_binding/test.lua) — Lua 绑定脚本
- [`test.js`](examples/14_script_binding/test.js) — JavaScript 绑定脚本

## API 参考

### 核心类型

| 类型 | 描述 |
|------|------|
| CReflection | 单例类型注册表 |
| CClass | 类元数据 — 大小、字段、基类、构造器 |
| CField | 字段、方法、属性或回调描述符 |
| CName | 驻留字符串，通过整数索引实现 O(1) 比较 |
| UPakDataType | 64 位紧凑类型描述符（指针深度 + const + ref + 数据） |
| SFieldNode | 字段路径中的单一节点（成员 / 类型转换 / 索引 / 自定义访问器） |
| SFieldPath | SFieldNode 的有序序列，用于复合字段导航 |
| CFieldTags | 分组和显示的标签元数据 |
| IClassDefinition | 对象生命周期 — 构造、析构、赋值、移动 |
| ICallbackHook | 虚函数拦截 — 脚本继承 C++ 类 |

### 注册宏

**类宏：**
DEFINE_CLASS_BEGIN(T) / DEFINE_CLASS_END()、REGIST_DEFAULT_CONSTRUCTOR()、REGIST_CUSTOM_CONSTRUCTOR(...)、REGIST_DESTRUCTOR()、REGIST_BASE_CLASS(Base)、

**成员宏：**
REGIST_CLASSMEMBER(field)、REGIST_CLASSPROPERTY(name,get,set)、REGIST_READONLY_CLASSPROPERTY(name,get)、REGIST_STATICMEMBER(member)、REGIST_STATICPROPERTY(name,get,set)、

**方法宏：**
REGIST_CLASSMETHOD(method)、REGIST_STATICMETHOD(method)、REGIST_CLASSCALLBACK(virtual_fn)、REGIST_GLOBALCMETHOD(fn)、

**限定符变体：**
REGIST_EDIT_DISABLE_CLASSMEMBER、REGIST_SERIALIZE_DISABLE_CLASSMEMBER、_WITH_NAME、_WITH_TYPE、_LAMBDA、

**枚举宏：**
DEFINE_ENUM_BEGIN(E) / DEFINE_ENUM_END()、REGIST_ENUMVALUE(V)、

**模板宏：**
DEFINE_TEMPLATE_BEGIN_WITH_1_PARAM(T, T0) / DEFINE_TEMPLATE_END()、REGIST_ARRAY(Elem, size_fn)、REGIST_ITERATOR(It)、REGIST_AS_STRONG_POINTER()、REGIST_AS_WEAK_POINTER(StrongType)、

### UPakDataType — 64 位布局

```
Bit  0–1:  指针深度（0–3）
Bit  2:    是否引用
Bit  3:    是否 const
Bit  4–15: 保留
Bit 16–63: CClass* 指针（类类型）或 EDataType ID（基础类型）
```

### SFieldNode — 64 位 Tagged Union

| 类型 | 载荷 | 用途 |
|------|------|------|
| eFNT_NormalField | const CField* | 成员访问 |
| eFNT_TypeCast | const CClass* | 基类转换 |
| eFNT_ElementIndex | uint64 | 容器元素索引 |
| eFNT_CustomFetcher | IFieldNodeFetcher* | 自定义访问器 |

## 架构

xreflect 通过一次注册解决四个问题：

1. **序列化** — CClass::Modify/Fetch + SFieldPath 对任意嵌套字段提供读写
2. **编辑器面板** — EnumFields() + 限定符标志自动生成属性控件
3. **撤销/重做** — SFieldPath 可作为命令系统的可序列化操作键
4. **脚本绑定** — 遍历所有已注册类，自动导出到 Lua、JS 等

## 要求与限制

- **编译器：** C++20（MSVC 2022+、GCC 12+、Clang 16+）
- **架构：** 仅 64 位（static_assert(sizeof(void*) == 8)）
- **指针深度：** 最多 3 层间接引用
- **虚表大小：** 每个类最多 512 项
- **字符串长度：** 最大 8192 字节（XR_MAX_NAME_LENGTH）
- **虚表 Hook** 依赖编译器 ABI（x86-64 上的 MSVC/GCC/Clang）。ARM/AArch64 或 -fvisibility=hidden 下不保证工作。
- **线程安全：** 类注册通过 std::recursive_mutex 保证线程安全。字段访问无锁。

## 第三方库致谢

本项目内嵌了以下开源库：

- **Lua 5.3** — 版权所有 (c) 1994-2026 Lua.org, PUC-Rio。[MIT 许可证](examples/14_script_binding/third_party/lua/LICENSE)
- **QuickJS** — 版权所有 (c) 2017-2021 Fabrice Bellard, Charlie Gordon。[MIT 许可证](examples/14_script_binding/third_party/quickjs/LICENSE)
- **RTTR v0.9.6** — 版权所有 (c) 2014-2018 Axel Menzel。[MIT 许可证](examples/benchmark/third_party/rttr/LICENSE.txt)

## 开源协议

基于 MIT License 授权。版权声明：Copyright (c) 2019-2026 Daphnis Kau。


---

## Appendix: Raw Benchmark Data

Measured on Intel i7-13700H, MSVC 19.50, Release /O2, 10M iterations.

### Field read (hot path)

| System | Run 1 | Run 2 | Run 3 | Average |
|--------|-------|-------|-------|---------|
| native | 0.29 ns | 0.29 ns | 0.28 ns | **0.3 ns** |
| xreflect | 1.76 ns | 1.76 ns | 1.72 ns | **1.7 ns** |
| RTTR | 44.95 ns | 40.40 ns | 37.75 ns | **41.0 ns** |

### Field write (hot path)

| System | Run 1 | Run 2 | Run 3 | Average |
|--------|-------|-------|-------|---------|
| native | 0.31 ns | 0.32 ns | 0.32 ns | **0.3 ns** |
| xreflect | 1.49 ns | 1.48 ns | 1.50 ns | **1.5 ns** |
| RTTR | 14.71 ns | 13.92 ns | 14.09 ns | **14.2 ns** |

### Method call (Length)

| System | Run 1 | Run 2 | Run 3 | Average |
|--------|-------|-------|-------|---------|
| native | 1.06 ns | 1.11 ns | 1.15 ns | **1.1 ns** |
| xreflect | 4.64 ns | 4.28 ns | 3.50 ns | **4.1 ns** |
| RTTR | 39.92 ns | 46.47 ns | 43.33 ns | **43.2 ns** |

### Void method call (Normalize)

| System | Run 1 | Run 2 | Run 3 | Average |
|--------|-------|-------|-------|---------|
| xreflect | 4.80 ns | 4.80 ns | 5.73 ns | **5.1 ns** |
| RTTR | 17.87 ns | 19.45 ns | 17.71 ns | **18.3 ns** |

### Lookup + read (worst case)

| System | Run 1 | Run 2 | Run 3 | Average |
|--------|-------|-------|-------|---------|
| xreflect | 73.06 ns | 75.24 ns | 86.47 ns | **78.3 ns** |
| RTTR | 74.31 ns | 77.61 ns | 67.04 ns | **73.0 ns** |

---

## 附录：原始测试数据

测试环境：Intel i7-13700H, MSVC 19.50, Release /O2, 1000 万次迭代。

### 字段读取（热路径）

| 系统 | Run 1 | Run 2 | Run 3 | 平均 |
|------|-------|-------|-------|------|
| 原生 | 0.29 ns | 0.29 ns | 0.28 ns | **0.3 ns** |
| xreflect | 1.76 ns | 1.76 ns | 1.72 ns | **1.7 ns** |
| RTTR | 44.95 ns | 40.40 ns | 37.75 ns | **41.0 ns** |

### 字段写入（热路径）

| 系统 | Run 1 | Run 2 | Run 3 | 平均 |
|------|-------|-------|-------|------|
| 原生 | 0.31 ns | 0.32 ns | 0.32 ns | **0.3 ns** |
| xreflect | 1.49 ns | 1.48 ns | 1.50 ns | **1.5 ns** |
| RTTR | 14.71 ns | 13.92 ns | 14.09 ns | **14.2 ns** |

### 方法调用（Length）

| 系统 | Run 1 | Run 2 | Run 3 | 平均 |
|------|-------|-------|-------|------|
| 原生 | 1.06 ns | 1.11 ns | 1.15 ns | **1.1 ns** |
| xreflect | 4.64 ns | 4.28 ns | 3.50 ns | **4.1 ns** |
| RTTR | 39.92 ns | 46.47 ns | 43.33 ns | **43.2 ns** |

### 空方法调用（Normalize）

| 系统 | Run 1 | Run 2 | Run 3 | 平均 |
|------|-------|-------|-------|------|
| xreflect | 4.80 ns | 4.80 ns | 5.73 ns | **5.1 ns** |
| RTTR | 17.87 ns | 19.45 ns | 17.71 ns | **18.3 ns** |

### 查找+读取（最差情况）

| 系统 | Run 1 | Run 2 | Run 3 | 平均 |
|------|-------|-------|-------|------|
| xreflect | 73.06 ns | 75.24 ns | 86.47 ns | **78.3 ns** |
| RTTR | 74.31 ns | 77.61 ns | 67.04 ns | **73.0 ns** |
