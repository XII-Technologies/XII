# Copyright (c) Theophilus Eriata. All Rights Reserved.

if (TARGET GraphicsVulkan)
  add_dependencies(GameEngine GraphicsVulkan)
endif()

if (TARGET GraphicsD3D12)
  add_dependencies(GameEngine GraphicsD3D12)
endif()
