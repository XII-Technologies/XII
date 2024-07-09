# #####################################
# ## xii_add_dependency(<dstTarget> <srcTarget>)
# #####################################

function(xii_add_dependency DST_TARGET SRC_TARGET)
  if(NOT TARGET ${DST_TARGET})
    # message(STATUS "DST_TARGET '${DST_TARGET}' is unknown")
    return()
  endif()

  if(NOT TARGET ${SRC_TARGET})
    # message(STATUS "SRC_TARGET '${SRC_TARGET}' is unknown")
    return()
  endif()

  add_dependencies(${DST_TARGET} ${SRC_TARGET})
endfunction()

# #####################################
# ## xii_add_as_runtime_dependency(<target>)
# #####################################
function(xii_add_as_runtime_dependency TARGET_NAME)
  # Editor
  xii_add_dependency(Editor ${TARGET_NAME})
  xii_add_dependency(EditorProcessor ${TARGET_NAME})

  # Player
  xii_add_dependency(Player ${TARGET_NAME})

  # Samples
  xii_add_dependency(ShaderExplorer ${TARGET_NAME})
  xii_add_dependency(TextureSample ${TARGET_NAME})
endfunction()
