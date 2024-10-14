xii_cmake_init()

xii_requires(XII_3RDPARTY_ENET_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX ALL_SYMBOLS_VISIBLE)

xii_set_clib_build_flags(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ENET_SUPPORT)

if(XII_CMAKE_PLATFORM_WINDOWS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _WINSOCK_DEPRECATED_NO_WARNINGS)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ws2_32.lib
    Kernel32.lib
    runtimeobject.lib
    winmm.lib
  )

  if(XII_CMAKE_COMPILER_MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4232 /wd4206 /wd4701)
  endif()
endif()
