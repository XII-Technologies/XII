# #####################################
# ## xii_requires_renderer()
# #####################################

macro(xii_requires_renderer)
	if(XII_CMAKE_PLATFORM_WINDOWS)
		xii_requires_d3d()
	endif()
endmacro()

# #####################################
# ## xii_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################

function(xii_add_renderers TARGET_NAME)
	if(XII_CMAKE_PLATFORM_WINDOWS)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDX11
		)
		xii_link_target_dx11(${TARGET_NAME})
	endif()
	
	if (XII_BUILD_DILIGENT)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDiligent
		)
	endif()

    add_dependencies(${TARGET_NAME}
        ShaderCompiler
    )
endfunction()
