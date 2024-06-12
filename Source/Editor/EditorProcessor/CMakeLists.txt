xii_cmake_init()

xii_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_make_winmain_executable(${PROJECT_NAME})

xii_add_output_xii_prefix(${PROJECT_NAME})

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
)

add_dependencies(${PROJECT_NAME}
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  TexConv
)
