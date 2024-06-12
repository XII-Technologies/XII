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
  include(${IMP_FILE})
endmacro()

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
endmacro()
