# #####################################
# ## Embree support
# #####################################

set(XII_BUILD_EMBREE OFF CACHE BOOL "Whether support for Intel Embree should be added")

# #####################################
# ## xii_requires_embree()
# #####################################
macro(xii_requires_embree)
	xii_requires_windows()
	xii_requires(XII_BUILD_EMBREE)
endmacro()

# #####################################
# ## xii_link_target_embree(<target>)
# #####################################
function(xii_link_target_embree TARGET_NAME)
	xii_requires_embree()

	find_package(XIIEmbree REQUIRED)

	if(XIIEMBREE_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE XIIEmbree::XIIEmbree)

		target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_EMBREE_SUPPORT)

		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:XIIEmbree::XIIEmbree> $<TARGET_FILE_DIR:${TARGET_NAME}>
		)
	endif()
endfunction()
