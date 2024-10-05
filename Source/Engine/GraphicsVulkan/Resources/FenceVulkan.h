#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

namespace vk
{
  class Semaphore;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALFenceVulkan final : public xiiGALFence
{
public:
  XII_ALWAYS_INLINE virtual xiiUInt64 GetCompletedValue() override final { return 0ULL; }

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  XII_ALWAYS_INLINE vk::Semaphore GetVulkanSemaphore() const { return m_vkTimelineSemaphore; }

  XII_ALWAYS_INLINE bool IsTimelineSemaphore() const { return m_vkTimelineSemaphore != nullptr; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  xiiUInt64 InternalGetCompletedValue();

protected:
  static constexpr xiiUInt32 s_uiRequiredArraySize = 8U;

  struct SyncPointData
  {
    const xiiUInt64 m_uiValue;
  };

  vk::Semaphore m_vkTimelineSemaphore;

  xiiMutex                m_SyncPointGuard;
  xiiDeque<SyncPointData> m_SyncPoints;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt64 m_uiMaxSyncPoints = 0U;
#endif
};
