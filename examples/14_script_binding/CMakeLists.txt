# ============================================================================
# Lua 5.3 — embedded third-party source, built as static library
# ============================================================================
set(LUA53_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua)
file(GLOB LUA53_SRC     ${LUA53_ROOT}/*.c)
file(GLOB LUA53_HEADERS ${LUA53_ROOT}/*.h)
list(FILTER LUA53_SRC EXCLUDE REGEX "onelua\.c$|lua\.c$|luac\.c$")
add_library(lua53 STATIC ${LUA53_SRC} ${LUA53_HEADERS})
target_include_directories(lua53 PUBLIC ${LUA53_ROOT})

# ============================================================================
# QuickJS — embedded third-party source, built as static library
# ============================================================================
set(QJS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs)
set(QJS_SRC
    ${QJS_ROOT}/cutils.c
    ${QJS_ROOT}/libregexp.c
    ${QJS_ROOT}/libunicode.c
    ${QJS_ROOT}/quickjs.c
    ${QJS_ROOT}/quickjs-libc.c
    ${QJS_ROOT}/libbf.c
)
set(QJS_HEADERS
    ${QJS_ROOT}/cutils.h
    ${QJS_ROOT}/libregexp.h
    ${QJS_ROOT}/libregexp-opcode.h
    ${QJS_ROOT}/libunicode.h
    ${QJS_ROOT}/libunicode-table.h
    ${QJS_ROOT}/list.h
    ${QJS_ROOT}/quickjs.h
    ${QJS_ROOT}/quickjs-atom.h
    ${QJS_ROOT}/quickjs-libc.h
    ${QJS_ROOT}/quickjs-opcode.h
    ${QJS_ROOT}/quickjs-jsx.h
)
if(WIN32)
    list(APPEND QJS_HEADERS
        ${QJS_ROOT}/win/dirent.h
        ${QJS_ROOT}/win/getopt.h
    )
endif()
add_library(quickjs STATIC ${QJS_SRC} ${QJS_HEADERS})
target_include_directories(quickjs PUBLIC ${QJS_ROOT})
target_compile_definitions(quickjs PRIVATE
    CONFIG_DEBUGGER
    JS_STRICT_NAN_BOXING
    CONFIG_BIGNUM
    CONFIG_JSX
    _AMD64_
    WIN32_LEAN_AND_MEAN
)
if(MSVC)
    target_compile_options(quickjs PRIVATE /FI"winsock2.h")
    target_compile_options(quickjs PRIVATE /FI"${QJS_ROOT}/quickjsport.h")
    # Match XEngine's warning suppression for clean builds
    target_compile_options(quickjs PRIVATE
        /wd4013 /wd4018 /wd4101 /wd4146
        /wd4244 /wd4267 /wd4334 /wd4996
    )
endif()

# ============================================================================
# Lua Binding — C++ layer wrapping Lua C API via xreflect
# ============================================================================
add_library(lua_binding STATIC
    lua_binding/CScriptLua.cpp
    lua_binding/CScriptLua.h
    lua_binding/CTypeLua.cpp
    lua_binding/CTypeLua.h
)
target_link_libraries(lua_binding PUBLIC xreflect lua53)
target_include_directories(lua_binding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lua_binding
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# JS Binding — C++ layer wrapping QuickJS API via xreflect
# ============================================================================
add_library(js_binding STATIC
    js_binding/CScriptJS.cpp
    js_binding/CTypeJS.cpp
    js_binding/CScriptJS.h
    js_binding/CTypeJS.h
    js_binding/quickjsport.h
)
target_link_libraries(js_binding PUBLIC xreflect quickjs)
target_include_directories(js_binding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/js_binding
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(js_binding PRIVATE JS_STRICT_NAN_BOXING)

# ============================================================================
# Script Binding executable
# ============================================================================
add_executable(scriptbinding
    main.cpp
    CApplication.h
    CApplication.cpp
)
target_link_libraries(scriptbinding xreflect lua_binding js_binding)
target_include_directories(scriptbinding PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set_target_properties(lua53 quickjs lua_binding js_binding scriptbinding
    PROPERTIES FOLDER "examples/14_script_binding")
