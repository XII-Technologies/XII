xii_cmake_init()

xii_requires(XII_BUILD_VULKAN)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX)

xii_set_clib_build_flags(${PROJECT_NAME})
