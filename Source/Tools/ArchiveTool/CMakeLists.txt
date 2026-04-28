# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

# currently the requirements are only met on Windows (ie. file iteration)
xii_requires_desktop()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_add_output_xii_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Foundation
)
