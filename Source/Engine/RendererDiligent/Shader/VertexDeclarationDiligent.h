
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

XII_DEFINE_AS_POD_TYPE(Diligent::LayoutElement);

class XII_RENDERERDILIGENT_DLL xiiGALVertexDeclarationDiligent : public xiiGALVertexDeclaration
{
public:
  XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* GetInputLayoutDesc() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  xiiGALVertexDeclarationDiligent(const xiiGALVertexDeclarationCreationDescription& Description);

  virtual ~xiiGALVertexDeclarationDiligent();

  Diligent::InputLayoutDesc m_InputLayoutDesc;

  xiiHybridArray<Diligent::LayoutElement, 8U> m_InputElements;
};

#include <RendererDiligent/Shader/Implementation/VertexDeclarationDiligent_inl.h>
