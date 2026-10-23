cmake_minimum_required(VERSION 3.21)

project(
	RealExpressionVR
	VERSION 1.0.0
	LANGUAGES CXX
)

set(PLUGIN_AUTHOR "zuofu")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

# ---- CommonLibSSE-NG (one DLL for SE, AE and VR) ----
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE-NG EXCLUDE_FROM_ALL)
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

# ---- Plugin ----
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)

add_commonlibsse_plugin(
	${PROJECT_NAME}
	AUTHOR "${PLUGIN_AUTHOR}"
	SOURCES ${SOURCES}
	# Address Library makes the plugin version-independent across SE/AE/VR.
	USE_ADDRESS_LIBRARY
)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h" REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE src ${SIMPLEINI_INCLUDE_DIRS})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
# Winsock, for the OSC receiver.
target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)

# ---- Optional: copy the DLL/PDB into a mod folder after building ----
# Configure with -DOUTPUT_FOLDER="C:/path/to/MO2/mods/MyPlugin"
if(DEFINED OUTPUT_FOLDER)
	set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/data/SKSE/Plugins/${PROJECT_NAME}.ini" "${DLL_FOLDER}"
		VERBATIM
	)
endif()
