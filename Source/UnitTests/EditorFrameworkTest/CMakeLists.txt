# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_editor()
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})

# Exercise the production scene plugin instead of compiling copies of selected implementation units.
target_link_libraries(${PROJECT_NAME} PUBLIC TestFramework EditorFramework SharedPluginScene EditorPluginScene EditorPluginAssets)
xii_ci_add_test(${PROJECT_NAME})
