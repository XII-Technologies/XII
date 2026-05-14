xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Compiled")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  DirectX-Headers
)

target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
