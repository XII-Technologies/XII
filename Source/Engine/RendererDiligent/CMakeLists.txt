xii_cmake_init()

xii_requires_diligent()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_link_target_diligent_d3d11(${PROJECT_NAME})
xii_link_target_diligent_d3d12(${PROJECT_NAME})
xii_link_target_diligent_vulkan(${PROJECT_NAME})
xii_link_target_diligent_metal(${PROJECT_NAME})
xii_link_target_diligent_opengl(${PROJECT_NAME})

if (XII_BUILD_D3D11)
  xii_link_target_d3d11(${PROJECT_NAME})
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Diligent-Common
    Diligent-BuildSettings
    Diligent-TargetPlatform
    Diligent-GraphicsEngine
    Diligent-GraphicsEngineD3DBase
    Diligent-GraphicsEngineD3D11-static
  )
endif()

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererFoundation
  ShaderCompiler
)
