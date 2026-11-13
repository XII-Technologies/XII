# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_editor()

xii_requires(XII_3RDPARTY_JOLT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

# EditorFramework's inline virtual hooks intentionally leave parameters unused.
if(XII_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4100)
endif()

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
  GraphicsCore
  JoltPlugin
  EditorFramework
  EditorPluginAssets
  EditorPluginScene
  EnginePluginJolt
  ModelImporter
)

add_dependencies(${PROJECT_NAME}
  EnginePluginJolt
)
