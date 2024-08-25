
XII_ALWAYS_INLINE xiiAllocatorBase* xiiGALDeviceVulkan::GetAllocator() const
{
  return m_Allocator.GetParent();
}

XII_ALWAYS_INLINE vk::Instance xiiGALDeviceVulkan::GetVulkanInstance() const
{
  return m_Instance;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALDeviceVulkan::GetVulkanVersion() const
{
  return m_uiVulkanVersion;
}

XII_ALWAYS_INLINE const vk::DispatchLoaderDynamic& xiiGALDeviceVulkan::GetVulkanDynamicDispatchLoader() const
{
  return m_InstanceDispatchLoader;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::LayerProperties> xiiGALDeviceVulkan::GetVulkanInstanceLayers()
{
  return m_Layers;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::ExtensionProperties> xiiGALDeviceVulkan::GetVulkanInstanceExtensionProperties()
{
  return m_Extensions;
}

XII_ALWAYS_INLINE xiiArrayPtr<const char*> xiiGALDeviceVulkan::GetVulkanInstanceEnabledExtensions()
{
  return m_EnabledExtensions;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::PhysicalDevice> xiiGALDeviceVulkan::GetVulkanPhysicalDevices()
{
  return m_PhysicalDevices;
}

XII_ALWAYS_INLINE vk::PhysicalDevice xiiGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_PhysicalDevice;
}

XII_ALWAYS_INLINE const vk::PhysicalDeviceProperties& xiiGALDeviceVulkan::GetVulkanPhysicalDeviceProperties() const
{
  return m_PhysicalDeviceProperties;
}

XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& xiiGALDeviceVulkan::GetVulkanPhysicalDeviceFeatures() const
{
  return m_PhysicalDeviceFeatures;
}

XII_ALWAYS_INLINE const vk::PhysicalDeviceMemoryProperties& xiiGALDeviceVulkan::GetVulkanPhysicalDeviceMemoryProperties() const
{
  return m_PhysicalDeviceMemoryProperties;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::QueueFamilyProperties> xiiGALDeviceVulkan::GetPhysicalDeviceQueueFamilyProperties()
{
  return m_PhysicalDeviceQueueFamilyProperties;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::ExtensionProperties> xiiGALDeviceVulkan::GetPhysicalDeviceSupportedExtensions()
{
  return m_PhysicalDeviceSupportedExtensions;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& xiiGALDeviceVulkan::GetPhysicalDeviceExtensionFeatures() const
{
  return m_PhysicalDeviceExtensionFeatures;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionProperties& xiiGALDeviceVulkan::GetPhysicalDeviceExtensionProperties() const
{
  return m_PhysicalDeviceExtensionProperties;
}

XII_ALWAYS_INLINE vk::Device xiiGALDeviceVulkan::GetVulkanLogicalDevice() const
{
  return m_LogicalDevice;
}

XII_ALWAYS_INLINE const vk::PhysicalDeviceFeatures& xiiGALDeviceVulkan::GetVulkanLogicalDeviceFeatures() const
{
  return m_LogicalDeviceFeatures;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::ExtensionFeatures& xiiGALDeviceVulkan::GetVulkanLogicalDeviceExtensionFeatures() const
{
  return m_LogicalDeviceExtensionFeatures;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::PipelineStageFlags> xiiGALDeviceVulkan::GetVulkanLogicalDeviceSupportedStagesFlags()
{
  return m_LogicalDeviceSupportedStagesFlags;
}

XII_ALWAYS_INLINE xiiArrayPtr<vk::AccessFlags> xiiGALDeviceVulkan::GetVulkanLogicalDeviceSupportedAccessFlags()
{
  return m_LogicalDeviceSupportedAccessFlags;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& xiiGALDeviceVulkan::GetGraphicsQueueInformation() const
{
  return m_GraphicsQueueInformation;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& xiiGALDeviceVulkan::GetComputeQueueInformation() const
{
  return m_ComputeQueueInformation;
}

XII_ALWAYS_INLINE const xiiGALDeviceVulkan::QueueInformation& xiiGALDeviceVulkan::GetTransferQueueInformation() const
{
  return m_TransferQueueInformation;
}

XII_ALWAYS_INLINE xiiGALDeviceVulkan::DebugMode xiiGALDeviceVulkan::GetDebugMode() const
{
  return m_DebugMode;
}
