
if (TARGET TextureSample AND TARGET RendererDiligent)
  add_dependencies(TextureSample RendererDiligent)
endif()
