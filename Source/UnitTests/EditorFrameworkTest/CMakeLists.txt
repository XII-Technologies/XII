# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_editor()
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})

# Shared, programmatically constructed GPU operation fixtures; no SDK fragment library.
target_sources(${PROJECT_NAME} PRIVATE ../GraphicsCoreTest/Pipeline/Implementation/PipelineTestFixtures.cpp)

# Exercise the production scene and pipeline plugins instead of compiling implementation copies.
target_link_libraries(${PROJECT_NAME} PUBLIC TestFramework EditorFramework SharedPluginScene SharedPluginAssets EditorPluginScene EditorPluginAssets EnginePluginAssets)
# Native test widgets need the same Qt Debug RTTI settings as production widgets.
xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)
if(XII_CMAKE_PLATFORM_WINDOWS)
  target_link_libraries(${PROJECT_NAME} PRIVATE Ole32.lib Uuid.lib Shell32.lib RuntimeObject.lib)
endif()
add_dependencies(${PROJECT_NAME} EditorProcessor)
xii_ci_add_test(${PROJECT_NAME})

# Compile the canonical authored assets before GPU suites consume them.
add_test(NAME RendererAssetPreparation
  COMMAND $<TARGET_FILE:EditorFrameworkTest> -noGui -run -close -noSave -all -filter RendererAssets
    -sdkRoot "${XII_ROOT}" -outputDir "${CMAKE_BINARY_DIR}/TestResults/$<CONFIG>/RendererAssets")
set_tests_properties(RendererAssetPreparation PROPERTIES
  FIXTURES_SETUP RendererAssets RUN_SERIAL TRUE TIMEOUT 120 LABELS interactive)
