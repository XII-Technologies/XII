# #####################################
# ## xii_requires_renderer()
# #####################################

macro(xii_requires_renderer)
	if(XII_CMAKE_PLATFORM_WINDOWS)
		xii_requires_d3d()
	else()
		xii_requires_vulkan()
	endif()
endmacro()

# #####################################
# ## xii_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has accedss to all available renderers.
# #####################################
function(xii_add_renderers TARGET_NAME)
	if(XII_BUILD_EXPERIMENTAL_VULKAN)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererVulkan
		)

		add_dependencies(${TARGET_NAME}
			ShaderCompilerDXC
		)
	endif()

	if(XII_CMAKE_PLATFORM_WINDOWS)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDX11
		)
		xii_link_target_dx11(${TARGET_NAME})

		add_dependencies(${TARGET_NAME}
			ShaderCompilerHLSL
		)
	endif()
endfunction()