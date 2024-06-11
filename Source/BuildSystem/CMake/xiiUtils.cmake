include(CheckIncludeFileCXX)

file(GLOB UTILS_FILES "${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/*.cmake")

# automatically include all files in the CMakeUtils subfolder
foreach(UTILS_FILE ${UTILS_FILES})
  include("${UTILS_FILE}")
endforeach()

# #####################################
# ## xii_pull_config_vars()
# #####################################
macro(xii_pull_config_vars)
  get_property(XII_BUILDTYPENAME_DEBUG GLOBAL PROPERTY XII_BUILDTYPENAME_DEBUG)
  get_property(XII_BUILDTYPENAME_DEV GLOBAL PROPERTY XII_BUILDTYPENAME_DEV)
  get_property(XII_BUILDTYPENAME_RELEASE GLOBAL PROPERTY XII_BUILDTYPENAME_RELEASE)

  get_property(XII_BUILDTYPENAME_DEBUG_UPPER GLOBAL PROPERTY XII_BUILDTYPENAME_DEBUG_UPPER)
  get_property(XII_BUILDTYPENAME_DEV_UPPER GLOBAL PROPERTY XII_BUILDTYPENAME_DEV_UPPER)
  get_property(XII_BUILDTYPENAME_RELEASE_UPPER GLOBAL PROPERTY XII_BUILDTYPENAME_RELEASE_UPPER)

  get_property(XII_DEV_BUILD_LINKERFLAGS GLOBAL PROPERTY XII_DEV_BUILD_LINKERFLAGS)

  get_property(XII_CMAKE_RELPATH GLOBAL PROPERTY XII_CMAKE_RELPATH)
  get_property(XII_CMAKE_RELPATH_CODE GLOBAL PROPERTY XII_CMAKE_RELPATH_CODE)
  get_property(XII_CONFIG_PATH_7ZA GLOBAL PROPERTY XII_CONFIG_PATH_7ZA)

  get_property(XII_CONFIG_QT_WINX64_URL GLOBAL PROPERTY XII_CONFIG_QT_WINX64_URL)
  get_property(XII_CONFIG_QT_WINX64_VERSION GLOBAL PROPERTY XII_CONFIG_QT_WINX64_VERSION)

  get_property(XII_CONFIG_VULKAN_SDK_LINUXX64_VERSION GLOBAL PROPERTY XII_CONFIG_VULKAN_SDK_LINUXX64_VERSION)
  get_property(XII_CONFIG_VULKAN_SDK_LINUXX64_URL GLOBAL PROPERTY XII_CONFIG_VULKAN_SDK_LINUXX64_URL)
endmacro()

