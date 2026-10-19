# Copyright (c) Theophilus Eriata. All Rights Reserved.

# #####################################
# ## xii_include_xiiExport()
# #####################################

macro(xii_include_xiiExport)
  # Create a modified version of the xiiExport.cmake file,
  # where the absolute paths to the original locations are replaced
  # with the absolute paths to this installation
  xii_get_export_location(EXP_FILE)
  set(IMP_FILE "${CMAKE_BINARY_DIR}/xiiExport.cmake")
  set(EXPINFO_FILE "${XII_OUTPUT_DIRECTORY_DLL}/xiiExportInfo.cmake")

  # read the file that contains the original paths
  include(${EXPINFO_FILE})

  # read the xiiExport file into a string
  file(READ ${EXP_FILE} IMP_CONTENT)

  # replace the original paths with our paths
  string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_DLL} ${XII_OUTPUT_DIRECTORY_DLL} IMP_CONTENT "${IMP_CONTENT}")
  string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_LIB} ${XII_OUTPUT_DIRECTORY_LIB} IMP_CONTENT "${IMP_CONTENT}")
  string(REPLACE ${EXPINP_SOURCE_DIR} ${XII_SDK_DIR} IMP_CONTENT "${IMP_CONTENT}")

  # write the modified xiiExport file to disk
  file(WRITE ${IMP_FILE} "${IMP_CONTENT}")

  # include the modified file, so that the CMake targets become known
  get_property(SDK_IMPORTS_BEFORE DIRECTORY PROPERTY IMPORTED_TARGETS)
  include(${IMP_FILE})
  get_property(SDK_IMPORTS_AFTER DIRECTORY PROPERTY IMPORTED_TARGETS)
  list(REMOVE_ITEM SDK_IMPORTS_AFTER ${SDK_IMPORTS_BEFORE})
  set_property(GLOBAL PROPERTY XII_SDK_RUNTIME_TARGETS ${SDK_IMPORTS_AFTER})
endmacro()

# Standalone project applications need the SDK's runtime libraries and dynamically
# loaded plugins beside them. Only copy into the project; never modify the SDK.
function(xii_stage_external_runtime TARGET_NAME)
  # Use the existing SDK discovery mechanism so older generated applications also
  # continue to find SDK data when their executable lives outside the installation.
  set(SDK_REDIRECTION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}-xiiSdkRoot.txt")
  file(WRITE "${SDK_REDIRECTION_FILE}" "${XII_SDK_DIR}")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDK_REDIRECTION_FILE}" "$<TARGET_FILE_DIR:${TARGET_NAME}>/xiiSdkRoot.txt"
    VERBATIM)

  get_property(SDK_RUNTIME_TARGETS GLOBAL PROPERTY XII_SDK_RUNTIME_TARGETS)
  foreach(SDK_TARGET IN LISTS SDK_RUNTIME_TARGETS)
    get_target_property(SDK_TARGET_TYPE ${SDK_TARGET} TYPE)
    if(SDK_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
      add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${SDK_TARGET}>" "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        VERBATIM)
    endif()
  endforeach()

  # DXC is a private native dependency, so it is not part of the exported target interface.
  if(TARGET ShaderCompilerSPIRV)
    if(WIN32)
      set(DXC_RUNTIME_NAME "dxcompiler.dll")
    else()
      set(DXC_RUNTIME_NAME "libdxcompiler.so")
    endif()
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:ShaderCompilerSPIRV>/${DXC_RUNTIME_NAME}" "$<TARGET_FILE_DIR:${TARGET_NAME}>"
      VERBATIM)
  endif()
endfunction()

# #####################################
# ## xii_configure_external_project()
# #####################################
macro(xii_configure_external_project)

  if (XII_SDK_DIR STREQUAL "")
    file(RELATIVE_PATH XII_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${XII_SDK_DIR})
  else()
    set(XII_SUBMODULE_PREFIX_PATH "")
  endif()

  set_property(GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH ${XII_SUBMODULE_PREFIX_PATH})

  if(XII_SUBMODULE_PREFIX_PATH STREQUAL "")
    set(XII_SUBMODULE_MODE FALSE)
  else()
    set(XII_SUBMODULE_MODE TRUE)
  endif()

  set_property(GLOBAL PROPERTY XII_SUBMODULE_MODE ${XII_SUBMODULE_MODE})

  xii_build_filter_init()

  xii_set_build_types()

  # Import SDK targets before changing these paths. Imported libraries must continue
  # to point into the SDK, while all project-owned build products stay in the project.
  get_filename_component(XII_PROJECT_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../Output" ABSOLUTE)
  set(XII_OUTPUT_DIRECTORY_DLL "${XII_PROJECT_OUTPUT_DIRECTORY}/Bin")
  set(XII_OUTPUT_DIRECTORY_LIB "${XII_PROJECT_OUTPUT_DIRECTORY}/Lib")
  set_property(GLOBAL PROPERTY XII_EXTERNAL_PROJECT_OUTPUT TRUE)
  set(XII_PROJECT_OUTPUT_LAYOUT_VERSION 1 CACHE INTERNAL "Project-local C++ plugin output layout" FORCE)
endmacro()
