# Copyright (c) Theophilus Eriata. All Rights Reserved.

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_NAME "Linux")
  set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_PREFIX "Linux")
  set_property(GLOBAL PROPERTY XII_CMAKE_PLATFORM_POSTFIX "Linux")
endif()
