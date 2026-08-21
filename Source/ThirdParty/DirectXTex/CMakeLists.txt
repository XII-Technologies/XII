xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Compiled")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  DirectX-Headers
  DirectXMath
)

if(XII_CMAKE_PLATFORM_LINUX)
  target_link_libraries(${PROJECT_NAME} PRIVATE sal)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)

if(XII_CMAKE_PLATFORM_LINUX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDSYSTEM_DISABLE_DIRECTX_COMPONENTS)
endif()
