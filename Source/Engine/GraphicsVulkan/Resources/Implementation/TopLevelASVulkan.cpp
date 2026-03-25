#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>

namespace
{
  [[nodiscard]] static vk::BuildAccelerationStructureFlagsKHR ConvertBuildFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> flags)
  {
    vk::BuildAccelerationStructureFlagsKHR vkFlags = {};

    if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowUpdate))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowCompaction))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastTrace))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastBuild))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::LowMemory))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eLowMemory;

    return vkFlags;
  }
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelASVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTopLevelASVulkan::xiiGALTopLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALTopLevelASVulkan::~xiiGALTopLevelASVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (m_vkAccelerationStructure != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkAccelerationStructure));
    m_vkAccelerationStructure = VK_NULL_HANDLE;
  }

  if (m_vkBuffer != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkBuffer), std::move(m_BufferMemoryAllocation));
    m_vkBuffer               = VK_NULL_HANDLE;
    m_BufferMemoryAllocation = VK_NULL_HANDLE;
  }
}

xiiResult xiiGALTopLevelASVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  const auto& extensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();
  if (extensionFeatures.m_AccelerationStructure.accelerationStructure == vk::False)
  {
    xiiLog::Error("Failed to create TLAS: VK_KHR_acceleration_structure is not enabled.");
    return XII_FAILURE;
  }

  xiiUInt64 uiAccelerationStructureSize = m_Description.m_uiCompactedSize;
  m_ScratchBufferSizeDescription        = {};

  if (uiAccelerationStructureSize == 0U)
  {
    vk::AccelerationStructureGeometryInstancesDataKHR instancesData = {};
    instancesData.arrayOfPointers                                   = vk::False;
    instancesData.data.deviceAddress                                = 0U;

    vk::AccelerationStructureGeometryDataKHR geometryData = {};
    geometryData.instances                                = instancesData;

    vk::AccelerationStructureGeometryKHR geometry = {};
    geometry.geometryType                           = vk::GeometryTypeKHR::eInstances;
    geometry.geometry                               = geometryData;

    const xiiUInt32 uiPrimitiveCount = m_Description.m_uiMaxInstanceCount;

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    buildInfo.type                                          = vk::AccelerationStructureTypeKHR::eTopLevel;
    buildInfo.flags                                         = ConvertBuildFlags(m_Description.m_Flags);
    buildInfo.mode                                          = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.geometryCount                                 = 1U;
    buildInfo.pGeometries                                   = &geometry;

    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = {};
    vkLogicalDevice.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, &uiPrimitiveCount, &sizeInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    uiAccelerationStructureSize            = sizeInfo.accelerationStructureSize;
    m_ScratchBufferSizeDescription.m_uiBuild  = sizeInfo.buildScratchSize;
    m_ScratchBufferSizeDescription.m_uiUpdate = sizeInfo.updateScratchSize;
  }

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.size                 = uiAccelerationStructureSize;
  vkBufferCreateInfo.usage                = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(pDeviceVulkan->GetVulkanMemoryAllocator()->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

  vk::AccelerationStructureCreateInfoKHR vkCreateInfo = {};
  vkCreateInfo.buffer                                 = m_vkBuffer;
  vkCreateInfo.offset                                 = 0U;
  vkCreateInfo.size                                   = uiAccelerationStructureSize;
  vkCreateInfo.type                                   = vk::AccelerationStructureTypeKHR::eTopLevel;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createAccelerationStructureKHR(&vkCreateInfo, nullptr, &m_vkAccelerationStructure, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  SetResourceState(xiiGALResourceStateFlags::Undefined);

  return XII_SUCCESS;
}

void xiiGALTopLevelASVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (m_vkAccelerationStructure != VK_NULL_HANDLE)
  {
    xiiStringBuilder asName;
    asName.SetFormat("{} (TLAS)", sName);
    pDeviceVulkan->SetVulkanObjectDebugName(m_vkAccelerationStructure, asName.GetData());
  }

  if (m_vkBuffer != VK_NULL_HANDLE)
  {
    xiiStringBuilder bufferName;
    bufferName.SetFormat("{} (TLAS Buffer)", sName);
    pDeviceVulkan->SetVulkanObjectDebugName(m_vkBuffer, bufferName.GetData(), m_BufferMemoryAllocation);
  }
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASVulkan::GetInstanceDescription(xiiStringView sName) const
{
  xiiGALTopLevelASInstanceDescription instanceDescription;
  instanceDescription.m_uiContributionToHitGroupIndex = xiiInvalidIndex;
  instanceDescription.m_uiInstanceIndex               = xiiInvalidIndex;

  if (sName.IsEmpty())
    return instanceDescription;

  m_NameToInstance.TryGetValue(sName, instanceDescription);
  return instanceDescription;
}

xiiGALTopLevelASBuildDescription xiiGALTopLevelASVulkan::GetBuildDescription() const
{
  return m_BuildDescription;
}

xiiGALScratchBufferSizeDescription xiiGALTopLevelASVulkan::GetScratchBufferSizeDescription() const
{
  return m_ScratchBufferSizeDescription;
}

vk::DeviceAddress xiiGALTopLevelASVulkan::GetVulkanDeviceAddress() const
{
  if (m_vkAccelerationStructure == VK_NULL_HANDLE)
    return 0U;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  vk::AccelerationStructureDeviceAddressInfoKHR vkAddressInfo = {};
  vkAddressInfo.accelerationStructure                          = m_vkAccelerationStructure;

  return pDeviceVulkan->GetVulkanLogicalDevice().getAccelerationStructureAddressKHR(&vkAddressInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TopLevelASVulkan);