# #####################################
# ## xii_pull_output_vars(LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
# #####################################
macro(xii_pull_output_vars LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
  xii_pull_all_vars()
  xii_pull_config_vars()

  set(SUB_DIR "")
  set(PLATFORM_PREFIX "")
  set(PLATFORM_POSTFIX "")
  set(ARCH "x${XII_CMAKE_ARCHITECTURE_POSTFIX}")

  if(XII_CMAKE_PLATFORM_WINDOWS_UWP)
    # UWP has deployment problems if all applications output to the same path.
    set(SUB_DIR "/${TARGET_NAME}")
    set(PLATFORM_PREFIX "uwp_")

    if(${ARCH} STREQUAL "x32")
      set(ARCH "x86")
    endif()

    if(${ARCH} STREQUAL "xArm32")
      set(ARCH "arm")
    endif()

    if(${ARCH} STREQUAL "xArm64")
      set(ARCH "arm64")
    endif()

  elseif(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    set(PLATFORM_POSTFIX "_win10")

  elseif(XII_CMAKE_PLATFORM_EMSCRIPTEN)
    set(PLATFORM_POSTFIX "_wasm")

  elseif(XII_CMAKE_PLATFORM_ANDROID)
    set(PLATFORM_POSTFIX "_android")
  endif()

  string(TOLOWER ${XII_CMAKE_GENERATOR_PREFIX} LOWER_GENERATOR_PREFIX)

  set(PRE_PATH "${XII_CMAKE_PLATFORM_PREFIX}${XII_CMAKE_GENERATOR_PREFIX}${XII_CMAKE_COMPILER_POSTFIX}")
  set(OUTPUT_DEBUG "${PRE_PATH}${XII_BUILDTYPENAME_DEBUG}${XII_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
  set(OUTPUT_RELEASE "${PRE_PATH}${XII_BUILDTYPENAME_RELEASE}${XII_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
  set(OUTPUT_DEV "${PRE_PATH}${XII_BUILDTYPENAME_DEV}${XII_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")

  set(OUTPUT_DLL_DEBUG "${DLL_OUTPUT_DIR}/${OUTPUT_DEBUG}")
  set(OUTPUT_LIB_DEBUG "${LIB_OUTPUT_DIR}/${OUTPUT_DEBUG}")

  set(OUTPUT_DLL_RELEASE "${DLL_OUTPUT_DIR}/${OUTPUT_RELEASE}")
  set(OUTPUT_LIB_RELEASE "${LIB_OUTPUT_DIR}/${OUTPUT_RELEASE}")

  set(OUTPUT_DLL_DEV "${DLL_OUTPUT_DIR}/${OUTPUT_DEV}")
  set(OUTPUT_LIB_DEV "${LIB_OUTPUT_DIR}/${OUTPUT_DEV}")
endmacro()

# #####################################
# ## xii_set_target_output_dirs(<target> <lib-output-dir> <dll-output-dir>)
# #####################################
function(xii_set_target_output_dirs TARGET_NAME LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
  if(XII_DO_NOT_SET_OUTPUT_DIRS)
    return()
  endif()

  xii_pull_output_vars("${LIB_OUTPUT_DIR}" "${DLL_OUTPUT_DIR}")

  # If we can't use generator expressions the non-generator expression version of the
  # output directory should point to the version matching CMAKE_BUILD_TYPE. This is the case for
  # add_custom_command BYPRODUCTS for example needed by Ninja.
  if("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_DEBUG})
    set_target_properties(${TARGET_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
      LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
      ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
    )
  elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_RELEASE})
    set_target_properties(${TARGET_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
      LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
      ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_RELEASE}"
    )
  elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_DEV})
    set_target_properties(${TARGET_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
      LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
      ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEV}"
    )
  else()
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
    LIBRARY_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
    ARCHIVE_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_LIB_DEBUG}"
  )

  set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
    LIBRARY_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
    ARCHIVE_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_LIB_RELEASE}"
  )

  set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
    LIBRARY_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
    ARCHIVE_OUTPUT_DIRECTORY_${XII_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_LIB_DEV}"
  )
endfunction()

# #####################################
# ## xii_set_default_target_output_dirs(<target>)
# #####################################
function(xii_set_default_target_output_dirs TARGET_NAME)
  xii_set_target_output_dirs("${TARGET_NAME}" "${XII_OUTPUT_DIRECTORY_LIB}" "${XII_OUTPUT_DIRECTORY_DLL}")
endfunction()

# #####################################
# ## xii_write_configuration_txt()
# #####################################
function(xii_write_configuration_txt)
  if(XII_NO_TXT_FILES)
    return()
  endif()

  # Clear Targets.txt and Tests.txt
  file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
  file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

  xii_pull_all_vars()
  xii_pull_config_vars()

  # Write configuration to file, as this is done at configure time we must pin the configuration in place (Dev is used because all build machines use this).
  file(WRITE ${CMAKE_BINARY_DIR}/Configuration.txt "")
  set(CONFIGURATION_DESC "${XII_CMAKE_PLATFORM_PREFIX}${XII_CMAKE_GENERATOR_PREFIX}${XII_CMAKE_COMPILER_POSTFIX}${XII_BUILDTYPENAME_DEV}${XII_CMAKE_ARCHITECTURE_POSTFIX}")
  file(APPEND ${CMAKE_BINARY_DIR}/Configuration.txt ${CONFIGURATION_DESC})
endfunction()

