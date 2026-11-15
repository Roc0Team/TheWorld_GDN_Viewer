cmake_minimum_required(VERSION 3.21)

project(TheWorld_GDN_Viewer LANGUAGES C CXX)

if(NOT WIN32 OR NOT MSVC)
	message(FATAL_ERROR "TheWorld_GDN_Viewer CMake build requires Windows and MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "TheWorld_GDN_Viewer requires a 64-bit build. Configure with -A x64.")
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
		"Supported build configurations" FORCE)
else()
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
	endif()
	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
		message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Debug or Release.")
	endif()
endif()

set(THEWORLD_EXTERNAL_ROOT "${PROJECT_SOURCE_DIR}/.." CACHE PATH
	"Directory containing godot-cpp, GSL, Eigen, jsoncpp, and sibling TheWorld projects")
set(THEWORLD_LIB_DIR "${PROJECT_SOURCE_DIR}/x64" CACHE PATH
	"Directory for the solution's generated libraries and DLLs")
set(THEWORLD_GODOT_CPP_DIR "${THEWORLD_EXTERNAL_ROOT}/godot-cpp" CACHE PATH
	"Path to the godot-cpp checkout")
set(THEWORLD_GODOT_CPP_LIB_DIR "${THEWORLD_GODOT_CPP_DIR}/bin" CACHE PATH
	"Directory containing the prebuilt godot-cpp import libraries")
option(THEWORLD_DEPLOY_NATIVE "Copy the built DLLs to the Godot addon after linking" ON)
set(THEWORLD_NATIVE_DIR "${THEWORLD_EXTERNAL_ROOT}/TheWorld_Viewer/TheWorld/addons/twviewer/native" CACHE PATH
	"Destination directory for the Godot addon DLLs")

set(_theworld_include_dirs
	"${THEWORLD_GODOT_CPP_DIR}/include"
	"${THEWORLD_GODOT_CPP_DIR}/gdextension"
	"${THEWORLD_GODOT_CPP_DIR}/gen/include"
	"${THEWORLD_EXTERNAL_ROOT}/TheWorld_MapManager"
	"${THEWORLD_EXTERNAL_ROOT}/GSL/include"
	"${PROJECT_SOURCE_DIR}/plog/include"
	"${THEWORLD_EXTERNAL_ROOT}/TheWorld_Utils"
	"${THEWORLD_EXTERNAL_ROOT}/eigen-5.0.0"
	"${THEWORLD_EXTERNAL_ROOT}/jsoncpp/include"
)

foreach(_include_dir IN LISTS _theworld_include_dirs)
	if(NOT IS_DIRECTORY "${_include_dir}")
		message(FATAL_ERROR
			"Required include directory not found: ${_include_dir}\n"
			"Set THEWORLD_EXTERNAL_ROOT or THEWORLD_GODOT_CPP_DIR to the correct location.")
	endif()
endforeach()

set(_theworld_required_libraries
	"${THEWORLD_GODOT_CPP_LIB_DIR}/libgodot-cpp.windows.editor.x86_64.lib"
	"${THEWORLD_GODOT_CPP_LIB_DIR}/libgodot-cpp.windows.template_release.x86_64.lib"
)

foreach(_library IN LISTS _theworld_required_libraries)
	if(NOT EXISTS "${_library}")
		message(FATAL_ERROR
			"Required prebuilt godot-cpp library not found: ${_library}\n"
			"Set THEWORLD_GODOT_CPP_LIB_DIR to the directory containing the godot-cpp import libraries.")
	endif()
endforeach()

enable_testing()
add_subdirectory("${THEWORLD_EXTERNAL_ROOT}/jsoncpp" "${CMAKE_BINARY_DIR}/lib_json")
add_subdirectory("${THEWORLD_EXTERNAL_ROOT}/TheWorld_Utils" "${CMAKE_BINARY_DIR}/TheWorld_Utils")
add_subdirectory("${THEWORLD_EXTERNAL_ROOT}/TheWorld_MapManager" "${CMAKE_BINARY_DIR}/TheWorld_MapManager")
add_subdirectory("${THEWORLD_EXTERNAL_ROOT}/TheWorld_Utils_UnitTest" "${CMAKE_BINARY_DIR}/TheWorld_Utils_UnitTest")

