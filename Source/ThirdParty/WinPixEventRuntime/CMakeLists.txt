xii_cmake_init()

xii_requires_pix_event_runtime()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

add_library(${PROJECT_NAME} IMPORTED STATIC GLOBAL)
set_target_properties(${PROJECT_NAME} PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/WinPixEventRuntime.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
  RUNTIME_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
)