# #####################################
# ## xii_add_target_folder_as_include_dir(<target> <path-to-target>)
# #####################################
function(xii_add_target_folder_as_include_dir TARGET_NAME TARGET_FOLDER)
  get_filename_component(PARENT_DIR ${TARGET_FOLDER} DIRECTORY)

  # target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_FOLDER}")
  target_include_directories(${TARGET_NAME} PUBLIC "${PARENT_DIR}")
endfunction()

# #####################################
# ## xii_set_common_target_definitions(<target>)
# #####################################
function(xii_set_common_target_definitions TARGET_NAME)
  xii_pull_all_vars()
  xii_pull_config_vars()

  # set the BUILDSYSTEM_COMPILE_ENGINE_AS_DLL definition
  if(XII_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_COMPILE_ENGINE_AS_DLL)
  endif()

  target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MAJOR=${XII_CMAKE_SDKVERSION_MAJOR})
  target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MINOR=${XII_CMAKE_SDKVERSION_MINOR})
  target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_PATCH=${XII_CMAKE_SDKVERSION_PATCH})

  set(ORIGINAL_BUILD_TYPE "$<IF:$<STREQUAL:${XII_CMAKE_GENERATOR_CONFIGURATION},${XII_BUILDTYPENAME_DEBUG}>,Debug,$<IF:$<STREQUAL:${XII_CMAKE_GENERATOR_CONFIGURATION},${XII_BUILDTYPENAME_DEV}>,Dev,Shipping>>")

  # set the BUILDSYSTEM_BUILDTYPE definition
  target_compile_definitions(${TARGET_NAME} PRIVATE "BUILDSYSTEM_BUILDTYPE=\"${ORIGINAL_BUILD_TYPE}\"")
  target_compile_definitions(${TARGET_NAME} PUBLIC "BUILDSYSTEM_BUILDTYPE_${ORIGINAL_BUILD_TYPE}")

  # set the BUILDSYSTEM_BUILDING_XYZ_LIB definition
  string(TOUPPER ${TARGET_NAME} PROJECT_NAME_UPPER)
  target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDING_${PROJECT_NAME_UPPER}_LIB)

  if (XII_BUILD_D3D11)
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_D3D11_SUPPORT)
  endif()

  if (XII_BUILD_D3D12)
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_D3D12_SUPPORT)
  endif()

  if (XII_BUILD_VULKAN)
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_VULKAN_SUPPORT)
  endif()

  # On Windows, make sure to use the Unicode API
  target_compile_definitions(${TARGET_NAME} PUBLIC UNICODE _UNICODE)
endfunction()

# #####################################
# ## xii_set_project_ide_folder(<target> <path-to-target>)
# #####################################
function(xii_set_project_ide_folder TARGET_NAME PROJECT_SOURCE_DIR)
  # globally enable sorting targets into folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  get_filename_component(PARENT_FOLDER ${PROJECT_SOURCE_DIR} PATH)
  get_filename_component(FOLDER_NAME ${PARENT_FOLDER} NAME)

  set(IDE_FOLDER "${FOLDER_NAME}")

  set(CMAKE_SOURCE_DIR_PREFIX "${CMAKE_SOURCE_DIR}/")
  cmake_path(IS_PREFIX CMAKE_SOURCE_DIR_PREFIX ${PROJECT_SOURCE_DIR} NORMALIZE FOLDER_IN_TREE)
  if(FOLDER_IN_TREE)
    set(IDE_FOLDER "")
    string(REPLACE ${CMAKE_SOURCE_DIR_PREFIX} "" PARENT_FOLDER ${PROJECT_SOURCE_DIR})

    get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
    get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

    get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)

    while(NOT ${PARENT_FOLDER2} STREQUAL "")
      set(IDE_FOLDER "${FOLDER_NAME}/${IDE_FOLDER}")

      get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
      get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

      get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)
    endwhile()
  endif()

  get_property(XII_SUBMODULE_MODE GLOBAL PROPERTY XII_SUBMODULE_MODE)

  if(XII_SUBMODULE_MODE)
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "XII/${IDE_FOLDER}")
  else()
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${IDE_FOLDER})
  endif()
endfunction()

# #####################################
# ## xii_add_output_xii_prefix(<target>)
# #####################################
function(xii_add_output_xii_prefix TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORT_PREFIX "xii")
  set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "xii")
