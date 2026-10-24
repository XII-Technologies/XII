# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_editor()
xii_requires_renderer()
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})

# Shared, programmatically constructed GPU operation fixtures; no SDK fragment library.
target_sources(${PROJECT_NAME} PRIVATE ../GraphicsCoreTest/Pipeline/Implementation/PipelineTestFixtures.cpp)

# Reuse the backend-neutral GPU fixture rather than duplicate device and shader setup.
target_sources(${PROJECT_NAME} PRIVATE ../GraphicsFoundationTest/TestingEnvironment/Implementation/TestingEnvironment.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC TestFramework EditorEngineProcessFramework SharedPluginScene SharedPluginAssets EnginePluginAssets ModelImporter)
add_dependencies(${PROJECT_NAME} TextureConverter)
xii_add_renderers(${PROJECT_NAME})
xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

set_tests_properties(${PROJECT_NAME} PROPERTIES FIXTURES_REQUIRED RendererAssets)
