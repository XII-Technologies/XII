#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>

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

  [[nodiscard]] static vk::Format ConvertTriangleVertexFormat(const xiiGALBLASTriangleDescription& triangle)
  {
    if (triangle.m_VertexValueType == xiiGALValueType::Float32)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;
    }

    if (triangle.m_VertexValueType == xiiGALValueType::Float16)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR16G16Sfloat : vk::Format::eR16G16B16Sfloat;
    }

    if (triangle.m_VertexValueType == xiiGALValueType::Int32)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sint : vk::Format::eR32G32B32Sint;
    }

    return vk::Format::eUndefined;
  }
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBottomLevelASVulkan::xiiGALBottomLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALBottomLevelASVulkan::~xiiGALBottomLevelASVulkan()
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

xiiResult xiiGALBottomLevelASVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  const auto& extensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();
  if (extensionFeatures.m_AccelerationStructure.accelerationStructure == vk::False)
  {
    xiiLog::Error("Failed to create BLAS: VK_KHR_acceleration_structure is not enabled.");
    return XII_FAILURE;
  }

  xiiUInt64 uiAccelerationStructureSize = m_Description.m_uiCompactedSize;
  m_ScratchBufferSizeDescription        = {};

  if (uiAccelerationStructureSize == 0U)
  {
    xiiDynamicArray<vk::AccelerationStructureGeometryKHR> vkGeometries(pDeviceVulkan->GetAllocator());
    xiiDynamicArray<xiiUInt32>                            primitiveCounts(pDeviceVulkan->GetAllocator());

    vkGeometries.Reserve(m_Description.m_Triangles.GetCount() + m_Description.m_BoundingBoxes.GetCount());
    primitiveCounts.Reserve(vkGeometries.GetCount());

    for (const xiiGALBLASTriangleDescription& triangle : m_Description.m_Triangles)
    {
      vk::AccelerationStructureGeometryTrianglesDataKHR triangleData = {};
      triangleData.vertexFormat                                      = ConvertTriangleVertexFormat(triangle);
      triangleData.vertexData.deviceAddress                          = 0U;
      triangleData.vertexStride                                      = 0U;
      triangleData.maxVertex                                         = triangle.m_uiMaxVertexCount;
      triangleData.indexType                                         = triangle.m_IndexType == xiiGALValueType::UInt16 ? vk::IndexType::eUint16 : (triangle.m_IndexType == xiiGALValueType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eNoneKHR);
      triangleData.indexData.deviceAddress                           = 0U;
      triangleData.transformData.deviceAddress                       = 0U;

      vk::AccelerationStructureGeometryDataKHR geometryData = {};
      geometryData.triangles                                = triangleData;

      vk::AccelerationStructureGeometryKHR geometry = {};
      geometry.geometryType                           = vk::GeometryTypeKHR::eTriangles;
      geometry.geometry                               = geometryData;
      geometry.flags                                  = vk::GeometryFlagBitsKHR::eOpaque;

      vkGeometries.PushBack(geometry);
      primitiveCounts.PushBack(triangle.m_uiMaxPrimitiveCount);
    }

    for (const xiiGALBLASBoundingBoxDescription& boundingBox : m_Description.m_BoundingBoxes)
    {
      vk::AccelerationStructureGeometryAabbsDataKHR aabbData = {};
      aabbData.data.deviceAddress                             = 0U;
      aabbData.stride                                         = sizeof(float) * 6U;

      vk::AccelerationStructureGeometryDataKHR geometryData = {};
      geometryData.aabbs                                    = aabbData;

      vk::AccelerationStructureGeometryKHR geometry = {};
      geometry.geometryType                           = vk::GeometryTypeKHR::eAabbs;
      geometry.geometry                               = geometryData;
      geometry.flags                                  = vk::GeometryFlagBitsKHR::eOpaque;

      vkGeometries.PushBack(geometry);
      primitiveCounts.PushBack(boundingBox.m_uiMaxBoxCount);
    }

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    buildInfo.type                                          = vk::AccelerationStructureTypeKHR::eBottomLevel;
    buildInfo.flags                                         = ConvertBuildFlags(m_Description.m_BuildASFlags);
    buildInfo.mode                                          = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.geometryCount                                 = vkGeometries.GetCount();
    buildInfo.pGeometries                                   = vkGeometries.GetData();

    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = {};
    vkLogicalDevice.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, primitiveCounts.GetData(), &sizeInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

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
  vkCreateInfo.type                                   = vk::AccelerationStructureTypeKHR::eBottomLevel;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createAccelerationStructureKHR(&vkCreateInfo, nullptr, &m_vkAccelerationStructure, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  SetResourceState(xiiGALResourceStateFlags::Undefined);

  return XII_SUCCESS;
}

void xiiGALBottomLevelASVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (m_vkAccelerationStructure != VK_NULL_HANDLE)
  {
    xiiStringBuilder asName;
    asName.SetFormat("{} (BLAS)", sName);
    pDeviceVulkan->SetVulkanObjectDebugName(m_vkAccelerationStructure, asName.GetData());
  }

  if (m_vkBuffer != VK_NULL_HANDLE)
  {
    xiiStringBuilder bufferName;
    bufferName.SetFormat("{} (BLAS Buffer)", sName);
    pDeviceVulkan->SetVulkanObjectDebugName(m_vkBuffer, bufferName.GetData(), m_BufferMemoryAllocation);
  }
}

vk::DeviceAddress xiiGALBottomLevelASVulkan::GetVulkanDeviceAddress() const
{
  if (m_vkAccelerationStructure == VK_NULL_HANDLE)
    return 0U;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  vk::AccelerationStructureDeviceAddressInfoKHR vkAddressInfo = {};
  vkAddressInfo.accelerationStructure                          = m_vkAccelerationStructure;

  return pDeviceVulkan->GetVulkanLogicalDevice().getAccelerationStructureAddressKHR(&vkAddressInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BottomLevelASVulkan);
