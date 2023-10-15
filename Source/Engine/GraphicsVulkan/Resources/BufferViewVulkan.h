#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBufferViewVulkan final : public xiiGALBufferView
{
public:
  XII_ALWAYS_INLINE const Diligent::IBufferView* GetBufferView() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferViewVulkan(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IBufferView> m_pBufferView;
};

#include <GraphicsVulkan/Resources/Implementation/BufferViewVulkan_inl.h>
