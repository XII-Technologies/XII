
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

class xiiGALUnorderedAccessViewDiligent : public xiiGALUnorderedAccessView
{
public:
  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IDeviceObject>& GetResourceView();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALUnorderedAccessViewDiligent(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description);

  ~xiiGALUnorderedAccessViewDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::IDeviceObject> m_pUnorderedAccessView;
};

#include <RendererDiligent/Resources/Implementation/UnorderedAccessViewDiligent_inl.h>
