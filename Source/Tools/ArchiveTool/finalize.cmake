xii_pull_all_vars()

if (TARGET FoundationTest AND TARGET ArchiveTool)
  if (XII_CMAKE_PLATFORM_WINDOWS)
    add_dependencies(FoundationTest ArchiveTool)
  endif()
endif()
