#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

namespace vk
{
  class Fence;
  class Semaphore;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALFenceVulkan final : public xiiGALFence
{
public:
  struct SyncPointData
  {
    xiiUInt64 m_uiValue;
    vk::Fence m_vkFence;
  };

  XII_ALWAYS_INLINE vk::Semaphore GetVulkanTimelineSemaphore() const { return m_vkTimelineSemaphore; }

  XII_ALWAYS_INLINE bool IsTimelineSemaphore() const { return m_vkTimelineSemaphore != nullptr; }

  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  void Reset(xiiUInt64 uiValue);

  const xiiGALFenceVulkan::SyncPointData& CreateSyncPoint(const xiiUInt64 uiFenceValue);

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  void ReleaseResourcesImmediately();

  xiiUInt64 InternalGetCompletedValue();

protected:
  static constexpr xiiUInt32 s_uiRequiredArraySize = 8U;

  vk::Semaphore m_vkTimelineSemaphore;

  xiiMutex                m_SyncPointGuard;
  xiiDeque<SyncPointData> m_SyncPoints;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt64 m_uiMaxSyncPoints = 0U;
#endif
};
