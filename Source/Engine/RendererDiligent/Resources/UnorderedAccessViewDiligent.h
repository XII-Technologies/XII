
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

class xiiGALUnorderedAccessViewDiligent : public xiiGALUnorderedAccessView
{
public:
  XII_ALWAYS_INLINE Diligent::IDeviceObject* GetResourceView();

  XII_ALWAYS_INLINE Diligent::ITextureView* GetTextureView();

  XII_ALWAYS_INLINE Diligent::IBufferView* GetBufferView();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALUnorderedAccessViewDiligent(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description);

  virtual ~xiiGALUnorderedAccessViewDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pUnorderedAccessTextureView;

  Diligent::RefCntAutoPtr<Diligent::IBufferView> m_pUnorderedAccessBufferView;
};

#include <RendererDiligent/Resources/Implementation/UnorderedAccessViewDiligent_inl.h>
