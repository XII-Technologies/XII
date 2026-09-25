# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsCore
  TestFramework
)

xii_ci_add_test(${PROJECT_NAME})