endfunction()

# #####################################
# ## xii_set_library_properties(<target>)
# #####################################
function(xii_set_library_properties TARGET_NAME)
  xii_pull_all_vars()

  if(XII_CMAKE_PLATFORM_LINUX)
    # c = libc.so (the C standard library)
    # m = libm.so (the C standard library math portion)
    # pthread = libpthread.so (thread support)
    # rt = librt.so (compiler runtime functions)
    target_link_libraries(${TARGET_NAME} PRIVATE pthread rt c m)

    if(XII_CMAKE_COMPILER_GCC)
      # Workaround for: https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
      target_link_libraries(${TARGET_NAME} PRIVATE -lgcc_s -lgcc)
    endif()
  endif()
endfunction()

# #####################################
# ## xii_set_application_properties(<target>)
# #####################################
function(xii_set_application_properties TARGET_NAME)
  xii_pull_all_vars()

  # We need to link against pthread and rt last or linker errors will occur.
  if(XII_CMAKE_PLATFORM_LINUX)
    target_link_libraries(${TARGET_NAME} PRIVATE pthread rt)
  endif()
endfunction()

# #####################################
# ## xii_make_winmain_executable(<target>)
# #####################################
function(xii_make_winmain_executable TARGET_NAME)
  set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
endfunction()

# #####################################
# ## xii_gather_subfolders(<abs-path-to-folder> <out-sub-folders>)
# #####################################
function(xii_gather_subfolders START_FOLDER RESULT_FOLDERS)
  set(ALL_FILES "")
  set(ALL_DIRS "")

  file(GLOB_RECURSE ALL_FILES RELATIVE "${START_FOLDER}" "${START_FOLDER}/*")

  foreach(FILE ${ALL_FILES})
    get_filename_component(FILE_PATH ${FILE} DIRECTORY)

    list(APPEND ALL_DIRS ${FILE_PATH})
  endforeach()

  list(REMOVE_DUPLICATES ALL_DIRS)

  set(${RESULT_FOLDERS} ${ALL_DIRS} PARENT_SCOPE)
endfunction()

# #####################################
# ## xii_glob_source_files(<path-to-folder> <out-files>)
# #####################################
function(xii_glob_source_files ROOT_DIR RESULT_ALL_SOURCES)
  file(GLOB_RECURSE RELEVANT_FILES
    "${ROOT_DIR}/*.cpp"
    "${ROOT_DIR}/*.cxx"
    "${ROOT_DIR}/*.cc"
    "${ROOT_DIR}/*.h"
    "${ROOT_DIR}/*.hpp"
    "${ROOT_DIR}/*.inl"
    "${ROOT_DIR}/*.c"
    "${ROOT_DIR}/*.cs"
    "${ROOT_DIR}/*.ui"
    "${ROOT_DIR}/*.qrc"
    "${ROOT_DIR}/*.def"
    "${ROOT_DIR}/*.ico"
    "${ROOT_DIR}/*.rc"
    "${ROOT_DIR}/*.s"
    "${ROOT_DIR}/*.cmake"
    "${ROOT_DIR}/*.natvis"
    "${ROOT_DIR}/*.txt"
    "${ROOT_DIR}/*.ddl"
    "${ROOT_DIR}/*.xiiPermVar"
    "${ROOT_DIR}/*.xiiShader"
    "${ROOT_DIR}/*.xiiShaderTemplate"
    "${ROOT_DIR}/*.rml"
    "${ROOT_DIR}/*.rcss"
  )

  set(${RESULT_ALL_SOURCES} ${RELEVANT_FILES} PARENT_SCOPE)
endfunction()

