# #####################################
# ## xii_detect_project_name(<out-name>)
# #####################################

function(xii_detect_project_name OUT_NAME)
	# unfortunately this has to be known before the PROJECT command,
	# but platform and compiler settings are only detected by CMake AFTER the project command
	# CMAKE_GENERATOR is the only value available before that, so we have to regex this a bit to
	# generate a useful name
	# thus, only VS solutions currently get nice names
	cmake_path(IS_PREFIX CMAKE_SOURCE_DIR ${CMAKE_BINARY_DIR} NORMALIZE IS_IN_SOURCE_BUILD)

	get_filename_component(NAME_REPO ${CMAKE_SOURCE_DIR} NAME)
	get_filename_component(NAME_DEST ${CMAKE_BINARY_DIR} NAME)

	set(DETECTED_NAME "${NAME_REPO}")

	if(NOT ${NAME_REPO} STREQUAL ${NAME_DEST})
		set(DETECTED_NAME "${DETECTED_NAME}_${NAME_DEST}")
	endif()

	set(${OUT_NAME} "${DETECTED_NAME}" PARENT_SCOPE)

	message(STATUS "Auto-detected solution name: ${DETECTED_NAME} (Generator = ${CMAKE_GENERATOR})")
endfunction()

# #####################################
# ## xii_detect_platform()
# #####################################
function(xii_detect_platform)
	get_property(PREFIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX)

	if(PREFIX)
		# has already run before and XII_CMAKE_PLATFORM_PREFIX is already set
		# message (STATUS "Redundant call to xii_detect_platform()")
		return()
	endif()

	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "")
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_DESKTOP OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_UWP OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_7 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_OSX OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_LINUX OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_ANDROID OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_EMSCRIPTEN OFF)

	message(STATUS "CMAKE_SYSTEM_NAME is '${CMAKE_SYSTEM_NAME}'")

	if(EMSCRIPTEN)
		message(STATUS "Platform is Emscripten (XII_CMAKE_PLATFORM_EMSCRIPTEN)")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_EMSCRIPTEN ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Web")

	elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows") # Desktop Windows
		message(STATUS "Platform is Windows (XII_CMAKE_PLATFORM_WINDOWS, XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)")
		message(STATUS "CMAKE_SYSTEM_VERSION is ${CMAKE_SYSTEM_VERSION}")
		message(STATUS "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Win")

		if(${CMAKE_SYSTEM_VERSION} EQUAL 6.1)
			set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_7 ON)
		endif()

	elseif(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") # Windows Universal
		message(STATUS "Platform is Windows Universal (XII_CMAKE_PLATFORM_WINDOWS, XII_CMAKE_PLATFORM_WINDOWS_UWP)")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_UWP ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "WinUWP")

	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CURRENT_OSX_VERSION) # OS X
		message(STATUS "Platform is OS X (XII_CMAKE_PLATFORM_OSX, XII_CMAKE_PLATFORM_POSIX)")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_OSX ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Osx")

	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux") # Linux
		message(STATUS "Platform is Linux (XII_CMAKE_PLATFORM_LINUX, XII_CMAKE_PLATFORM_POSIX)")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_LINUX ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Linux")

	elseif(CMAKE_SYSTEM_NAME STREQUAL "Android") # Android
		message(STATUS "Platform is Android (XII_CMAKE_PLATFORM_ANDROID, XII_CMAKE_PLATFORM_POSIX)")

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_ANDROID ON)

		set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Android")

	else()
		message(FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' is not supported! Please extend xii_detect_platform().")
	endif()

	get_property(XII_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS)

	if(XII_CMAKE_PLATFORM_WINDOWS)
		if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
			set(XII_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
		else()
			set(XII_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
			string(REGEX MATCHALL "\\." NUMBER_OF_DOTS "${XII_CMAKE_WINDOWS_SDK_VERSION}")
			list(LENGTH NUMBER_OF_DOTS NUMBER_OF_DOTS)

			if(NUMBER_OF_DOTS EQUAL 2)
				set(XII_CMAKE_WINDOWS_SDK_VERSION "${XII_CMAKE_WINDOWS_SDK_VERSION}.0")
			endif()
		endif()

		set_property(GLOBAL PROPERTY XII_CMAKE_WINDOWS_SDK_VERSION ${XII_CMAKE_WINDOWS_SDK_VERSION})
	endif()
endfunction()

# #####################################
# ## xii_pull_platform_vars()
# #####################################
macro(xii_pull_platform_vars)
	xii_detect_platform()

	get_property(XII_CMAKE_PLATFORM_PREFIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX)
	get_property(XII_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS)
	get_property(XII_CMAKE_PLATFORM_WINDOWS_UWP GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_UWP)
	get_property(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	get_property(XII_CMAKE_PLATFORM_WINDOWS_7 GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_7)
	get_property(XII_CMAKE_PLATFORM_POSIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX)
	get_property(XII_CMAKE_PLATFORM_OSX GLOBAL PROPERTY XII_CMAKE_PLATFORM_OSX)
	get_property(XII_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY XII_CMAKE_PLATFORM_LINUX)
	get_property(XII_CMAKE_PLATFORM_ANDROID GLOBAL PROPERTY XII_CMAKE_PLATFORM_ANDROID)
	get_property(XII_CMAKE_PLATFORM_EMSCRIPTEN GLOBAL PROPERTY XII_CMAKE_PLATFORM_EMSCRIPTEN)

	if(XII_CMAKE_PLATFORM_WINDOWS)
		get_property(XII_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY XII_CMAKE_WINDOWS_SDK_VERSION)
	endif()
endmacro()

# #####################################
# ## xii_detect_generator()
# #####################################
function(xii_detect_generator)
	get_property(PREFIX GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX)

	if(PREFIX)
		# has already run before and XII_CMAKE_GENERATOR_PREFIX is already set
		# message (STATUS "Redundant call to xii_detect_generator()")
		return()
	endif()

	xii_pull_platform_vars()

	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "")
	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION "undefined")
	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_MSVC OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_XCODE OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_MAKE OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_INSIDE_VS OFF) # if cmake is called through the visual studio open folder workflow

	message(STATUS "CMAKE_VERSION is '${CMAKE_VERSION}'")
	message(STATUS "CMAKE_BUILD_TYPE is '${CMAKE_BUILD_TYPE}'")

	string(FIND ${CMAKE_VERSION} "MSVC" VERSION_CONTAINS_MSVC)

	if(${VERSION_CONTAINS_MSVC} GREATER -1)
		message(STATUS "CMake was called from Visual Studio Open Folder workflow")
		set_property(GLOBAL PROPERTY XII_CMAKE_INSIDE_VS ON)
	endif()

	message(STATUS "CMAKE_GENERATOR is '${CMAKE_GENERATOR}'")

	if(XII_CMAKE_PLATFORM_WINDOWS) # Supported windows generators
		if(CMAKE_GENERATOR MATCHES "Visual Studio")
			# Visual Studio (All VS generators define MSVC)
			message(STATUS "Generator is MSVC (XII_CMAKE_GENERATOR_MSVC)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_MSVC ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Vs")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)
		elseif(CMAKE_GENERATOR MATCHES "Ninja") # Ninja makefiles. Only makefile format supported by Visual Studio Open Folder
			message(STATUS "Buildsystem is Ninja (XII_CMAKE_GENERATOR_NINJA)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
		else()
			message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend xii_detect_generator()")
		endif()

	elseif(XII_CMAKE_PLATFORM_OSX) # Supported OSX generators
		if(CMAKE_GENERATOR MATCHES "Xcode") # XCODE
			message(STATUS "Buildsystem is Xcode (XII_CMAKE_GENERATOR_XCODE)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_XCODE ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Xcode")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)

		elseif(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
			message(STATUS "Buildsystem is Make (XII_CMAKE_GENERATOR_MAKE)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_MAKE ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Make")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

		else()
			message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on OS X! Please extend xii_detect_generator()")
		endif()

	elseif(XII_CMAKE_PLATFORM_LINUX)
		if(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
			message(STATUS "Buildsystem is Make (XII_CMAKE_GENERATOR_MAKE)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_MAKE ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Make")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

		elseif(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
			message(STATUS "Buildsystem is Ninja (XII_CMAKE_GENERATOR_NINJA)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
		else()
			message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend xii_detect_generator()")
		endif()

	elseif(XII_CMAKE_PLATFORM_ANDROID)
		if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
			message(STATUS "Buildsystem is Ninja (XII_CMAKE_GENERATOR_NINJA)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

		else()
			message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Android! Please extend xii_detect_generator()")
		endif()

	elseif(XII_CMAKE_PLATFORM_EMSCRIPTEN)
		if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
			message(STATUS "Buildsystem is Ninja (XII_CMAKE_GENERATOR_NINJA)")

			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

		else()
			message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Emscripten! Please extend xii_detect_generator()")
		endif()

	else()
		message(FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' has not set up the supported generators. Please extend xii_detect_generator()")
	endif()
endfunction()

# #####################################
# ## xii_pull_generator_vars()
# #####################################
macro(xii_pull_generator_vars)
	xii_detect_generator()

	get_property(XII_CMAKE_GENERATOR_PREFIX GLOBAL PROPERTY XII_CMAKE_GENERATOR_PREFIX)
	get_property(XII_CMAKE_GENERATOR_CONFIGURATION GLOBAL PROPERTY XII_CMAKE_GENERATOR_CONFIGURATION)
	get_property(XII_CMAKE_GENERATOR_MSVC GLOBAL PROPERTY XII_CMAKE_GENERATOR_MSVC)
	get_property(XII_CMAKE_GENERATOR_XCODE GLOBAL PROPERTY XII_CMAKE_GENERATOR_XCODE)
	get_property(XII_CMAKE_GENERATOR_MAKE GLOBAL PROPERTY XII_CMAKE_GENERATOR_MAKE)
	get_property(XII_CMAKE_GENERATOR_NINJA GLOBAL PROPERTY XII_CMAKE_GENERATOR_NINJA)
	get_property(XII_CMAKE_INSIDE_VS GLOBAL PROPERTY XII_CMAKE_INSIDE_VS)
endmacro()

# #####################################
# ## xii_detect_compiler_and_architecture()
# #####################################
function(xii_detect_compiler_and_architecture)
	get_property(PREFIX GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX)

	if(PREFIX)
		# has already run before and XII_CMAKE_COMPILER_POSTFIX is already set
		# message (STATUS "Redundant call to xii_detect_compiler()")
		return()
	endif()

	xii_pull_platform_vars()
	xii_pull_generator_vars()
	xii_pull_config_vars()
	get_property(GENERATOR_MSVC GLOBAL PROPERTY XII_CMAKE_GENERATOR_MSVC)

	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "")
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_140 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_141 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_142 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_143 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_CLANG OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_GCC OFF)

	set(FILE_TO_COMPILE "${CMAKE_SOURCE_DIR}/${XII_SUBMODULE_PREFIX_PATH}/${XII_CMAKE_RELPATH}/ProbingSrc/ArchitectureDetect.c")
	
	if (XII_SDK_DIR)
		set(FILE_TO_COMPILE "${XII_SDK_DIR}/${XII_CMAKE_RELPATH}/ProbingSrc/ArchitectureDetect.c")
	endif()

	# Only compile the detect file if we don't have a cached result from the last run
	if((NOT XII_DETECTED_COMPILER) OR (NOT XII_DETECTED_ARCH) OR (NOT XII_DETECTED_MSVC_VER))
		set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
		try_compile(COMPILE_RESULT
			${CMAKE_CURRENT_BINARY_DIR}
			${FILE_TO_COMPILE}
			OUTPUT_VARIABLE COMPILE_OUTPUT
		)

		if(NOT COMPILE_RESULT)
			message(FATAL_ERROR "Failed to detect compiler / target architecture. Compiler output: ${COMPILE_OUTPUT}")
		endif()

		if(${COMPILE_OUTPUT} MATCHES "ARCH:'([^']*)'")
			set(XII_DETECTED_ARCH ${CMAKE_MATCH_1} CACHE INTERNAL "")
		else()
			message(FATAL_ERROR "The compile test did not output the architecture. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
		endif()

		if(${COMPILE_OUTPUT} MATCHES "COMPILER:'([^']*)'")
			set(XII_DETECTED_COMPILER ${CMAKE_MATCH_1} CACHE INTERNAL "")
		else()
			message(FATAL_ERROR "The compile test did not output the compiler. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
		endif()
		
		if(XII_DETECTED_COMPILER STREQUAL "msvc")
			if(${COMPILE_OUTPUT} MATCHES "MSC_VER:'([^']*)'")
				set(XII_DETECTED_MSVC_VER ${CMAKE_MATCH_1} CACHE INTERNAL "")
			else()
				message(FATAL_ERROR "The compile test did not output the MSC_VER. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
			endif()
		else()
			set(XII_DETECTED_MSVC_VER "<NOT USING MSVC>" CACHE INTERNAL "")
		endif()
	endif()

	if(XII_DETECTED_COMPILER STREQUAL "msvc") # Visual Studio Compiler
		message(STATUS "Compiler is MSVC (XII_CMAKE_COMPILER_MSVC) version ${XII_DETECTED_MSVC_VER}")

		set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC ON)

		if(XII_DETECTED_MSVC_VER GREATER_EQUAL 1930)
			message(STATUS "Compiler is Visual Studio 2022 (XII_CMAKE_COMPILER_MSVC_143)")
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_143 ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2022")

		elseif(XII_DETECTED_MSVC_VER GREATER_EQUAL 1920)
			message(STATUS "Compiler is Visual Studio 2019 (XII_CMAKE_COMPILER_MSVC_142)")
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_142 ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2019")

		elseif(XII_DETECTED_MSVC_VER GREATER_EQUAL 1910)
			message(STATUS "Compiler is Visual Studio 2017 (XII_CMAKE_COMPILER_MSVC_141)")
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_141 ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2017")

		elseif(MSVC_VERSION GREATER_EQUAL 1900)
			message(STATUS "Compiler is Visual Studio 2015 (XII_CMAKE_COMPILER_MSVC_140)")
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_140 ON)
			set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2015")

		else()
			message(FATAL_ERROR "Compiler for generator '${CMAKE_GENERATOR}' is not supported on MSVC! Please extend xii_detect_compiler()")
		endif()

	elseif(XII_DETECTED_COMPILER STREQUAL "clang")
		message(STATUS "Compiler is clang (XII_CMAKE_COMPILER_CLANG)")
		set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_CLANG ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "Clang")

	elseif(XII_DETECTED_COMPILER STREQUAL "gcc")
		message(STATUS "Compiler is gcc (XII_CMAKE_COMPILER_GCC)")
		set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_GCC ON)
		set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "Gcc")

	else()
		message(FATAL_ERROR "Unhandled compiler ${XII_DETECTED_COMPILER}")
	endif()

	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX "")
	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_64BIT OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_X86 OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_ARM OFF)
	set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_EMSCRIPTEN OFF)

	if(XII_DETECTED_ARCH STREQUAL "x86")
		message(STATUS "Architecture is X86 (XII_CMAKE_ARCHITECTURE_X86)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_X86 ON)

		message(STATUS "Architecture is 32-Bit (XII_CMAKE_ARCHITECTURE_32BIT)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT ON)

	elseif(XII_DETECTED_ARCH STREQUAL "x64")
		message(STATUS "Architecture is X86 (XII_CMAKE_ARCHITECTURE_X86)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_X86 ON)

		message(STATUS "Architecture is 64-Bit (XII_CMAKE_ARCHITECTURE_64BIT)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_64BIT ON)

	elseif(XII_DETECTED_ARCH STREQUAL "arm32")
		message(STATUS "Architecture is ARM (XII_CMAKE_ARCHITECTURE_ARM)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_ARM ON)

		message(STATUS "Architecture is 32-Bit (XII_CMAKE_ARCHITECTURE_32BIT)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT ON)

	elseif(XII_DETECTED_ARCH STREQUAL "arm64")
		message(STATUS "Architecture is ARM (XII_CMAKE_ARCHITECTURE_ARM)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_ARM ON)

		message(STATUS "Architecture is 64-Bit (XII_CMAKE_ARCHITECTURE_64BIT)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_64BIT ON)

	elseif(XII_DETECTED_ARCH STREQUAL "emscripten")
		message(STATUS "Architecture is WEBASSEMBLY (XII_CMAKE_ARCHITECTURE_WEBASSEMBLY)")
		set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_WEBASSEMBLY ON)

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			message(STATUS "Architecture is 64-Bit (XII_CMAKE_ARCHITECTURE_64BIT)")
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_64BIT ON)
		else()
			message(STATUS "Architecture is 32-Bit (XII_CMAKE_ARCHITECTURE_32BIT)")
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT ON)
		endif()

	else()
		message(FATAL_ERROR "Unhandled target architecture ${XII_DETECTED_ARCH}")
	endif()

	get_property(XII_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT)
	get_property(XII_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_ARM)

	if(XII_CMAKE_ARCHITECTURE_ARM)
		if(XII_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX "Arm32")
		else()
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX "Arm64")
		endif()
	else()
		if(XII_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX "32")
		else()
			set_property(GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX "64")
		endif()
	endif()
endfunction()

# #####################################
# ## xii_pull_compiler_vars()
# #####################################
macro(xii_pull_compiler_and_architecture_vars)
	xii_detect_compiler_and_architecture()

	get_property(XII_CMAKE_COMPILER_POSTFIX GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX)
	get_property(XII_CMAKE_COMPILER_MSVC GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC)
	get_property(XII_CMAKE_COMPILER_MSVC_140 GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_140)
	get_property(XII_CMAKE_COMPILER_MSVC_141 GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_141)
	get_property(XII_CMAKE_COMPILER_MSVC_142 GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_142)
	get_property(XII_CMAKE_COMPILER_MSVC_143 GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC_143)
	get_property(XII_CMAKE_COMPILER_CLANG GLOBAL PROPERTY XII_CMAKE_COMPILER_CLANG)
	get_property(XII_CMAKE_COMPILER_GCC GLOBAL PROPERTY XII_CMAKE_COMPILER_GCC)

	get_property(XII_CMAKE_ARCHITECTURE_POSTFIX GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_POSTFIX)
	get_property(XII_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_32BIT)
	get_property(XII_CMAKE_ARCHITECTURE_64BIT GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_64BIT)
	get_property(XII_CMAKE_ARCHITECTURE_X86 GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_X86)
	get_property(XII_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_ARM)
	get_property(XII_CMAKE_ARCHITECTURE_WEBASSEMBLY GLOBAL PROPERTY XII_CMAKE_ARCHITECTURE_WEBASSEMBLY)
endmacro()

# #####################################
# ## xii_pull_all_vars()
# #####################################
macro(xii_pull_all_vars)
	get_property(XII_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH)

	xii_pull_version()
	xii_pull_compiler_and_architecture_vars()
	xii_pull_generator_vars()
	xii_pull_platform_vars()
endmacro()

# #####################################
# ## xii_get_version(<VERSIONFILE> <OUT_MAJOR> <OUT_MINOR> <OUT_PATCH>)
# #####################################
function(xii_get_version VERSIONFILE OUT_MAJOR OUT_MINOR OUT_PATCH)
	file(READ ${VERSIONFILE} VERSION_STRING)

	string(STRIP ${VERSION_STRING} VERSION_STRING)

	if(VERSION_STRING MATCHES "([0-9]+).([0-9]+).([0-9+])")
		STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" VERSION_MAJOR "${VERSION_STRING}")
		STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" VERSION_MINOR "${VERSION_STRING}")
		STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" VERSION_PATCH "${VERSION_STRING}")

		string(STRIP ${VERSION_MAJOR} VERSION_MAJOR)
		string(STRIP ${VERSION_MINOR} VERSION_MINOR)
		string(STRIP ${VERSION_PATCH} VERSION_PATCH)

		set(${OUT_MAJOR} ${VERSION_MAJOR} PARENT_SCOPE)
		set(${OUT_MINOR} ${VERSION_MINOR} PARENT_SCOPE)
		set(${OUT_PATCH} ${VERSION_PATCH} PARENT_SCOPE)

	else()
		message(FATAL_ERROR "Invalid version string '${VERSION_STRING}'")
	endif()
endfunction()

# #####################################
# ## xii_detect_version()
# #####################################
function(xii_detect_version)
	get_property(VERSION_MAJOR GLOBAL PROPERTY XII_CMAKE_SDKVERSION_MAJOR)

	if(VERSION_MAJOR)
		# has already run before and XII_CMAKE_SDKVERSION_MAJOR is already set
		return()
	endif()

	xii_get_version("${CMAKE_SOURCE_DIR}/${XII_SUBMODULE_PREFIX_PATH}/version.txt" VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

	set_property(GLOBAL PROPERTY XII_CMAKE_SDKVERSION_MAJOR "${VERSION_MAJOR}")
	set_property(GLOBAL PROPERTY XII_CMAKE_SDKVERSION_MINOR "${VERSION_MINOR}")
	set_property(GLOBAL PROPERTY XII_CMAKE_SDKVERSION_PATCH "${VERSION_PATCH}")

	message(STATUS "SDK version: Major = '${VERSION_MAJOR}', Minor = '${VERSION_MINOR}', Patch = '${VERSION_PATCH}'")
endfunction()

# #####################################
# ## xii_pull_version()
# #####################################
macro(xii_pull_version)
	xii_detect_version()

	get_property(XII_CMAKE_SDKVERSION_MAJOR GLOBAL PROPERTY XII_CMAKE_SDKVERSION_MAJOR)
	get_property(XII_CMAKE_SDKVERSION_MINOR GLOBAL PROPERTY XII_CMAKE_SDKVERSION_MINOR)
	get_property(XII_CMAKE_SDKVERSION_PATCH GLOBAL PROPERTY XII_CMAKE_SDKVERSION_PATCH)
endmacro()