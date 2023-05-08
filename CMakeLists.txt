cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not supported. Please choose a different binary directory.")
	return()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/xiiCMakeConfig.cmake")
get_property(XII_CMAKE_RELPATH GLOBAL PROPERTY XII_CMAKE_RELPATH)
get_property(XII_CMAKE_RELPATH_CODE GLOBAL PROPERTY XII_CMAKE_RELPATH_CODE)

# Required to find cmake modules like 'FindDirectX.cmake'
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/${XII_CMAKE_RELPATH}")

include("xiiUtils")

file(RELATIVE_PATH XII_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH ${XII_SUBMODULE_PREFIX_PATH})

if(XII_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(XII_SUBMODULE_MODE FALSE)
else()
	set(XII_SUBMODULE_MODE TRUE)
endif()

set_property(GLOBAL PROPERTY XII_SUBMODULE_MODE ${XII_SUBMODULE_MODE})

xii_build_filter_init()

xii_detect_project_name(DETECTED_NAME)

if(NOT XII_SUBMODULE_MODE)
	xii_detect_languages()

	set(XII_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")
	project(${XII_SOLUTION_NAME}
		VERSION 1.0
		DESCRIPTION "Copyright (c) 2022-2023 Theophilus Eriata. All Rights Reserved."
		HOMEPAGE_URL "https://gitlab.com/xii1/XII-Engine"
		LANGUAGES ${XII_LANGUAGES})
endif()

xii_set_build_types()
xii_check_build_type()
xii_write_configuration_txt()

if(XII_CUSTOM_TOOLCHAIN_FILE)
	message(STATUS "Including XII_CUSTOM_TOOLCHAIN_FILE: '${XII_CUSTOM_TOOLCHAIN_FILE}'")
	include("${XII_CUSTOM_TOOLCHAIN_FILE}")
endif()

xii_pull_all_vars()

include("xiiPlatformVars")

set(XII_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")

xii_find_qt()
xii_init_projects()

get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY XII_BUILD_FILTER_SELECTED)
if (NOT ${SELECTED_FILTER_NAME} MATCHES "Everything")
	set(CMAKE_SUPPRESS_REGENERATION true)
endif()
add_subdirectory(${XII_CMAKE_RELPATH_CODE})

xii_add_external_projects_folder("_1")
xii_add_external_projects_folder("_2")
xii_add_external_projects_folder("_3")

if(NOT XII_SUBMODULE_MODE)
	if(TARGET Editor)
		# sets the Editor project as the default startup project in Visual Studio
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)
	endif()
endif()

# finalizers may run into problems in split solutions
# however, they aren't needed there, since CI will always build all projects
# and finalizers are typically only needed to set up build dependencies
if (${SELECTED_FILTER_NAME} MATCHES "Everything")
xii_finalize_projects()
endif()

get_property(EXPORT_PROJECTS GLOBAL PROPERTY "EXPORT_PROJECTS")
export(TARGETS ${EXPORT_PROJECTS} FILE "${XII_OUTPUT_DIRECTORY_DLL}/xiiExport.cmake")

set(EXPORT_INFO "
set(EXPINP_OUTPUT_DIRECTORY_DLL ${XII_OUTPUT_DIRECTORY_DLL})
set(EXPINP_OUTPUT_DIRECTORY_LIB ${XII_OUTPUT_DIRECTORY_LIB})
set(EXPINP_BINARY_DIR ${CMAKE_BINARY_DIR})
set(EXPINP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
")

file(WRITE "${XII_OUTPUT_DIRECTORY_DLL}/xiiExportInfo.cmake" ${EXPORT_INFO})
