
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class xiiGALResourceViewDX11 : public xiiGALResourceView
{
public:
  XII_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALResourceViewDX11(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description);

  ~xiiGALResourceViewDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView;
};

#include <RendererDX11/Resources/Implementation/ResourceViewDX11_inl.h>
