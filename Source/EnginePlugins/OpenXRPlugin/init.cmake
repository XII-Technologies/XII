######################################
### OpenXR support
######################################

set (XII_BUILD_OPENXR OFF CACHE BOOL "Whether support for OpenXR should be added")

######################################
### xii_requires_openxr()
######################################

macro(xii_requires_openxr)

	xii_requires_windows()
	xii_requires(XII_BUILD_OPENXR)
	# While counter-intuitive, we need to find the package here so that the PUBLIC inherited
	# target_sources using generator expressions can be resolved in the dependant projects.
	find_package(xiiOpenXR REQUIRED)

endmacro()

######################################
### xii_link_target_openxr(<target>)
######################################

function(xii_link_target_openxr TARGET_NAME)

	xii_requires_openxr()

	find_package(xiiOpenXR REQUIRED)

	if (XIIOPENXR_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE xiiOpenXR::Loader)

        get_target_property(_dll_location xiiOpenXR::Loader IMPORTED_LOCATION)
		if (NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:xiiOpenXR::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()
        
        if (XII_CMAKE_PLATFORM_WINDOWS_DESKTOP AND XII_CMAKE_ARCHITECTURE_64BIT)
            # This will add the remoting .targets file.
            target_link_libraries(${TARGET_NAME} PRIVATE $<TARGET_FILE:xiiOpenXR::Remoting>)
        endif()
		unset(_dll_location)
		xii_uwp_add_import_to_sources(${TARGET_NAME} xiiOpenXR::Loader)
      
	endif()

endfunction()

