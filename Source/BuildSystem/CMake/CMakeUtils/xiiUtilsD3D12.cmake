# #####################################
# ## DirectX 12 support
# #####################################

if (XII_CMAKE_PLATFORM_WINDOWS)
    set(XII_BUILD_D3D12 ON CACHE BOOL "Build the DirectX 12 Graphics Device")
else()
    set(XII_BUILD_D3D12 OFF CACHE BOOL "Build the DirectX 12 Graphics Device")
endif()

# #####################################
# ## xii_requires_d3d12()
# #####################################

macro(xii_requires_d3d12)
    xii_requires_windows()
    xii_requires(XII_BUILD_D3D12)
endmacro()

# #####################################
# ## xii_link_target_dx11(<target>)
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

    if(${D3D12_COPY_DLLS_WINSDKVERSION})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"%ProgramFiles(x86)%/Windows Kits/${D3D12_COPY_DLLS_WINSDKVERSION}/Redist/D3D/${D3D12_COPY_DLLS_BIT}/d3dcompiler_${D3D12_COPY_DLLS_DLL_VERSION}.dll"
			$<TARGET_FILE_DIR:${TARGET_NAME}>
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	endif()

    # Note that CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is stated to be defined when targeting Windows 10
    # and above, however it is also defined when targeting 8.1 and Visual Studio 2019 (but not VS2017)
    if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
        if (DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
            # Note that VS_WINDOWS_SDK_BIN_DIR as well as all derived paths can only be used in Visual Studio
            # commands and are not valid paths during CMake configuration
            set(VS_WINDOWS_SDK_BIN_DIR "$(WindowsSdkDir)\\bin\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\${D3D12_COPY_DLLS_BIT}")

            # DXC is only present in Windows SDK starting with version 10.0.17763.0
            if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_GREATER_EQUAL "10.0.17763.0")
                message("\"${VS_WINDOWS_SDK_BIN_DIR}\\dxil.dll\"")
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			        COMMAND ${CMAKE_COMMAND} -E copy_if_different
			        "\"${VS_WINDOWS_SDK_BIN_DIR}\\dxil.dll\""
			        $<TARGET_FILE_DIR:${TARGET_NAME}>
			        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		        )
            endif()
        endif()
    endif()
endfunction()
