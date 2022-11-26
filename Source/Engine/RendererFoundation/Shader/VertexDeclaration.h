
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_RENDERERFOUNDATION_DLL xiiGALVertexDeclaration : public xiiGALObject<xiiGALVertexDeclarationCreationDescription>
{
public:
protected:
  friend class xiiGALDevice;

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALVertexDeclaration(const xiiGALVertexDeclarationCreationDescription& Description);

  virtual ~xiiGALVertexDeclaration();
};
