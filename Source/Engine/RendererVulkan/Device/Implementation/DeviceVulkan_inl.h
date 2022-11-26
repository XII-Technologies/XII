XII_ALWAYS_INLINE vk::Device xiiGALDeviceVulkan::GetVulkanDevice() const
{
  return m_device;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::Queue& xiiGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_graphicsQueue;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::Queue& xiiGALDeviceVulkan::GetTransferQueue() const
{
  return m_transferQueue;
}

XII_ALWAYS_INLINE vk::PhysicalDevice xiiGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_physicalDevice;
}

XII_ALWAYS_INLINE vk::Instance xiiGALDeviceVulkan::GetVulkanInstance() const
{
  return m_instance;
}


XII_ALWAYS_INLINE const xiiGALFormatLookupTableVulkan& xiiGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

/*
inline ID3D11Query* xiiGALDeviceVulkan::GetTimestamp(xiiGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<xiiUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}

*/
