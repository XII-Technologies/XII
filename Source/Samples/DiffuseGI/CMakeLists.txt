# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()
xii_requires_renderer()

xii_create_target(APPLICATION DiffuseGI)
xii_add_renderers(DiffuseGI)
target_link_libraries(DiffuseGI PRIVATE GraphicsCore)
if(TARGET EditorEngineProcessFramework AND TARGET SharedPluginScene)
  target_compile_definitions(DiffuseGI PRIVATE XII_DIFFUSE_GI_EDITOR)
  target_link_libraries(DiffuseGI PRIVATE EditorEngineProcessFramework SharedPluginScene)
endif()
