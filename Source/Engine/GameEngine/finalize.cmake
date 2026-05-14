# Copyright (c) Theophilus Eriata. All Rights Reserved.

if (TARGET GraphicsVulkan)
  add_dependencies(GameEngine GraphicsVulkan)
endif()

if (TARGET GraphicsD3D12)
  add_dependencies(GameEngine GraphicsD3D12)
endif()

if (TARGET InspectorPlugin)
  add_dependencies(GameEngine InspectorPlugin)
endif()

if (TARGET FileservePlugin)
  add_dependencies(GameEngine FileservePlugin)
endif()
