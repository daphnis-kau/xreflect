# ============================================================================
# RTTR — embedded third-party source, built as static library
# ============================================================================
set(RTTR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rttr)
file(GLOB_RECURSE RTTR_SRC "${RTTR_ROOT}/src/rttr/*.cpp")
file(GLOB_RECURSE RTTR_HEADERS "${RTTR_ROOT}/src/rttr/*.h")

add_library(rttr_core STATIC ${RTTR_SRC} ${RTTR_HEADERS})
add_library(rttr::core ALIAS rttr_core)

target_include_directories(rttr_core PUBLIC
    ${RTTR_ROOT}/src
)
target_link_libraries(rttr_core PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(rttr_core PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
if(MSVC)
    # RTTR's headers rely on standard library includes brought in via PCH in the
    # original build system. Force-include the PCH header for all RTTR sources.
    target_compile_options(rttr_core PRIVATE /FI"${RTTR_ROOT}/src/rttr/detail/base/pch.h")
endif()

# ============================================================================
# Benchmark executable
# ============================================================================
add_executable(benchmark main.cpp)
target_link_libraries(benchmark xreflect rttr::core)
target_compile_definitions(benchmark PRIVATE $<$<CONFIG:Release>:NDEBUG>)
if(MSVC)
    # RTTR v0.9.6 uses some C++ constructs that MSVC's /permissive- (enabled by
    # default under /std:c++20) rejects. Restore the older parsing behavior.
    target_compile_options(benchmark PRIVATE /permissive)
endif()
