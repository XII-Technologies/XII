xii_cmake_init()

xii_requires(XII_3RDPARTY_ZLIB_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

if(XII_CMAKE_PLATFORM_WINDOWS_UWP OR XII_CMAKE_PLATFORM_POSIX)
  xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_WARNINGS_AS_ERRORS)
else()
  xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_WARNINGS_AS_ERRORS)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZLIB_SUPPORT)

if(XII_CMAKE_PLATFORM_WINDOWS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _CRT_SECURE_NO_WARNINGS)

  if(NOT XII_CMAKE_PLATFORM_WINDOWS_UWP AND XII_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ZLIB_DLL=1)
  endif()

  target_compile_options(${PROJECT_NAME} PRIVATE /wd4131 /wd4996)
endif()
