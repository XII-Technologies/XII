# #####################################
# ## xii_check_build_type()
# #####################################

function(xii_check_build_type)
  # set the default build type
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${XII_BUILDTYPENAME_DEV} CACHE STRING "Choose the type of build, options are: None ${XII_BUILDTYPENAME_DEBUG} ${XII_BUILDTYPENAME_DEV} ${XII_BUILDTYPENAME_RELEASE}." FORCE)
  endif()
endfunction()

# #####################################
# ## xii_set_build_flags_msvc(<target>)
# #####################################
function(xii_set_build_flags_msvc TARGET_NAME)
  set(ARG_OPTIONS ENABLE_RTTI NO_WARNINGS_AS_ERRORS NO_COMPLIANCE NO_DEBUG)
  set(ARG_ONEVALUEARGS "")
  set(ARG_MULTIVALUEARGS "")
  cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

  xii_pull_config_vars()

  set(OPT_CPP_PRIVATE "")
  set(OPT_CPP_PUBLIC "")

  # target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")

  # Enable multi-threaded compilation
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/MP")

  # Disable RTTI
  if(${ARG_ENABLE_RTTI})
    message(STATUS "Enabling RTTI for target '${TARGET_NAME}'")
  else()
    set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/GR-")
  endif()

  # Use precise floating point model
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/fp:precise")

  # Enable floating point exceptions
  # set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/fp:except")

  # Enable default exception handling
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/EHsc")

  # Disable permissive mode
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/permissive-")

  # Enable standard conform casting behavior - casting results always in rvalue
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/Zc:rvalueCast")

  # Force the compiler to interpret code as utf8.
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/utf-8")

  # Set the __cplusplus preprocessor macro to something useful.
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/Zc:__cplusplus")

  # Set high warning level
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/W3")

  # /WX: Treat warnings as errors
  if(NOT ${ARG_NO_WARNINGS_AS_ERRORS} AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Deprecation warnings are not relevant at the moment, thus we can enable warnings as errors for now
    set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/WX")
    # switch Warning 4996 (deprecation warning) from warning level 3 to warning level 1
    # since you can't mark warnings as "not errors" in MSVC, we must switch off
    # the global warning-as-errors flag
    # instead we could switch ON selected warnings as errors
    set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/w14996")
  endif()

  # /Zo: Improved debugging of optimized code
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_RELEASE_UPPER}>:/Zo>")
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_DEV_UPPER}>:/Zo>")

  # /Ob1: Only consider functions for inlining that are marked with inline or forceinline
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_DEBUG_UPPER}>:/Ob1>")

  # /Ox: Favor speed for optimizations
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_RELEASE_UPPER}>:/Ox>")

  # /Ob2: Consider all functions for inlining
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_RELEASE_UPPER}>:/Ob2>")

  # /Oi: Replace some functions with intrinsics or other special forms of the function
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "$<$<CONFIG:${XII_BUILDTYPENAME_RELEASE_UPPER}>:/Oi>")

  set(LINKER_FLAGS_DEBUG "")

  # Do not remove unreferenced data. Required to make incremental linking work.
  set(LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG} /OPT:NOREF")

  # Do not enable comdat folding in debug. Required to make incremental linking work.
  set(LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG} /OPT:NOICF")

  set(LINKER_FLAGS_RELEASE "")

  set(LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /INCREMENTAL:NO")

  # Remove unreferenced data (does not work together with incremental build)
  set(LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /OPT:REF")

  # Enable comdat folding. Reduces the number of redundant template functions and thus reduces binary size. Makes debugging harder though.
  set(LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /OPT:ICF")

  if(${ARG_NO_DEBUG})
    # if NO_DEBUG is set, use the Dev build configuration even for Debug builds, to get best performance
    message(STATUS "Using Dev build flags for Debug builds '${TARGET_NAME}'")
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${LINKER_FLAGS_RELEASE})
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${LINKER_FLAGS_RELEASE})
  else()
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${LINKER_FLAGS_DEBUG})
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${LINKER_FLAGS_${XII_DEV_BUILD_LINKERFLAGS}})
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${LINKER_FLAGS_RELEASE})

  if(XII_ENABLE_COMPILER_STATIC_ANALYSIS)
    set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} "/analyze")
  endif()

  # Ignore various warnings we are not interested in

  # 4100 = Unreferenced formal parameter *
  # 4127 = Conditional expression is constant *
  # 4201 = Nonstandard extension used: nameless struct/union *
  # 4251 = Class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates) *
  # 4324 = Structure was padded due to alignment specifier *
  # 4345 = Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
  # 4714 = Function 'function' marked as __forceinline not inlined
  set(OPT_CPP_PUBLIC ${OPT_CPP_PUBLIC} /wd4201 /wd4251 /wd4324 /wd4345)
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} /wd4100 /wd4127 /wd4714)

  # Set Warnings as Errors: Too few/many parameters given for Macro
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} /we4002 /we4003)

  # 4099 = Linker warning "PDB was not found with lib"
  target_link_options(${TARGET_NAME} PRIVATE /ignore:4099)

  # 'nodiscard': Attribute is ignored in this syntactic position
  set(OPT_CPP_PRIVATE ${OPT_CPP_PRIVATE} /wd5240)

  target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${OPT_CPP_PRIVATE}>)
  target_compile_options(${TARGET_NAME} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${OPT_CPP_PUBLIC}>)
  target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:${OPT_CPP_PRIVATE}>)
  target_compile_options(${TARGET_NAME} PUBLIC $<$<COMPILE_LANGUAGE:C>:${OPT_CPP_PUBLIC}>)
