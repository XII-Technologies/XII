# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_make_windowapp(${PROJECT_NAME})

xii_add_output_xii_prefix(${PROJECT_NAME})

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  EditorEngineProcessFramework
)

add_dependencies(${PROJECT_NAME}
  EditorProcessor
  EditorEngineProcess
  Player
)

# For SetProcessDpiAwareness
if (XII_CMAKE_PLATFORM_WINDOWS)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Shcore.lib
  )
endif()

set(EDITOR_DATA_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Tools/xiiEditor")
file(GLOB_RECURSE EDITOR_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Tools/xiiEditor/*")

source_group(TREE ${EDITOR_DATA_FOLDER} PREFIX "Editor Data" FILES ${EDITOR_DATA})
target_sources(${PROJECT_NAME} PRIVATE "${EDITOR_DATA}")

set_source_files_properties(${EDITOR_DATA} PROPERTIES HEADER_FILE_ONLY TRUE)
