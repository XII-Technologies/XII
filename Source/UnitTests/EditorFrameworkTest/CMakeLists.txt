# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_editor()
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})

# Exercise the scene plugin's actual Qt-side adapters without linking its still-migrating engine plugin.
target_sources(${PROJECT_NAME} PRIVATE
  ../../EditorPlugins/Scene/EditorPluginScene/Scene/SceneComponentPropertyMetaState.cpp
  ../../EditorPlugins/Scene/EditorPluginScene/Visualizers/ReflectionCaptureVisualizerAdapter.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC TestFramework EditorFramework SharedPluginScene)
xii_ci_add_test(${PROJECT_NAME})
