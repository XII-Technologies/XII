xii_cmake_init()

xii_requires_pix_event_runtime()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

add_library(${PROJECT_NAME} IMPORTED STATIC)
set_target_properties(${PROJECT_NAME} PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/WinPixEventRuntime.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT)
