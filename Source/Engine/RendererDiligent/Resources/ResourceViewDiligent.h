
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

  Diligent::ITextureView* m_pTextureView = nullptr;

  Diligent::IBufferView* m_pBufferView = nullptr;
};

#include <RendererDiligent/Resources/Implementation/ResourceViewDiligent_inl.h>
