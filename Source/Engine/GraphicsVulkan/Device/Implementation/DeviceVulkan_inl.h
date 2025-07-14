
template <typename ObjectHandle, typename>
XII_FORCE_INLINE void xiiGALDeviceVulkan::SetVulkanObjectDebugName(ObjectHandle& vkObject, const char* szDebugName, VmaAllocation vmaAllocation /*= {}*/) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_DebugMode != DebugMode::Disabled)
  {
    if (vkObject == VK_NULL_HANDLE)
      return;

    vk::DebugUtilsObjectNameInfoEXT vkDebugObjectNameInfo = {};
    vkDebugObjectNameInfo.pNext                           = nullptr;
    vkDebugObjectNameInfo.objectType                      = vkObject.objectType;
    vkDebugObjectNameInfo.objectHandle                    = (uint64_t)static_cast<typename ObjectHandle::NativeType>(vkObject);
    vkDebugObjectNameInfo.pObjectName                     = szDebugName;

    m_LogicalDevice.setDebugUtilsObjectNameEXT(vkDebugObjectNameInfo, m_InstanceDispatchLoader);

    if (vmaAllocation != nullptr)
    {
      vmaSetAllocationUserData(m_vkVmaAllocator, vmaAllocation, (void*)vkDebugObjectNameInfo.pObjectName);
    }
  }
#else
  XII_IGNORE_UNUSED(vkObject);
  XII_IGNORE_UNUSED(szDebugName);
  XII_IGNORE_UNUSED(vmaAllocation);
#endif
}

template <typename T, typename>
XII_FORCE_INLINE void xiiGALDeviceVulkan::SafeReleaseDeviceObject(T&& vkObject, VmaAllocation&& vmaAllocation /*= nullptr*/)
{
  if (vkObject == VK_NULL_HANDLE)
    return;

  SafeReleaseDeviceObjectInternal(vkObject.objectType, static_cast<void*>(vkObject), vmaAllocation);
}

XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& xiiGALDeviceVulkan::GetCommandQueueInformation(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_TransferQueueInformation;

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_ComputeQueueInformation;

  return m_GraphicsQueueInformation;
}

XII_ALWAYS_INLINE xiiGALQueryPoolVulkan* xiiGALDeviceVulkan::GetCommandQueueQueryPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueueQueryPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueueQueryPool.Borrow();

  return m_pGraphicsCommandQueueQueryPool.Borrow();
}