# #####################################
# ## xii_add_all_subdirs()
# #####################################
function(xii_add_all_subdirs)
  # find all cmake files below this directory
  file(GLOB SUB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

  foreach(VAR ${SUB_DIRS})
    get_filename_component(RES ${VAR} DIRECTORY)

    add_subdirectory(${RES})
  endforeach()
endfunction()

# #####################################
# ## xii_cmake_init()
# #####################################
macro(xii_cmake_init)
  xii_pull_all_vars()
endmacro()

# #####################################
# ## xii_requires(<variable>)
# #####################################
macro(xii_requires)
  if(${ARGC} EQUAL 0)
    return()
  endif()

  set(ALL_ARGS "${ARGN}")

  foreach(arg IN LISTS ALL_ARGS)
    if(NOT ${arg})
      return()
    endif()
  endforeach()
endmacro()

# #####################################
# ## xii_requires_one_of(<variable1> (<variable2>) (<variable3>) ...)
# #####################################
macro(xii_requires_one_of)
  if(${ARGC} EQUAL 0)
    message(FATAL_ERROR "xii_requires_one_of needs at least one argument")
  endif()

  set(ALL_ARGS "${ARGN}")

  set(VALID 0)

  foreach(arg IN LISTS ALL_ARGS)
    if(${arg})
      set(VALID 1)
    endif()
  endforeach()

  if(NOT VALID)
    return()
  endif()
endmacro()

# #####################################
# ## xii_requires_windows()
# #####################################
macro(xii_requires_windows)
  xii_requires(XII_CMAKE_PLATFORM_WINDOWS)
endmacro()

# #####################################
# ## xii_requires_windows_desktop()
# #####################################
macro(xii_requires_windows_desktop)
  xii_requires(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)
endmacro()

# #####################################
# ## xii_requires_desktop()
# #####################################
macro(xii_requires_desktop)
  xii_requires_one_of(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP XII_CMAKE_PLATFORM_LINUX)
endmacro()

# #####################################
# ## xii_requires_editor()
# #####################################
macro(xii_requires_editor)
  xii_requires_qt()
  xii_requires_renderer()
  if(XII_CMAKE_PLATFORM_LINUX)
    xii_requires(XII_EXPERIMENTAL_EDITOR_ON_LINUX)
  endif()
endmacro()

# #####################################
# ## xii_add_external_folder(<project-number>)
# #####################################
function(xii_add_external_projects_folder PROJECT_NUMBER)
  set(CACHE_VAR_NAME "XII_EXTERNAL_PROJECT${PROJECT_NUMBER}")

  set(${CACHE_VAR_NAME} "" CACHE PATH "A folder outside the xii repository that should be parsed for CMakeLists.txt files to include projects into the xii solution.")

  set(CACHE_VAR_VALUE ${${CACHE_VAR_NAME}})

  if(NOT CACHE_VAR_VALUE)
    return()
  endif()

  set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" TRUE)
  add_subdirectory(${CACHE_VAR_VALUE} "${CMAKE_BINARY_DIR}/ExternalProject${PROJECT_NUMBER}")
  set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" FALSE)
endfunction()

# #####################################
# ## xii_init_projects()
# #####################################
# By defining XII_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(xii_init_projects)
  # find all init.cmake files below this directory or the given source directory if any.
  if(XII_SOURCE_DIR)
    file(GLOB_RECURSE INIT_FILES "${XII_SOURCE_DIR}/init.cmake")
  else()
    file(GLOB_RECURSE INIT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/init.cmake")
  endif()

  foreach(INIT_FILE ${INIT_FILES})
    message(STATUS "Including '${INIT_FILE}'")
    include("${INIT_FILE}")
  endforeach()
endfunction()

# #####################################
# ## xii_finalize_projects()
# #####################################
# By defining XII_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(xii_finalize_projects)
  # find all finalize.cmake files below this directory or the given source directory if any.
  if(XII_SOURCE_DIR)
    file(GLOB_RECURSE FINALIZE_FILES "${XII_SOURCE_DIR}/finalize.cmake")
  else()
    file(GLOB_RECURSE FINALIZE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/finalize.cmake")
  endif()

  # TODO: also finalize external projects
  foreach(FINALIZE_FILE ${FINALIZE_FILES})
    message(STATUS "Including '${FINALIZE_FILE}'")
    include("${FINALIZE_FILE}")
  endforeach()
endfunction()

# #####################################
# ## xii_build_filter_init()
# #####################################

# The build filter is intended to only build a subset of xiiEngine.
# The build filters are configured through cmake files in the 'BuildFilters' directory.
function(xii_build_filter_init)
  file(GLOB_RECURSE FILTER_FILES "${XII_ROOT}/Source/BuildSystem/CMake/BuildFilters/*.BuildFilter")

  get_property(XII_BUILD_FILTER_NAMES GLOBAL PROPERTY XII_BUILD_FILTER_NAMES)

  foreach(VAR ${FILTER_FILES})
    cmake_path(GET VAR STEM FILTER_NAME)
    list(APPEND XII_BUILD_FILTER_NAMES "${FILTER_NAME}")

    message(STATUS "Reading build filter '${FILTER_NAME}'")
    include(${VAR})
  endforeach()

  list(REMOVE_DUPLICATES XII_BUILD_FILTER_NAMES)
  set_property(GLOBAL PROPERTY XII_BUILD_FILTER_NAMES ${XII_BUILD_FILTER_NAMES})

  set(XII_BUILD_FILTER "Everything" CACHE STRING "Which projects to include in the solution.")

  get_property(XII_BUILD_FILTER_NAMES GLOBAL PROPERTY XII_BUILD_FILTER_NAMES)
  set_property(CACHE XII_BUILD_FILTER PROPERTY STRINGS ${XII_BUILD_FILTER_NAMES})
  set_property(GLOBAL PROPERTY XII_BUILD_FILTER_SELECTED ${XII_BUILD_FILTER})
endfunction()

# #####################################
# ## xii_project_build_filter_index(<PROJECT_NAME> <OUT_INDEX>)
# #####################################
function(xii_project_build_filter_index PROJECT_NAME OUT_INDEX)
  get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY XII_BUILD_FILTER_SELECTED)
  set(FILTER_VAR_NAME "XII_BUILD_FILTER_${SELECTED_FILTER_NAME}")
  get_property(FILTER_PROJECTS GLOBAL PROPERTY ${FILTER_VAR_NAME})

  list(LENGTH FILTER_PROJECTS LIST_LENGTH)

  if(${LIST_LENGTH} GREATER 1)
    list(FIND FILTER_PROJECTS ${PROJECT_NAME} FOUND_INDEX)
    set(${OUT_INDEX} ${FOUND_INDEX} PARENT_SCOPE)
  else()
    set(${OUT_INDEX} 0 PARENT_SCOPE)
  endif()
endfunction()

# #####################################
# ## xii_apply_build_filter(<PROJECT_NAME>)
# #####################################
macro(xii_apply_build_filter PROJECT_NAME)
  xii_project_build_filter_index(${PROJECT_NAME} PROJECT_INDEX)

  if(${PROJECT_INDEX} EQUAL -1)
    get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY XII_BUILD_FILTER_SELECTED)
    message(STATUS "Project '${PROJECT_NAME}' excluded by build filter '${SELECTED_FILTER_NAME}'.")
    return()
  endif()
endmacro()

# #####################################
# ## xii_set_build_types()
# #####################################
function(xii_set_build_types)
  xii_pull_config_vars()

  set(CMAKE_CONFIGURATION_TYPES "${XII_BUILDTYPENAME_DEBUG};${XII_BUILDTYPENAME_DEV};${XII_BUILDTYPENAME_RELEASE}" CACHE STRING "" FORCE)

    if (XII_BUILDTYPE_ONLY)
    set(CMAKE_CONFIGURATION_TYPES "${XII_BUILDTYPE_ONLY}" CACHE STRING "" FORCE)
  endif()

  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "XII build config types" FORCE)

  set(CMAKE_BUILD_TYPE ${XII_BUILDTYPENAME_DEV} CACHE STRING "The default build type")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})

  set(CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

  # Fix for cl : Command line warning D9025 : overriding '/Ob0' with '/Ob1'
  # We are adding /Ob1 to debug inside ./CMakeUtils/xiiUtilsCppFlags.cmake
  if(XII_CMAKE_COMPILER_GCC)
    string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/Ob0" "/Ob1" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  endif ()

  set(CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CXX_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_C_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_C_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CSharp_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CSharp_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CSharp_FLAGS_RELEASE} CACHE STRING "" FORCE)

  set(CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_RC_FLAGS_DEBUG} CACHE STRING "" FORCE)
  set(CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER} ${CMAKE_RC_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
  set(CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_RC_FLAGS_RELEASE} CACHE STRING "" FORCE)

  mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_C_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_C_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_C_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
  mark_as_advanced(FORCE CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_DEBUG_UPPER})
  mark_as_advanced(FORCE CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_DEV_UPPER})
  mark_as_advanced(FORCE CMAKE_RC_FLAGS_${XII_BUILDTYPENAME_RELEASE_UPPER})
