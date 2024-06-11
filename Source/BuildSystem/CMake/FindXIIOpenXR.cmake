# Find the folder into which the OpenXR loader has been installed

# Early out, if this target has been created before
if(TARGET xiiOpenXR::Loader)
  return()
endif()

set(XII_OPENXR_LOADER_DIR "XII_OPENXR_LOADER_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR loader installation")
set(XII_OPENXR_HEADERS_DIR "XII_OPENXR_HEADERS_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR headers installation")
set(XII_OPENXR_PREVIEW_DIR "" CACHE PATH "Directory of OpenXR preview include root")
set(XII_OPENXR_REMOTING_DIR "" CACHE PATH "Directory of OpenXR remoting installation")
mark_as_advanced(FORCE XII_OPENXR_LOADER_DIR)
mark_as_advanced(FORCE XII_OPENXR_HEADERS_DIR)
mark_as_advanced(FORCE XII_OPENXR_PREVIEW_DIR)
mark_as_advanced(FORCE XII_OPENXR_REMOTING_DIR)

xii_pull_compiler_and_architecture_vars()

if((XII_OPENXR_LOADER_DIR STREQUAL "XII_OPENXR_LOADER_DIR-NOTFOUND") OR(XII_OPENXR_LOADER_DIR STREQUAL "") OR(XII_OPENXR_HEADERS_DIR STREQUAL "XII_OPENXR_HEADERS_DIR-NOTFOUND") OR(XII_OPENXR_HEADERS_DIR STREQUAL "") OR(XII_OPENXR_REMOTING_DIR STREQUAL "XII_OPENXR_REMOTING_DIR-NOTFOUND") OR(XII_OPENXR_REMOTING_DIR STREQUAL ""))
  xii_nuget_init()
  execute_process(COMMAND ${NUGET} restore ${CMAKE_SOURCE_DIR}/Source/EnginePlugins/OpenXRPlugin/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set(XII_OPENXR_LOADER_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Loader.1.0.10.2" CACHE PATH "Directory of OpenXR loader installation" FORCE)
  set(XII_OPENXR_HEADERS_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Headers.1.0.10.2" CACHE PATH "Directory of OpenXR headers installation" FORCE)
  set(XII_OPENXR_REMOTING_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Holographic.Remoting.OpenXr.2.4.0" CACHE PATH "Directory of OpenXR remoting installation" FORCE)
endif()

if(XII_CMAKE_PLATFORM_WINDOWS_UWP)
  set(OPENXR_DYNAMIC ON)
  find_path(XII_OPENXR_HEADERS_DIR include/openxr/openxr.h)

  if(XII_CMAKE_ARCHITECTURE_ARM)
    if(XII_CMAKE_ARCHITECTURE_64BIT)
      set(OPENXR_BIN_PREFIX "arm64_uwp")
    else()
      set(OPENXR_BIN_PREFIX "arm_uwp")
    endif()
  else()
    if(XII_CMAKE_ARCHITECTURE_64BIT)
      set(OPENXR_BIN_PREFIX "x64_uwp")
    else()
      set(OPENXR_BIN_PREFIX "Win32_uwp")
    endif()
  endif()

elseif(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  set(OPENXR_DYNAMIC ON)
  find_path(XII_OPENXR_HEADERS_DIR include/openxr/openxr.h)

  if(XII_CMAKE_ARCHITECTURE_64BIT)
    set(OPENXR_BIN_PREFIX "x64")
    find_path(XII_OPENXR_REMOTING_DIR build/native/include/openxr/openxr_msft_holographic_remoting.h)
  else()
    set(OPENXR_BIN_PREFIX "Win32")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(xiiOpenXR DEFAULT_MSG XII_OPENXR_LOADER_DIR)
find_package_handle_standard_args(xiiOpenXR DEFAULT_MSG XII_OPENXR_HEADERS_DIR)
find_package_handle_standard_args(xiiOpenXR DEFAULT_MSG XII_OPENXR_REMOTING_DIR)

if(XIIOPENXR_FOUND)
  add_library(xiiOpenXR::Loader SHARED IMPORTED)

  if(OPENXR_DYNAMIC)
    set_target_properties(xiiOpenXR::Loader PROPERTIES IMPORTED_LOCATION "${XII_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
    set_target_properties(xiiOpenXR::Loader PROPERTIES IMPORTED_LOCATION_DEBUG "${XII_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
  endif()

  set_target_properties(xiiOpenXR::Loader PROPERTIES IMPORTED_IMPLIB "${XII_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")
  set_target_properties(xiiOpenXR::Loader PROPERTIES IMPORTED_IMPLIB_DEBUG "${XII_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")

  set_target_properties(xiiOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_OPENXR_HEADERS_DIR}/include")

  if(NOT XII_OPENXR_PREVIEW_DIR STREQUAL "")
    set_target_properties(xiiOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_OPENXR_HEADERS_DIR}/include")
  endif()

  xii_uwp_mark_import_as_content(xiiOpenXR::Loader)

  if(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP AND XII_CMAKE_ARCHITECTURE_64BIT)

    add_library(xiiOpenXR::Remoting INTERFACE IMPORTED)

    if(XII_CMAKE_PLATFORM_WINDOWS_UWP)
      list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/uwp/RemotingXR.json")
      list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/uwp/Microsoft.Holographic.AppRemoting.OpenXr.dll")
    else()
      list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/RemotingXR.json")
      list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/Microsoft.Holographic.AppRemoting.OpenXr.dll")
    endif()

    set_target_properties(xiiOpenXR::Remoting PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_OPENXR_REMOTING_DIR}/build/native/include")
    set_target_properties(xiiOpenXR::Remoting PROPERTIES INTERFACE_SOURCES "${REMOTING_ASSETS}")

    set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
    set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "")
  endif()
endif()

unset(OPENXR_DYNAMIC)
unset(OPENXR_BIN_PREFIX)
