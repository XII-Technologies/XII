xii_cmake_init()



xii_requires_editor()

xii_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_include_directories (${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../EditorPlugins/Scene")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  TestFramework
  EditorFramework
  EditorEngineProcessFramework
  RendererDX11
  EditorPluginScene
)

add_dependencies(${PROJECT_NAME}
  EditorProcessor
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  EditorPluginParticle
  EditorPluginProcGen
  ParticlePlugin
  TexConv
  Player
  InspectorPlugin
  ShaderCompiler
)

xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)
