# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_qt()
xii_requires_desktop()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(LIBRARY ${PROJECT_NAME})
xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
  EditorFramework
  MCPPlugin
)
