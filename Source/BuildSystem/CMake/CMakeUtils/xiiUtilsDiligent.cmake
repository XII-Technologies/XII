# #####################################
# ## Diligent Engine support
# #####################################

set (XII_BUILD_DILIGENT ON CACHE BOOL "Enable Diligent Graphics abstraction driver")

# #####################################
# ## xii_requires_diligent()
# #####################################

macro(xii_requires_diligent)
	xii_requires(XII_BUILD_DILIGENT)
endmacro()

# #####################################
# ## xii_link_target_diligent(<target>)
# #####################################

function(xii_link_target_diligent TARGET_NAME)
	target_link_libraries(${TARGET_NAME}
        PRIVATE
        Diligent-BuildSettings
        Diligent-Common
        Diligent-GraphicsTools
    )
	target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/ThirdParty/DiligentCore/)
    target_compile_definitions(${TARGET_NAME} PRIVATE ENGINE_DLL=1)
endfunction()

# #####################################
# ## xii_link_target_diligent_d3d11(<target>)
# #####################################
function(xii_link_target_diligent_d3d11 TARGET_NAME)
	xii_link_target_diligent(${TARGET_NAME})

	if(D3D11_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			Diligent-GraphicsEngineD3D11-shared
		)
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineD3D11-shared)
	endif()

	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
endfunction()

# #####################################
# ## xii_link_target_diligent_d3d12(<target>)
# #####################################
function(xii_link_target_diligent_d3d12 TARGET_NAME)
	xii_link_target_diligent(${TARGET_NAME})

	if(D3D12_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
            PRIVATE
            Diligent-GraphicsEngineD3D12-shared
        )
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineD3D12-shared)
	endif()

	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)

	if(MSVC)
        # Copy PIX Runtime if available
		if(D3D12_SUPPORTED AND EXISTS ${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH})
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH}
					"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
		endif()
	endif()
endfunction()

# #####################################
# ## xii_link_target_diligent_vulkan(<target>)
# #####################################
function(xii_link_target_diligent_vulkan TARGET_NAME)
	xii_link_target_diligent(${TARGET_NAME})

	if(VULKAN_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
            PRIVATE
            Diligent-GraphicsEngineVk-shared
        )
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineVk-shared)
	endif()

    foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
endfunction()

# #####################################
# ## xii_link_target_diligent_metal(<target>)
# #####################################
function(xii_link_target_diligent_metal TARGET_NAME)
	xii_link_target_diligent(${TARGET_NAME})

	if(METAL_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
            PRIVATE
            Diligent-GraphicsEngineMetal-shared
        )
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineMetal-shared)
	endif()

    foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
endfunction()

# #####################################
# ## xii_link_target_diligent_opengl(<target>)
# #####################################
function(xii_link_target_diligent_opengl TARGET_NAME)
	xii_link_target_diligent(${TARGET_NAME})

	if(GL_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
            PRIVATE
            Diligent-GraphicsEngineOpenGL-shared
        )
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineOpenGL-shared)
	endif()

    foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
endfunction()
