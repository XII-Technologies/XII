#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBufferViewVulkan final : public xiiGALBufferView
{
public:
  Diligent::IBufferView* GetBufferView() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferViewVulkan(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IBufferView* m_pBufferView = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/BufferViewVulkan_inl.h>