endfunction()

# #####################################
# ## xii_set_build_flags_clang(<target>)
# #####################################
function(xii_set_build_flags_clang TARGET_NAME)
  # Disable warning: multi-character character constant
  target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)

  if(NOT(CMAKE_CURRENT_SOURCE_DIR MATCHES "Source/ThirdParty"))
    target_compile_options(${TARGET_NAME} PRIVATE -Werror=inconsistent-missing-override -Werror=switch -Werror=uninitialized -Werror=unused-result -Werror=return-type)
  else()
    # Ignore all warnings in third party code.
    target_compile_options(${TARGET_NAME} PRIVATE -Wno-everything)
  endif()

  if(XII_ENABLE_QT_SUPPORT)
    # Ignore any warnings caused by Qt headers
    target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${XII_QT_DIR}\"")
  endif()

  # Ignore any warnings caused by headers inside the ThirdParty directory.
  if(XII_SUBMODULE_PREFIX_PATH)
    target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${XII_ROOT}/Source/ThirdParty\"")
  else()
    target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${CMAKE_SOURCE_DIR}/Source/ThirdParty\"")
  endif()

  if(COMMAND xii_platformhook_set_build_flags_clang)
    # call platform-specific hook
    xii_platformhook_set_build_flags_clang(${TARGET_NAME})
  endif()
endfunction()

