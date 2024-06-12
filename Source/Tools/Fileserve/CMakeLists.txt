xii_cmake_init()

xii_requires_qt()

xii_requires_desktop()

xii_requires(XII_3RDPARTY_ENET_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_make_winmain_executable(${PROJECT_NAME})

xii_add_output_xii_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  FileservePlugin
)

xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorPluginFileserve
)