endfunction()

# #####################################
# ## xii_download_and_extract(<url-to-download> <dest-folder-path> <dest-filename-without-extension>)
# #####################################
function(xii_download_and_extract URL DEST_FOLDER DEST_FILENAME)
  if(${URL} MATCHES ".tar.gz$")
    set(PKG_TYPE "tar.gz")
  elseif(${URL} MATCHES ".tar.xz$")
    set(PKG_TYPE "tar.xz")
  else()
    get_filename_component(PKG_TYPE ${URL} LAST_EXT)
  endif()

  set(FULL_FILENAME "${DEST_FILENAME}.${PKG_TYPE}")
  set(PKG_FILE "${DEST_FOLDER}/${FULL_FILENAME}")
  set(EXTRACT_MARKER "${PKG_FILE}.extracted")

  if(EXISTS "${EXTRACT_MARKER}")
    return()
  endif()

  # if the "URL" is actually a file path
  if(NOT "${URL}" MATCHES "http*")
    set(PKG_FILE "${URL}")
  endif()

  if(NOT EXISTS "${PKG_FILE}")
    message(STATUS "Downloading '${FULL_FILENAME}'...")
    file(DOWNLOAD ${URL} "${PKG_FILE}" SHOW_PROGRESS STATUS DOWNLOAD_STATUS)

    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)

    if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
      message(FATAL_ERROR "Download failed: ${DOWNLOAD_STATUS}")
      return()
    endif()
  endif()

  xii_pull_config_vars()

  message(STATUS "Extracting '${FULL_FILENAME}'...")

  if(${PKG_TYPE} MATCHES "7z")
    set(FULL_7ZA_PATH "${XII_ROOT}/${XII_CONFIG_PATH_7ZA}")
    execute_process(COMMAND "${FULL_7ZA_PATH}"
      x "${PKG_FILE}"
      -aoa
      WORKING_DIRECTORY "${DEST_FOLDER}"
      COMMAND_ERROR_IS_FATAL ANY
      RESULT_VARIABLE CMD_STATUS)

  else()
    execute_process(COMMAND ${CMAKE_COMMAND}
      -E tar -xf "${PKG_FILE}"
      WORKING_DIRECTORY "${DEST_FOLDER}"
      COMMAND_ERROR_IS_FATAL ANY
      RESULT_VARIABLE CMD_STATUS)
  endif()

  if(NOT CMD_STATUS EQUAL 0)
    message(FATAL_ERROR "Extracting package '${FULL_FILENAME}' failed.")
    return()
  endif()

  file(TOUCH ${EXTRACT_MARKER})
endfunction()

function(xii_get_export_location DST_VAR)
  xii_pull_config_vars()
  xii_pull_output_vars("" "${XII_OUTPUT_DIRECTORY_DLL}")

  if(GENERATOR_IS_MULTI_CONFIG OR (CMAKE_GENERATOR MATCHES "Visual Studio"))
    set("${DST_VAR}" "${XII_OUTPUT_DIRECTORY_DLL}/xiiExport.cmake" PARENT_SCOPE)
  else()
    if("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_DEBUG})
      set("${DST_VAR}" "${XII_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEBUG}/xiiExport.cmake" PARENT_SCOPE)
    elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_RELEASE})
      set("${DST_VAR}" "${XII_OUTPUT_DIRECTORY_DLL}/${OUTPUT_RELEASE}/xiiExport.cmake" PARENT_SCOPE)
    elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${XII_BUILDTYPENAME_DEV})
      set("${DST_VAR}" "${XII_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEV}/xiiExport.cmake" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
    endif()
  endif()
endfunction()