# #####################################
# ## xii_set_build_flags_gcc(<target>)
# #####################################
function(xii_set_build_flags_gcc TARGET_NAME)
  # Wno-enum-compare removes all annoying enum cast warnings
  # -fno-gnu-unique prevents symbols like static inline or static templates to be marked with STB_GNU_UNIQUE, preventing the owning dll from being unloaded.
  target_compile_options(${TARGET_NAME} PRIVATE -fPIC -Wno-enum-compare -gdwarf-3 -pthread -fno-gnu-unique)

  # Dynamic linking will fail without fPIC (plugins)
  # gdwarf-3 will use the old debug info which is compatible with older gdb versions.
  # These were previously set as CMAKE_C_FLAGS (not CPP)
  target_compile_options(${TARGET_NAME} PRIVATE -fPIC -gdwarf-3)

  # Disable warning: multi-character character constant
  target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)

  if(NOT(CMAKE_CURRENT_SOURCE_DIR MATCHES "Source/ThirdParty"))
    # Warning / Error settings for XII code
    # attributes = error if a attribute is placed incorrectly (e.g. XII_FOUNDATION_DLL)
    # unused-result = error if [[nodiscard]] return value is not handeled (xiiResult)
    target_compile_options(${TARGET_NAME} PRIVATE -Werror=attributes -Werror=unused-result -Wno-ignored-attributes -Werror=return-type)
  else()
    # Ignore all warnings in third party code.
    target_compile_options(${TARGET_NAME} PRIVATE -w)
  endif()

  # Look for the super fast ld compatible linker called "mold". If present we want to use it.
  # GCC can not be told to directly use mold. Instead if we have to look for a symlink called "ld"
  # Which might reside either in /usr/libexec/mold or /usr/local/libexec/mold
  # We can then use gcc's "-B" argument to specify the ld executable path.
  find_program(MOLD_PATH "ld" HINTS "/usr/libexec/mold" "/usr/local/libexec/mold" NO_DEFAULT_PATH)

  if(MOLD_PATH)
    get_filename_component(MOLD_DIR ${MOLD_PATH} DIRECTORY)

    # Use the ultra fast mold linker if present and the user didn't specify a different linker manually
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

    if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
      if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
        target_link_options(${TARGET_NAME} PRIVATE -B ${MOLD_DIR})
      endif()
    elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
      if(NOT("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
        target_link_options(${TARGET_NAME} PRIVATE -B ${MOLD_DIR})
      endif()
    endif()
  endif()

  # Reporting missing symbols at linktime
  if(("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY") OR("${TARGET_TYPE}" STREQUAL "EXECUTABLE"))
    target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
  endif()
endfunction()

# #####################################
# ## xii_set_simd_build_flags(<target>)
# #####################################
function(xii_set_simd_build_flags TARGET_NAME)
  if(XII_CMAKE_ARCHITECTURE_X86)
    get_property(cpuSimdFlags GLOBAL PROPERTY XII_CMAKE_CPU_ID_FLAGS)
    if(NOT cpuSimdFlags)
      message(WARNING "Global property XII_CMAKE_CPU_ID_FLAGS is not set.")
    endif()

    if(XII_CMAKE_COMPILER_MSVC)
      # Remove "/fp:except" from existing MSVC compile options.
      get_target_property(msvcOptions ${TARGET_NAME} COMPILE_OPTIONS)
      if(NOT msvcOptions OR msvcOptions STREQUAL "NOTFOUND")
        set(msvcOptions "")
      endif()
      list(REMOVE_ITEM msvcOptions "/fp:except")
      set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_OPTIONS "${msvcOptions}")

      # In MSVC only one /arch flag is allowed. Select highest available SIMD.
      # The ordering considers AVX2 highest, then AVX, then SSE2 then SSE.
      if("AVX2" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE /arch:AVX2)
      elseif("AVX" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE /arch:AVX)
      elseif("SSE2" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE /arch:SSE2)
      elseif("SSE" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE /arch:SSE)
      endif()

    elseif(XII_CMAKE_COMPILER_CLANG OR XII_CMAKE_COMPILER_GCC)
      # For GCC/Clang, select the most advanced available SIMD flag.
      # Here we test from highest to lowest instruction set.
      if("AVX2" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -mavx2 -mfma -mf16c -mbmi -mlzcnt)
      elseif("AVX" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -mavx -mf16c -mlzcnt)
      elseif("SSE4.2" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -msse4.2 -mf16c -mlzcnt)
      elseif("SSE4.1" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -msse4.1 -mf16c)
      elseif("SSSE3" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -mssse3)
      elseif("SSE3" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -msse3)
      elseif("SSE2" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -msse2)
      elseif("SSE" IN_LIST cpuSimdFlags)
        target_compile_options(${TARGET_NAME} PRIVATE -msse)
      endif()
    endif()

  elseif(XII_CMAKE_ARCHITECTURE_ARM)
    if(XII_CMAKE_COMPILER_CLANG OR XII_CMAKE_COMPILER_GCC)
      # Prefer using detected CPU flags (from the CpuIdFlagsDetect probe) when available.
      get_property(cpuSimdFlags GLOBAL PROPERTY XII_CMAKE_CPU_ID_FLAGS)

      if(cpuSimdFlags)
        if("NEON" IN_LIST cpuSimdFlags)
          if(NOT XII_CMAKE_PLATFORM_OSX)
            # On non-OSX ARM targets, request NEON explicitly.
            target_compile_options(${TARGET_NAME} PRIVATE -mfpu=neon)
          else()
            # On macOS ARM (Apple Silicon), use appropriate flags instead.
            target_compile_options(${TARGET_NAME} PRIVATE -march=armv8-a)
          endif()
        endif()
      else()
        # No detected flags available; fall back to compiler capability check for NEON.
        check_cxx_compiler_flag(-mfpu=neon HAS_NEON)
        if(HAS_NEON AND NOT XII_CMAKE_PLATFORM_OSX)
          target_compile_options(${TARGET_NAME} PRIVATE -mfpu=neon)
        elseif(XII_CMAKE_PLATFORM_OSX)
          target_compile_options(${TARGET_NAME} PRIVATE -march=armv8-a)
        endif()
      endif()
    endif()
  endif()
endfunction()

# #####################################
# ## xii_set_build_flags(<target>)
# #####################################
function(xii_set_build_flags TARGET_NAME)
  xii_pull_compiler_and_architecture_vars()

  set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 23)

  if(XII_CMAKE_COMPILER_MSVC)
    xii_set_build_flags_msvc(${TARGET_NAME} ${ARGN})
  endif()

  if(XII_CMAKE_COMPILER_CLANG)
    xii_set_build_flags_clang(${TARGET_NAME} ${ARGN})
  endif()

  if(XII_CMAKE_COMPILER_GCC)
    xii_set_build_flags_gcc(${TARGET_NAME} ${ARGN})
  endif()

  xii_set_simd_build_flags(${TARGET_NAME})
endfunction()

# #####################################
# ## xii_enable_strict_warnings(<target>)
# #####################################
function(xii_enable_strict_warnings TARGET_NAME)
  if(XII_CMAKE_COMPILER_MSVC)
    get_target_property(TARGET_COMPILE_OPTS ${PROJECT_NAME} COMPILE_OPTIONS)
    list(REMOVE_ITEM TARGET_COMPILE_OPTS /W3) # In case there is W3 already, remove it so it doesn't spam warnings when using Ninja builds.
    list(REMOVE_ITEM TARGET_COMPILE_OPTS /wd4100) # Enable 4100 = unreferenced formal parameter again
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_OPTIONS "${TARGET_COMPILE_OPTS}")

    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  endif()

  if(XII_CMAKE_COMPILER_CLANG)
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wall -Wlogical-op-parentheses)
  endif()
endfunction()

# #####################################
# ## xii_set_clib_build_flags(<target>)
# #####################################
function(xii_set_clib_build_flags TARGET_NAME)
  # Since Clang does not support the C++23 flag on C libraries, ensure to remove the flag (compilation will fail otherwise).
  if(XII_CMAKE_COMPILER_CLANG)
    get_target_property(TARGET_COMPILE_OPTS ${PROJECT_NAME} COMPILE_OPTIONS)
    list(REMOVE_ITEM TARGET_COMPILE_OPTS -std=c++23)
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_OPTIONS "${TARGET_COMPILE_OPTS}")
  endif()
endfunction()
