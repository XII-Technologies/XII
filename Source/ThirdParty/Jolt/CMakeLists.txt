xii_cmake_init()

xii_requires(XII_3RDPARTY_JOLT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX)

if (XII_COMPILE_ENGINE_AS_DLL)
  target_compile_definitions(${PROJECT_NAME} PUBLIC JPH_SHARED_LIBRARY)
  target_compile_definitions(${PROJECT_NAME} PRIVATE JPH_BUILD_SHARED_LIBRARY)
endif()

xii_pull_config_vars()
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "$<$<CONFIG:${XII_BUILDTYPENAME_DEBUG},${XII_BUILDTYPENAME_DEV}>:JPH_DEBUG_RENDERER>"
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
