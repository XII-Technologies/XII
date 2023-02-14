
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

class xiiGALResourceViewDiligent : public xiiGALResourceView
{
public:
  XII_ALWAYS_INLINE Diligent::IDeviceObject* GetResourceView();

  XII_ALWAYS_INLINE Diligent::ITextureView* GetTextureView();

  XII_ALWAYS_INLINE Diligent::IBufferView* GetBufferView();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALResourceViewDiligent(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description);

  ~xiiGALResourceViewDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pTextureView;

  Diligent::RefCntAutoPtr<Diligent::IBufferView> m_pBufferView;
};

#include <RendererDiligent/Resources/Implementation/ResourceViewDiligent_inl.h>
