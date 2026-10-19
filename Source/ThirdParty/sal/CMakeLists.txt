xii_cmake_init()

xii_requires_one_of(XII_CMAKE_PLATFORM_LINUX XII_CMAKE_PLATFORM_OSX)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

add_library(${PROJECT_NAME} INTERFACE IMPORTED GLOBAL)

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
