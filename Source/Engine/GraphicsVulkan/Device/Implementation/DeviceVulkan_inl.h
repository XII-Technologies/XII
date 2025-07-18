
XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceVulkan::GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueue.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueue.Borrow();

  return m_pGraphicsCommandQueue.Borrow();
}

template <typename ObjectHandle, typename>
XII_FORCE_INLINE void xiiGALDeviceVulkan::SetVulkanObjectDebugName(ObjectHandle& vkObject, const char* szDebugName, xiiVulkanAllocation allocation /*= {}*/) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_DebugMode == DebugMode::Utils)
  {
    if (vkObject == VK_NULL_HANDLE)
      return;

    vk::DebugUtilsObjectNameInfoEXT vkDebugObjectNameInfo = {};
    vkDebugObjectNameInfo.pNext                           = nullptr;
    vkDebugObjectNameInfo.objectType                      = vkObject.objectType;
    vkDebugObjectNameInfo.objectHandle                    = (uint64_t)static_cast<typename ObjectHandle::NativeType>(vkObject);
    vkDebugObjectNameInfo.pObjectName                     = szDebugName;

    VK_ASSERT_DEV(m_LogicalDevice.setDebugUtilsObjectNameEXT(&vkDebugObjectNameInfo, m_InstanceDispatchLoader));

    SetVulkanAllocationDebugName(allocation, szDebugName);
  }
#else
  XII_IGNORE_UNUSED(vkObject);
  XII_IGNORE_UNUSED(szDebugName);
  XII_IGNORE_UNUSED(allocation);
#endif
}

template <typename T, typename>
XII_ALWAYS_INLINE void xiiGALDeviceVulkan::SafeReleaseDeviceObject(T&& vkObject, xiiVulkanAllocation&& allocation /*= nullptr*/)
{
  if (vkObject == VK_NULL_HANDLE)
    return;

  SafeReleaseDeviceObjectInternal(vkObject.objectType, static_cast<void*>(vkObject), allocation);
}

XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& xiiGALDeviceVulkan::GetCommandQueueInformation(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_TransferQueueInformation;

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_ComputeQueueInformation;

  return m_GraphicsQueueInformation;
}

XII_ALWAYS_INLINE xiiGALCommandBufferPoolVulkan* xiiGALDeviceVulkan::GetCommandBufferPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandBufferPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandBufferPool.Borrow();

  return m_pGraphicsCommandBufferPool.Borrow();
}

XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* xiiGALDeviceVulkan::GetCommandQueueQueryPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueueQueryPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueueQueryPool.Borrow();

  return m_pGraphicsCommandQueueQueryPool.Borrow();
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiGALDeviceVulkan::GetSupportedStagesFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  const xiiGALQueueInformationVulkan& queueInformation = GetCommandQueueInformation(queueFlags);

  return GetVulkanLogicalDeviceSupportedStagesFlags(queueInformation.m_uiQueueFamilyIndex);
}

XII_ALWAYS_INLINE vk::AccessFlags xiiGALDeviceVulkan::GetSupportedAccessFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  const xiiGALQueueInformationVulkan& queueInformation = GetCommandQueueInformation(queueFlags);

  return GetVulkanLogicalDeviceSupportedAccessFlags(queueInformation.m_uiQueueFamilyIndex);
}
