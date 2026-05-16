# Copyright (c) Theophilus Eriata. All Rights Reserved.

# #####################################
# ## xii_requires_renderer()
# #####################################
macro(xii_requires_renderer)
  xii_requires_one_of(XII_BUILD_VULKAN XII_BUILD_D3D12)
endmacro()

# #####################################
# ## xii_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################
function(xii_add_renderers TARGET_NAME)
  set(ARG_OPTIONS EXCLUDE_SHADER_COMPILER EXCLUDE_VULKAN EXCLUDE_D3D12)
  set(ARG_ONEVALUEARGS "")
  set(ARG_MULTIVALUEARGS "")
  cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "xii_add_renderers: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
  endif()

  target_link_libraries(${TARGET_NAME} PRIVATE GraphicsFoundation)

  if(XII_BUILD_VULKAN AND NOT ARG_EXCLUDE_VULKAN)
    if(TARGET GraphicsVulkan)
      add_dependencies(${TARGET_NAME} GraphicsVulkan)
    endif()
    if(TARGET ShaderCompilerSPIRV AND NOT ARG_EXCLUDE_SHADER_COMPILER)
      add_dependencies(${TARGET_NAME} ShaderCompilerSPIRV)
    endif()
  endif()

  if(XII_BUILD_D3D12 AND NOT ARG_EXCLUDE_D3D12)
    if(TARGET GraphicsD3D12)
      add_dependencies(${TARGET_NAME} GraphicsD3D12)
    endif()
    if(TARGET ShaderCompilerDXC AND NOT ARG_EXCLUDE_SHADER_COMPILER)
      add_dependencies(${TARGET_NAME} ShaderCompilerDXC)
    endif()
  endif()
endfunction()
