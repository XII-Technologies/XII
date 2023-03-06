xii_cmake_init()

xii_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/SampleGame.xiiPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
