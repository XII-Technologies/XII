######################################
### Diligent Engine support
######################################

set (XII_BUILD_DILIGENT OFF CACHE BOOL "Whether to enable experimental / work-in-progress Diligent Renderer code")

######################################
### xii_requires_diligent()
######################################

macro(xii_requires_diligent)

	xii_requires(XII_BUILD_DILIGENT)

endmacro()

######################################
### xii_link_target_diligent(<target>)
######################################

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

######################################
### xii_link_target_diligent_dx11(<target>)
######################################
function(xii_link_target_diligent_dx11 TARGET_NAME)

	xii_link_target_diligent(${TARGET_NAME})

	if(D3D11_SUPPORTED)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			Diligent-GraphicsEngineD3D11-shared
			RendererDiligent
			RendererDiligentD3D11
		)
		
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineD3D11-shared)
	endif()
	
	if(TARGET Diligent-Archiver-shared)
		list(APPEND ENGINE_DLLS Diligent-Archiver-shared)
    endif()
	
	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
	
	# Copy D3Dcompiler_47.dll, dxcompiler.dll, and dxil.dll
	if(MSVC)
		if (D3D11_SUPPORTED AND VS_D3D_COMPILER_PATH)
			# Note that VS_D3D_COMPILER_PATH can only be used in a Visual Studio command
			# and is not a valid path during CMake configuration
			list(APPEND SHADER_COMPILER_DLLS ${VS_D3D_COMPILER_PATH})
		endif()

		foreach(DLL ${SHADER_COMPILER_DLLS})
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					${DLL}
					"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
		endforeach(DLL)
		
	endif()

endfunction()

######################################
### xii_link_target_diligent_dx12(<target>)
######################################
function(xii_link_target_diligent_dx12 TARGET_NAME)

	xii_link_target_diligent(${TARGET_NAME})

	if(D3D12_SUPPORTED)
		target_link_libraries(${TARGET_NAME} PRIVATE Diligent-GraphicsEngineD3D12-shared)
		
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineD3D12-shared)
	endif()
	
	 if(TARGET Diligent-Archiver-shared)
		list(APPEND ENGINE_DLLS Diligent-Archiver-shared)
	endif()

	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)
	
	# Copy D3Dcompiler_47.dll, dxcompiler.dll, and dxil.dll
	if(MSVC)
		if (D3D12_SUPPORTED AND VS_D3D_COMPILER_PATH)
			# Note that VS_D3D_COMPILER_PATH can only be used in a Visual Studio command
			# and is not a valid path during CMake configuration
			list(APPEND SHADER_COMPILER_DLLS ${VS_D3D_COMPILER_PATH})
		endif()

		if(D3D12_SUPPORTED AND VS_DXC_COMPILER_PATH AND VS_DXIL_SIGNER_PATH)
			# For the compiler to sign the bytecode, you have to have a copy of dxil.dll in
			# the same folder as the dxcompiler.dll at runtime.

			# Note that VS_DXC_COMPILER_PATH and VS_DXIL_SIGNER_PATH can only be used in a Visual Studio command
			# and are not valid paths during CMake configuration
			list(APPEND SHADER_COMPILER_DLLS ${VS_DXC_COMPILER_PATH})
			list(APPEND SHADER_COMPILER_DLLS ${VS_DXIL_SIGNER_PATH})
		endif()

		foreach(DLL ${SHADER_COMPILER_DLLS})
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					${DLL}
					"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
		endforeach(DLL)

		if(D3D12_SUPPORTED AND EXISTS ${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH})
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH}
					"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
		endif()

	endif()
	
endfunction()

######################################
### xii_link_target_diligent_vulkan(<target>)
######################################
function(xii_link_target_diligent_vulkan TARGET_NAME)

	xii_link_target_diligent(${TARGET_NAME})

	if(VULKAN_SUPPORTED)
		target_link_libraries(${TARGET_NAME} PRIVATE Diligent-GraphicsEngineVk-shared)
		
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineVk-shared)
	endif()
	
	# Copy D3Dcompiler_47.dll, dxcompiler.dll, and dxil.dll
	if(MSVC)
		if(VULKAN_SUPPORTED)
			if(NOT DEFINED DILIGENT_DXCOMPILER_FOR_SPIRV_PATH)
				message(FATAL_ERROR "DILIGENT_DXCOMPILER_FOR_SPIRV_PATH is undefined, check order of cmake includes")
			endif()
			if(EXISTS ${DILIGENT_DXCOMPILER_FOR_SPIRV_PATH})
				add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
						${DILIGENT_DXCOMPILER_FOR_SPIRV_PATH}
						"\"$<TARGET_FILE_DIR:${TARGET_NAME}>/spv_dxcompiler.dll\"")
			endif()
		endif()
	endif()

endfunction()

######################################
### xii_link_target_diligent_metal(<target>)
######################################
function(xii_link_target_diligent_metal TARGET_NAME)

	xii_link_target_diligent(${TARGET_NAME})

	if(METAL_SUPPORTED)
		target_link_libraries(${TARGET_NAME} PRIVATE Diligent-GraphicsEngineMetal-shared)
		
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineMetal-shared)
	endif()
	
	if(TARGET Diligent-Archiver-shared)
		list(APPEND ENGINE_DLLS Diligent-Archiver-shared)
	endif()

	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)

endfunction()

######################################
### xii_link_target_diligent_opengl(<target>)
######################################
function(xii_link_target_diligent_opengl TARGET_NAME)

	xii_link_target_diligent(${TARGET_NAME})

	if(GL_SUPPORTED)
		target_link_libraries(${TARGET_NAME} PRIVATE Diligent-GraphicsEngineOpenGL-shared)
		
		list(APPEND ENGINE_DLLS Diligent-GraphicsEngineOpenGL-shared)
	endif()
	
	if(TARGET Diligent-Archiver-shared)
		list(APPEND ENGINE_DLLS Diligent-Archiver-shared)
	endif()

	foreach(DLL ${ENGINE_DLLS})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:${DLL}>\""
				"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endforeach(DLL)

endfunction()
