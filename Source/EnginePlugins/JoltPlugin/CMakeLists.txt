# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires(XII_3RDPARTY_JOLT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  PRIVATE
  Jolt
)

xii_copy_plugin_bundle(${PROJECT_NAME} "Jolt")
