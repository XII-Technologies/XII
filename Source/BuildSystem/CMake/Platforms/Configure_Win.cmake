include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Windows")

set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_D3D11 ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)

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

# #####################################
# ## General settings
# #####################################
set(XII_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE XII_COMPILE_ENGINE_AS_DLL)

macro(xii_platform_pull_properties)
  get_property(XII_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS)
  get_property(XII_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY XII_CMAKE_WINDOWS_SDK_VERSION)
endmacro()

macro (xii_platformhook_set_build_flags_clang TARGET_NAME)
  # Disable the warning that clang doesn't support pragma optimize.
  target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
endmacro()

macro(xii_platform_detect_generator)
  string(FIND ${CMAKE_VERSION} "MSVC" VERSION_CONTAINS_MSVC)

  if(${VERSION_CONTAINS_MSVC} GREATER -1)
  message(STATUS "CMake was called from Visual Studio Open Folder workflow")
  set_property(GLOBAL PROPERTY XII_CMAKE_INSIDE_VS ON)
  endif()

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
    message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend xii_platform_detect_generator()")
  endif()
endmacro()

macro(xii_platformhook_find_vulkan)
  if(XII_CMAKE_ARCHITECTURE_64BIT)
    if((XII_VULKAN_DIR STREQUAL "XII_VULKAN_DIR-NOTFOUND") OR (XII_VULKAN_DIR STREQUAL ""))
      unset(XII_VULKAN_DIR CACHE)
      unset(XIIVulkan_DIR CACHE)

      # set(CMAKE_FIND_DEBUG_MODE TRUE)
      find_path(XII_VULKAN_DIR Config/vk_layer_settings.txt PATHS ${XII_VULKAN_DIR} $ENV{VULKAN_SDK})
      # set(CMAKE_FIND_DEBUG_MODE FALSE)
    endif()

    if((XII_VULKAN_DIR STREQUAL "XII_VULKAN_DIR-NOTFOUND") OR (XII_VULKAN_DIR STREQUAL ""))
      unset(XII_VULKAN_DIR CACHE)
      unset(XIIVulkan_DIR CACHE)

      xii_download_and_extract("${XII_CONFIG_VULKAN_SDK_WINDOWSX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${XII_CONFIG_VULKAN_SDK_WINDOWSX64_VERSION}")
      set(XII_VULKAN_DIR "${CMAKE_BINARY_DIR}/vulkan-sdk/${XII_CONFIG_VULKAN_SDK_WINDOWSX64_VERSION}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

      # On windows, the Vulkan SDK is an installer, we need to install the components to the sdk directory.
      set(XII_FULL_VULKAN_INSTALLER_PATH "${CMAKE_BINARY_DIR}/vulkan-sdk/vulkan-sdk-${XII_CONFIG_VULKAN_SDK_WINDOWSX64_VERSION}.exe")
      set(XII_FULL_VULKAN_INSTALL_PATH "${CMAKE_BINARY_DIR}/vulkan-sdk/${XII_CONFIG_VULKAN_SDK_WINDOWSX64_VERSION}")
      # execute_process(COMMAND "{XII_FULL_VULKAN_INSTALLER_PATH}" --root --accept-licenses --default-answer --confirm-command install WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/vulkan-sdk")

      # set(CMAKE_FIND_DEBUG_MODE TRUE)
      find_path(XII_VULKAN_DIR config/vk_layer_settings.txt PATHS ${XII_VULKAN_DIR} $ENV{VULKAN_SDK} REQUIRED)
      # set(CMAKE_FIND_DEBUG_MODE FALSE)

      # TODO: Remove once we have the required vulkan libraries on windows.
      unset(XII_VULKAN_DIR CACHE)
      unset(XIIVulkan_DIR CACHE)
    endif()
  else()
    message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XIIVulkan DEFAULT_MSG XII_VULKAN_DIR)

  if(XIIVULKAN_FOUND)
    if(XII_CMAKE_ARCHITECTURE_64BIT)
      add_library(XIIVulkan::Loader STATIC IMPORTED)
      set_target_properties(XIIVulkan::Loader PROPERTIES IMPORTED_LOCATION "${XII_VULKAN_DIR}/Lib/vulkan-1.lib")
      set_target_properties(XIIVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_VULKAN_DIR}/Include")

      add_library(XIIVulkan::DXC SHARED IMPORTED)
      set_target_properties(XIIVulkan::DXC PROPERTIES IMPORTED_LOCATION "${XII_VULKAN_DIR}/Bin/dxcompiler.dll")
      set_target_properties(XIIVulkan::DXC PROPERTIES IMPORTED_IMPLIB "${XII_VULKAN_DIR}/Lib/dxcompiler.lib")
      set_target_properties(XIIVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_VULKAN_DIR}/Include")
    else()
      message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()
  endif()
endmacro()

macro(xii_platformhook_find_qt)
  if(XII_CMAKE_COMPILER_CLANG)
  # The qt6 interface compile options contain msvc specific flags which don't exist for clang.
  set_target_properties(Qt6::Platform PROPERTIES INTERFACE_COMPILE_OPTIONS "")

  # Qt6 link options include '-NXCOMPAT' which does not exist on clang.
  get_target_property(QtLinkOptions Qt6::PlatformCommonInternal INTERFACE_LINK_OPTIONS)
  string(REPLACE "-NXCOMPAT;" "" QtLinkOptions "${QtLinkOptions}")
  set_target_properties(Qt6::PlatformCommonInternal PROPERTIES INTERFACE_LINK_OPTIONS ${QtLinkOptions})
  endif()
endmacro()

macro(xii_platformhook_download_qt)
  # Currently only implemented for x64
  if(XII_CMAKE_ARCHITECTURE_64BIT)
    # Upgrade from Qt5 to Qt6 if the XII_QT_DIR points to a previously automatically downloaded Qt5 package.
    if("${XII_QT_DIR}" MATCHES ".*Qt-5\\.13\\.0-vs141-x64")
      set(XII_QT_DIR "XII_QT_DIR-NOTFOUND" CACHE PATH "Directory of the Qt installation" FORCE)
    endif()

    if(XII_CMAKE_ARCHITECTURE_64BIT)
      set(XII_SDK_VERSION "${XII_CONFIG_QT_WINX64_VERSION}")
      set(XII_SDK_URL "${XII_CONFIG_QT_WINX64_URL}")
    endif()

    if((XII_QT_DIR STREQUAL "XII_QT_DIR-NOTFOUND") OR(XII_QT_DIR STREQUAL ""))
      xii_download_and_extract("${XII_SDK_URL}" "${CMAKE_BINARY_DIR}" "${XII_SDK_VERSION}")

      set(XII_QT_DIR "${CMAKE_BINARY_DIR}/${XII_SDK_VERSION}" CACHE PATH "Directory of the Qt installation" FORCE)
    endif()
  endif()
endmacro()

macro (xii_platformhook_make_windowapp TARGET_NAME)
  set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
endmacro()
