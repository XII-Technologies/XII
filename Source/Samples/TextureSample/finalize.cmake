
if (TARGET TextureSample AND TARGET RendererDiligent)
  add_dependencies(TextureSample RendererDiligent)
endif()

if (TARGET TextureSample AND TARGET RendererDX11)
  add_dependencies(TextureSample RendererDX11)
endif()