add_library(TheWorld_GDN_Viewer SHARED
	TheWorld_GDN_Viewer/Chunk.cpp
	TheWorld_GDN_Viewer/ClientServer.cpp
	TheWorld_GDN_Viewer/Collider.cpp
	TheWorld_GDN_Viewer/DisusedCode.cpp
	TheWorld_GDN_Viewer/dllmain.cpp
	TheWorld_GDN_Viewer/GDN_Template.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Camera.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Edit.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Exception.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Globals.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_MainNode.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Quadrant.cpp
	TheWorld_GDN_Viewer/GDN_TheWorld_Viewer.cpp
	TheWorld_GDN_Viewer/MeshCache.cpp
	TheWorld_GDN_Viewer/pch.cpp
	TheWorld_GDN_Viewer/QuadTree.cpp
	TheWorld_GDN_Viewer/register_types.cpp
	TheWorld_GDN_Viewer/Tools.cpp
	TheWorld_GDN_Viewer/Viewer_Utils.cpp
)

target_include_directories(TheWorld_GDN_Viewer PRIVATE ${_theworld_include_dirs})
target_compile_features(TheWorld_GDN_Viewer PRIVATE cxx_std_20)
target_compile_definitions(TheWorld_GDN_Viewer PRIVATE
	THEWORLDGDNVIEWER_EXPORTS
	_WINDOWS
	_USRDLL
	UNICODE
	_UNICODE
	_THEWORLD_CLIENT
	TYPED_METHOD_BIND
	"$<$<CONFIG:Debug>:_DEBUG>"
	"$<$<CONFIG:Release>:NDEBUG>"
)

target_link_libraries(TheWorld_GDN_Viewer PRIVATE
	Rpcrt4
	TheWorld_MapManager
	TheWorld_Utils
	lib_json
	"$<$<CONFIG:Debug>:${THEWORLD_GODOT_CPP_LIB_DIR}/libgodot-cpp.windows.editor.x86_64.lib>"
	"$<$<CONFIG:Release>:${THEWORLD_GODOT_CPP_LIB_DIR}/libgodot-cpp.windows.template_release.x86_64.lib>"
)

target_compile_options(TheWorld_GDN_Viewer PRIVATE
	/W3
	/sdl
	/permissive-
	/Zc:__cplusplus
)
target_link_options(TheWorld_GDN_Viewer PRIVATE /DEBUG)

set_target_properties(TheWorld_GDN_Viewer PROPERTIES
	CXX_EXTENSIONS OFF
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
	PREFIX ""
	OUTPUT_NAME_DEBUG "TheWorld_GDN_Viewer_d"
	OUTPUT_NAME_RELEASE "TheWorld_GDN_Viewer"
	RUNTIME_OUTPUT_DIRECTORY "${THEWORLD_LIB_DIR}/$<0:>"
	ARCHIVE_OUTPUT_DIRECTORY "${THEWORLD_LIB_DIR}/$<0:>"
)

if(THEWORLD_DEPLOY_NATIVE)
	if(NOT IS_DIRECTORY "${THEWORLD_NATIVE_DIR}")
		message(FATAL_ERROR "Godot addon destination does not exist: ${THEWORLD_NATIVE_DIR}. Set THEWORLD_NATIVE_DIR or disable THEWORLD_DEPLOY_NATIVE.")
	endif()
	add_custom_command(TARGET TheWorld_GDN_Viewer POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"$<TARGET_FILE:TheWorld_GDN_Viewer>"
			"$<TARGET_FILE:TheWorld_MapManager>"
			"$<TARGET_FILE:TheWorld_Utils>"
			"${THEWORLD_NATIVE_DIR}"
		COMMENT "Copying ${CMAKE_PROJECT_NAME} DLLs to the Godot addon"
		VERBATIM
	)
endif()
