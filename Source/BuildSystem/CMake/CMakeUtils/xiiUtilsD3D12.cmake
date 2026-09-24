# Copyright (c) Theophilus Eriata. All Rights Reserved.

# #####################################
# ## DirectX 12 support
# #####################################
# D3D12 is intentionally held out while the descriptor-indexing and GPU-scene contracts are
# implemented and validated on Vulkan. FORCE also clears stale developer cache values so a
# previously enabled backend cannot enter the build accidentally.
set(XII_BUILD_D3D12 OFF CACHE BOOL "Build the DirectX 12 Graphics Device" FORCE)

# #####################################
# ## xii_requires_d3d12()
# #####################################
macro(xii_requires_d3d12)
  xii_requires(XII_CMAKE_PLATFORM_SUPPORTS_D3D12)
  xii_requires(XII_BUILD_D3D12)
endmacro()

# #####################################
# ## xii_link_target_d3d12(<target>)
# #####################################
function(xii_link_target_d3d12 TARGET_NAME)
  xii_requires_d3d12()

  get_property(XII_D3D12_LIBRARY GLOBAL PROPERTY XII_D3D12_LIBRARY)
  # Execute find_package once
  if(NOT XII_D3D12_LIBRARY)
    find_package(DirectX12 REQUIRED)
    if(DirectX12_FOUND)
      set_property(GLOBAL PROPERTY XII_D3D12_LIBRARY ${DirectX12_LIBRARY})
      set_property(GLOBAL PROPERTY XII_D3D12_LIBRARIES ${DirectX12_D3D12_LIBRARIES})
    endif()
  endif()

  get_property(XII_D3D12_LIBRARY GLOBAL PROPERTY XII_D3D12_LIBRARY)
  get_property(XII_D3D12_LIBRARIES GLOBAL PROPERTY XII_D3D12_LIBRARIES)

  target_link_libraries(${TARGET_NAME}
    PRIVATE
    ${XII_D3D12_LIBRARIES}
  )

  if(XII_CMAKE_ARCHITECTURE_ARM)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(D3D12_COPY_DLLS_BIT "arm64")
    else()
      set(D3D12_COPY_DLLS_BIT "arm")
      endif()
    else()
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(D3D12_COPY_DLLS_BIT "x64")
    else()
        set(D3D12_COPY_DLLS_BIT "x86")
    endif()
  endif()

  # ARM dll is not provide in the windows SDK.
  if(NOT XII_CMAKE_ARCHITECTURE_ARM)
    if(${XII_D3D12_LIBRARY} MATCHES "/10/")
      set(D3D12_COPY_DLLS_WINSDKVERSION "10")
      set(D3D12_COPY_DLLS_DLL_VERSION "47")
    endif()
  endif()

  # Copy the DirectX Shader Compiler DLL.
  if(${D3D12_COPY_DLLS_WINSDKVERSION})
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "%ProgramFiles(x86)%/Windows Kits/${D3D12_COPY_DLLS_WINSDKVERSION}/Redist/D3D/${D3D12_COPY_DLLS_BIT}/d3dcompiler_${D3D12_COPY_DLLS_DLL_VERSION}.dll"
      $<TARGET_FILE_DIR:${TARGET_NAME}>
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endif()

  # Copy the DXIL (signer) dll.
  if(${D3D12_COPY_DLLS_WINSDKVERSION})
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "%ProgramFiles(x86)%/Windows Kits/${D3D12_COPY_DLLS_WINSDKVERSION}/Redist/D3D/${D3D12_COPY_DLLS_BIT}/dxil.dll"
      $<TARGET_FILE_DIR:${TARGET_NAME}>
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endif()

endfunction()

# #####################################
# ## xii_link_target_pix_event_runtime(<target>)
# #####################################
function(xii_link_target_pix_event_runtime TARGET_NAME)
  xii_requires_pix_event_runtime()

  target_link_libraries(${TARGET_NAME} PRIVATE WinPixEventRuntime)

  target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT)

  get_target_property(_PIX_DLL_PATH WinPixEventRuntime RUNTIME_DLL_PATH)
  if (NOT _PIX_DLL_PATH)
    message(FATAL_ERROR "WinPixEventRuntime target does not have RUNTIME_DLL_PATH property set.")
  endif()

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_PIX_DLL_PATH}"
        $<TARGET_FILE_DIR:${TARGET_NAME}>
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endfunction()
