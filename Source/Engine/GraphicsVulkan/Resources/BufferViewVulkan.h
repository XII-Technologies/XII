#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBufferViewVulkan final : public xiiGALBufferView
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferViewVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Resources/Implementation/BufferViewVulkan_inl.h>
