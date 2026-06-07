# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  SharedPluginAssets
  ModelImporter
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  TextureConverter
  ShaderCompilerTool
)

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)
xii_link_target_assimp(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} EnginePluginAssets)

xii_copy_plugin_bundle(${PROJECT_NAME} "Assets")
