
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

struct ID3D11InputLayout;

class xiiGALVertexDeclarationDX11 : public xiiGALVertexDeclaration
{
public:
  XII_ALWAYS_INLINE ID3D11InputLayout* GetDXInputLayout() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  xiiGALVertexDeclarationDX11(const xiiGALVertexDeclarationCreationDescription& Description);

  virtual ~xiiGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
