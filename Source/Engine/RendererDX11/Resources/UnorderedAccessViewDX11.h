
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class xiiGALUnorderedAccessViewDX11 : public xiiGALUnorderedAccessView
{
public:
  XII_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALUnorderedAccessViewDX11(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description);

  ~xiiGALUnorderedAccessViewDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/UnorderedAccessViewDX11_inl.h>
