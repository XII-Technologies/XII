#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class Fence;
  class Semaphore;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALCpuWaitOnlyFenceVulkan final
{
public:
  struct SyncPointData
  {
    xiiUInt64 m_uiValue;
    vk::Fence m_vkFence;
  };

  xiiGALCpuWaitOnlyFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan);

  ~xiiGALCpuWaitOnlyFenceVulkan();

  virtual xiiUInt64 GetCompletedValue();

  virtual void Wait(xiiUInt64 uiValue);

  void Reset(xiiUInt64 uiValue);

  [[nodiscard]] const xiiGALCpuWaitOnlyFenceVulkan::SyncPointData& CreateSyncPoint(const xiiUInt64 uiFenceValue);

private:
  xiiUInt64 InternalGetCompletedValue();

  void UpdateLastCompletedFenceValue(xiiUInt64 uiValue);

  static constexpr xiiUInt32 s_uiRequiredArraySize = 8U;

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiMutex                m_SyncPointGuard;
  xiiDeque<SyncPointData> m_SyncPoints;

  std::atomic<xiiUInt64> m_LastCompletedFenceValue;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt64 m_uiMaxSyncPoints = 0U;
#endif
};
