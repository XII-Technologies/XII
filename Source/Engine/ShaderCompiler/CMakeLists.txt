xii_cmake_init()

xii_requires_one_of(XII_BUILD_D3D12 XII_BUILD_VULKAN)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GraphicsCore
)

if (XII_BUILD_D3D12)
  # Always link D3D12 first before Vulkan so that the SPIRV Enabled DX Compiler
  # can overwrite the default one from the Windows SDK.
  xii_link_target_d3d12(${PROJECT_NAME})

  # Using the Diligent Library, Copy the DXIL dll.
  if(D3D12_SUPPORTED AND VS_DXIL_SIGNER_PATH)
    # Note that VS_DXC_COMPILER_PATH and VS_DXIL_SIGNER_PATH can only be used in a Visual Studio command
    # and are not valid paths during CMake configuration

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${VS_DXIL_SIGNER_PATH}
              "\"$<TARGET_FILE_DIR:${PROJECT_NAME}>\""
    )
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE
    dxcompiler
  )
endif()

if (XII_BUILD_VULKAN)
  xii_requires_vulkan()
  xii_link_target_dxc(${PROJECT_NAME})
  xii_sources_target_spirv_reflect(${PROJECT_NAME})
endif()
