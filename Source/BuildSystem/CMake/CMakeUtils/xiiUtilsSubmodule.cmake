# Copyright (c) Theophilus Eriata. All Rights Reserved.

function(xii_detect_languages)
  # Default languages
  set(XII_LANGUAGES C CXX PARENT_SCOPE)

  # On Windows, add CSharp only for Visual Studio generators.
  # Match any Visual Studio generator name (e.g. "Visual Studio 17 2022", "Visual Studio 18 2026").
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set(XII_LANGUAGES C CXX CSharp PARENT_SCOPE)
  endif()
endfunction()
