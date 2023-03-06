xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)

if (XII_CMAKE_PLATFORM_WINDOWS)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    dxguid.lib
    d3dcompiler.lib
  )
endif()

if (XII_BUILD_DILIGENT)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Diligent-ShaderTools
  )

  #  Add Supported Tools
  if(GL_SUPPORTED OR GLES_SUPPORTED)
    xii_link_target_diligent_opengl(${PROJECT_NAME})
  endif()

  if(D3D11_SUPPORTED)
    xii_link_target_diligent_dx11(${PROJECT_NAME})
  endif()

  if(D3D12_SUPPORTED)
    xii_link_target_diligent_dx12(${PROJECT_NAME})
  endif()

  if(VULKAN_SUPPORTED)
    xii_link_target_diligent_vulkan(${PROJECT_NAME})

    xii_requires_vulkan()
    xii_sources_target_spirv_reflect(${PROJECT_NAME})
  endif()

  if(METAL_SUPPORTED)
    xii_link_target_diligent_metal(${PROJECT_NAME})
  endif()

  # Link SPIRV Cross for Vulkan Shader Reflection
  if(ENABLE_SPIRV)
      target_link_libraries(${PROJECT_NAME}
          PRIVATE
          spirv-cross-core
      )
      if (${USE_SPIRV_TOOLS})
          target_link_libraries(${PROJECT_NAME}
              PRIVATE
              SPIRV-Tools-opt
          )
      endif()

      if (${USE_GLSLANG})
          target_link_libraries(${PROJECT_NAME}
              PRIVATE
              SPIRV
              glslang
          )
      endif()
  endif()
endif()
