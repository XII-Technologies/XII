#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFenceVulkan final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override;

  virtual void Signal(xiiUInt64 uiValue) override;

  virtual void Wait(xiiUInt64 uiValue) override;

  Diligent::IFence* GetFence() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFenceVulkan(const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IFence* m_pFence = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/FenceVulkan_inl.h>
