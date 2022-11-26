xii_cmake_init()

xii_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  SharedPluginAssets
  ModelImporter2
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  TexConv
  ShaderCompiler
)

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)
xii_link_target_assimp(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} EnginePluginAssets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Assets.xiiPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  )
