#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFenceVulkan final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  Diligent::IFence* GetFence() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFenceVulkan(const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IFence* m_pFence = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/FenceVulkan_inl.h>
