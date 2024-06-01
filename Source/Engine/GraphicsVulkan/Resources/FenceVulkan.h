#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFenceVulkan final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Resources/Implementation/FenceVulkan_inl.h>
