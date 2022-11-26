xii_cmake_init()

xii_requires(XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})