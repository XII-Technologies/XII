
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetNextFenceValue() const
{
  return 0U;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetCompletedFenceValue()
{
  return 0U;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALCommandQueueVulkan::GetVulkanQueueFamilyIndex() const
{
  return m_uiQueueFamilyIndex;
}

XII_ALWAYS_INLINE vk::Queue xiiGALCommandQueueVulkan::GetVulkanQueue() const
{
  return m_vkQueue;
}
