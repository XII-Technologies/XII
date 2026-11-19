# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_desktop()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})
xii_add_output_xii_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Foundation MCPPlugin)

if(XII_ENABLE_QT_SUPPORT)
  xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)
  target_link_libraries(${PROJECT_NAME} PRIVATE GuiFoundation)
endif()
