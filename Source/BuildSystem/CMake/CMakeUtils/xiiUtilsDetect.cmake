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
# ## xii_pull_platform_vars()
# #####################################
macro(xii_pull_platform_vars)
  get_property(XII_CMAKE_PLATFORM_NAME GLOBAL PROPERTY XII_CMAKE_PLATFORM_NAME)
  get_property(XII_CMAKE_PLATFORM_PREFIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX)
  get_property(XII_CMAKE_PLATFORM_POSTFIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSTFIX)
  get_property(XII_CMAKE_PLATFORM_POSIX GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSIX)
  get_property(XII_CMAKE_PLATFORM_SUPPORTS_VULKAN GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_VULKAN)
  get_property(XII_CMAKE_PLATFORM_SUPPORTS_D3D12 GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_D3D12)
  get_property(XII_CMAKE_PLATFORM_SUPPORTS_EDITOR GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_EDITOR)

  xii_platform_pull_properties()
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
  message(STATUS "CMAKE_GENERATOR is '${CMAKE_GENERATOR}'")

  xii_platform_detect_generator()
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
    # message(SEND_ERROR "Redundant call to xii_detect_compiler()")
    return()
  endif()

  xii_pull_platform_vars()
  xii_pull_generator_vars()
  xii_pull_config_vars()
  get_property(GENERATOR_MSVC GLOBAL PROPERTY XII_CMAKE_GENERATOR_MSVC)

  set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "")
  set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC OFF)
  set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_CLANG OFF)
  set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_GCC OFF)

  set(FILE_TO_COMPILE "${XII_ROOT}/${XII_CMAKE_RELPATH}/ProbingSrc/ArchitectureDetect.c")

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

    if (XII_DETECTED_MSVC_VER GREATER_EQUAL 1950)
      message(STATUS "Compiler is Visual Studio 2026")
      set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2026")

    elseif(XII_DETECTED_MSVC_VER GREATER_EQUAL 1930)
      message(STATUS "Compiler is Visual Studio 2022")
      set_property(GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX "2022")

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
# ## xii_detect_cpuid_flags()
# #####################################
function(xii_detect_cpuid_flags)
  # Early return if flags are already detected.
  get_property(PREFIX GLOBAL PROPERTY XII_CMAKE_CPU_ID_FLAGS)

  if(PREFIX)
    # Has already run before and XII_CMAKE_CPU_ID_FLAGS is already set.
    # message(SEND_ERROR "Redundant call to xii_detect_cpuid_flags()")
    return()
  endif()

  set_property(GLOBAL PROPERTY XII_CMAKE_CPU_ID_FLAGS "")

  set(FILE_TO_COMPILE "${XII_ROOT}/${XII_CMAKE_RELPATH}/ProbingSrc/CpuIdFlagsDetect.c")

  if (XII_SDK_DIR)
    set(FILE_TO_COMPILE "${XII_SDK_DIR}/${XII_CMAKE_RELPATH}/ProbingSrc/CpuIdFlagsDetect.c")
  endif()

  # Check if we need to run detection.
  if(NOT XII_DETECTED_CPU_ID_FLAGS)
    # Configure try_compile.
    set(CMAKE_TRY_COMPILE_TARGET_TYPE "EXECUTABLE")

    try_compile(
      COMPILE_RESULT
      ${CMAKE_CURRENT_BINARY_DIR}
      ${FILE_TO_COMPILE}
      CMAKE_FLAGS -DCMAKE_C_FLAGS="/EHsc /W4"
      OUTPUT_VARIABLE COMPILE_OUTPUT
      COPY_FILE ${CMAKE_BINARY_DIR}/xiiCPUIdFlagsDetect
    )

    if(NOT COMPILE_RESULT)
      message(FATAL_ERROR "Failed to detect CPU ID flags. Compiler output: ${COMPILE_OUTPUT}")
      return()
    endif()

    execute_process(
      COMMAND ${CMAKE_BINARY_DIR}/xiiCPUIdFlagsDetect
      OUTPUT_VARIABLE XII_CPU_ID_FLAGS_DETECT
      ERROR_VARIABLE XII_CPU_ID_FLAGS_DETECT_ERRORS
      RESULT_VARIABLE XII_CPU_ID_FLAGS_DETECT_RESULT
      COMMAND_ERROR_IS_FATAL ANY
    )

    if(NOT XII_CPU_ID_FLAGS_DETECT_RESULT EQUAL 0)
      message(SEND_ERROR "CPU ID flags test failed. Output: ${XII_CPU_ID_FLAGS_DETECT}")
      return()
    endif()

    string(REGEX REPLACE "\n" ";" XII_CPU_ID_FLAGS_DETECT_LIST "${XII_CPU_ID_FLAGS_DETECT}")
    set(XII_DETECTED_CPU_ID_FLAGS ${XII_CPU_ID_FLAGS_DETECT_LIST} CACHE INTERNAL "")
  endif()

  set_property(GLOBAL PROPERTY XII_CMAKE_CPU_ID_FLAGS "${XII_DETECTED_CPU_ID_FLAGS}")
  message(STATUS "CPU ID Flags (XII_CMAKE_CPU_ID_FLAGS) are ${XII_DETECTED_CPU_ID_FLAGS}")
endfunction()

# #####################################
# ## xii_pull_compiler_vars()
# #####################################
macro(xii_pull_compiler_and_architecture_vars)
  xii_detect_compiler_and_architecture()
  xii_detect_cpuid_flags()

  get_property(XII_CMAKE_COMPILER_POSTFIX GLOBAL PROPERTY XII_CMAKE_COMPILER_POSTFIX)
  get_property(XII_CMAKE_COMPILER_MSVC GLOBAL PROPERTY XII_CMAKE_COMPILER_MSVC)
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
  get_property(XII_ROOT GLOBAL PROPERTY XII_ROOT)

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

  xii_get_version("${XII_ROOT}/version.txt" VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

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
