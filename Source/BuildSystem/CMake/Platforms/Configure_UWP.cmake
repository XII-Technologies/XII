include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: UWP")

set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_UWP ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_D3D11 ON)
set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_SUPPORTS_D3D12 ON)

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
  get_property(XII_CMAKE_PLATFORM_WINDOWS_UWP GLOBAL PROPERTY XII_CMAKE_PLATFORM_WINDOWS_UWP)
  get_property(XII_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY XII_CMAKE_WINDOWS_SDK_VERSION)
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
  else()
    message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on UWP! Please extend xii_platform_detect_generator()")
  endif()
endmacro()
