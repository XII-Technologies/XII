# Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  # This existing NuGet consumer uses the same verified persistent cache as native providers.
  xii_nuget_fetch_package(PACKAGE OpenXR.Headers VERSION 1.0.10.2
    SHA256 f1bd28e0eb0cf6c476e211f3e9201dd480d7cf37be4c8ba75ebba9c60b75a8c8 OUT_PACKAGE_DIR OPENXR_HEADERS)
  xii_nuget_fetch_package(PACKAGE OpenXR.Loader VERSION 1.0.10.2
    SHA256 a9286f2977a207689e6838487c85685317309b7bd1f528655e552c760e335b12
    DEPENDENCIES openxr.headers@1.0.10.2 OUT_PACKAGE_DIR OPENXR_LOADER)
  xii_nuget_fetch_package(PACKAGE Microsoft.Holographic.Remoting.OpenXr VERSION 2.9.4
    SHA256 0efbc12a5b75fe83d6104d153aa3d80f2cacdfecb90bdbc1d76ebb18d22a791c OUT_PACKAGE_DIR OPENXR_REMOTING)
  set(XII_OPENXR_LOADER_DIR "${OPENXR_LOADER}" CACHE PATH "Directory of OpenXR loader installation" FORCE)
  set(XII_OPENXR_HEADERS_DIR "${OPENXR_HEADERS}" CACHE PATH "Directory of OpenXR headers installation" FORCE)
  set(XII_OPENXR_REMOTING_DIR "${OPENXR_REMOTING}" CACHE PATH "Directory of OpenXR remoting installation" FORCE)
endif()

if(XII_CMAKE_PLATFORM_WINDOWS)
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

  if(XII_CMAKE_PLATFORM_WINDOWS AND XII_CMAKE_ARCHITECTURE_64BIT)

    add_library(xiiOpenXR::Remoting INTERFACE IMPORTED)

    list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/RemotingXR.json")
    list(APPEND REMOTING_ASSETS "${XII_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/Microsoft.Holographic.AppRemoting.OpenXr.dll")

    set_target_properties(xiiOpenXR::Remoting PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_OPENXR_REMOTING_DIR}/build/native/include")
    set_target_properties(xiiOpenXR::Remoting PROPERTIES INTERFACE_SOURCES "${REMOTING_ASSETS}")

    set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
    set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "")
  endif()
endif()

unset(OPENXR_DYNAMIC)
unset(OPENXR_BIN_PREFIX)
