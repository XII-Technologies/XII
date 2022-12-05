
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

class xiiGALResourceViewDiligent : public xiiGALResourceView
{
public:
  XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::IDeviceObject>& GetResourceView();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALResourceViewDiligent(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description);

  ~xiiGALResourceViewDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::IDeviceObject> m_pResourceView;
};

#include <RendererDiligent/Resources/Implementation/ResourceViewDiligent_inl.h>
