cmake_minimum_required(VERSION 3.20)
project(xreflect VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(xreflect STATIC
	src/CClass.cpp
	src/CField.cpp
	src/CReflection.cpp
	src/ReflectionHelp.cpp
	src/CName.cpp
)
target_include_directories(xreflect PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/xreflect>
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(XREFLECT_BUILD_EXAMPLES "Build example programs" ON)
if(XREFLECT_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

install(TARGETS xreflect
	EXPORT xreflectTargets
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include/xreflect)
install(EXPORT xreflectTargets
	NAMESPACE xreflect::
	DESTINATION lib/cmake/xreflect
)
