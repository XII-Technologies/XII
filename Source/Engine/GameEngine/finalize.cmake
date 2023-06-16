
if (TARGET RendererDiligent)
  add_dependencies(GameEngine RendererDiligent)
endif()

if (TARGET RendererDX11)
  add_dependencies(GameEngine RendererDX11)
endif()
