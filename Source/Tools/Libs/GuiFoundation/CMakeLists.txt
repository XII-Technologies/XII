# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_qt()

xii_requires(XII_3RDPARTY_ADS_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  ads
  Core
)

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg COPY_DLLS)
