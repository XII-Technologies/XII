#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBufferVulkan::xiiGALBufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(pDeviceVulkan, creationDescription)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan() = default;

xiiResult xiiGALBufferVulkan::InitPlatform(const xiiGALBufferData* pInitialData)
{
  xiiGALDeviceVulkan*                 pDeviceVulkan              = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::PhysicalDevice                  vkPhysicalDevice           = pDeviceVulkan->GetVulkanPhysicalDevice();
  vk::Device                          vkLogicalDevice            = pDeviceVulkan->GetVulkanLogicalDevice();
  const vk::PhysicalDeviceProperties& vkPhysicalDeviceProperties = pDeviceVulkan->GetVulkanPhysicalDeviceProperties();

  vk::BufferCreateInfo vkBufferCreateInfo  = {};
  vkBufferCreateInfo.pNext                 = nullptr;
  vkBufferCreateInfo.flags                 = {};
  vkBufferCreateInfo.size                  = m_Description.m_uiSize;
  vkBufferCreateInfo.usage                 = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
  vkBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive; // Sharing mode of the buffer when it is accessed by multiple queue families.
  vkBufferCreateInfo.pQueueFamilyIndices   = nullptr;                     // The list of queue families that will access this buffer (ignored if sharingMode is not vk::SharingMode::eConcurrent).
  vkBufferCreateInfo.queueFamilyIndexCount = 0U;                          // The number of entries in the pQueueFamilyIndices array.

  for (xiiUInt32 uiBit : m_Description.m_BindFlags)
  {
    switch (uiBit)
    {
      case xiiGALBindFlags::ShaderResource:
      {
        if (m_Description.m_Mode == xiiGALBufferMode::Formatted)
        {
          // Formatted buffers are mapped to uniform texel buffers in Vulkan.
          vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        }
        else
        {
          // Structured and ByteAddress buffers are mapped to read-only storage buffers in Vulkan.
          vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
      }
      break;
      case xiiGALBindFlags::UnorderedAccess:
      {
        if (m_Description.m_Mode == xiiGALBufferMode::Formatted)
        {
          // Read-Write formatted buffers are mapped to storage texel buffers in Vulkan.
          vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }
        else
        {
          // Read-Write Structured and Read-Write ByteAddress buffers are mapped to storage buffers in Vulkan.
          vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
      }
      break;
      case xiiGALBindFlags::VertexBuffer:
      {
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
      }
      break;
      case xiiGALBindFlags::IndexBuffer:
      {
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
      }
      break;
      case xiiGALBindFlags::IndirectDrawArguments:
      {
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
      }
      break;
      case xiiGALBindFlags::UniformBuffer:
      {
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
      }
      break;
      case xiiGALBindFlags::RayTracing:
      {
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eStorageBuffer; // For scratch buffer.
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
        vkBufferCreateInfo.usage |= vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR; // Acceleration structure build inputs such as vertex, index, transform, aabb, and instance data.
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  /// \todo GraphicsVulkan: Selectively utilize vk::SharingMode::eConcurrent for multiple queue family's ownership of the Vulkan buffer.
#if 0
  xiiHybridArray<xiiUInt32, 3U> queueFamilies;
  queueFamilies.PushBack(pDeviceVulkan->GetGraphicsQueueInformation().m_uiQueueFamilyIndex);
  if (pDeviceVulkan->GetComputeQueueInformation().m_uiQueueFamilyIndex != xiiInvalidIndex && !queueFamilies.Contains(pDeviceVulkan->GetComputeQueueInformation().m_uiQueueFamilyIndex))
  {
    queueFamilies.PushBack(pDeviceVulkan->GetComputeQueueInformation().m_uiQueueFamilyIndex);
  }
  if (pDeviceVulkan->GetTransferQueueInformation().m_uiQueueFamilyIndex != xiiInvalidIndex && !queueFamilies.Contains(pDeviceVulkan->GetTransferQueueInformation().m_uiQueueFamilyIndex))
  {
    queueFamilies.PushBack(pDeviceVulkan->GetTransferQueueInformation().m_uiQueueFamilyIndex);
  }

  if (queueFamilies.GetCount() > 1U)
  {
    // If sharingMode is vk::SharingMode::eConcurrent, queueFamilyIndexCount must be greater than 1.
    vkBufferCreateInfo.sharingMode           = vk::SharingMode::eConcurrent;
    vkBufferCreateInfo.pQueueFamilyIndices   = queueFamilies.GetData();
    vkBufferCreateInfo.queueFamilyIndexCount = queueFamilies.GetCount();
  }
#endif

  constexpr vk::BufferUsageFlags usageFlagsThatRequireBackingBuffer = vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
  const bool                     bRequiresBackingBuffer             = (vkBufferCreateInfo.usage & usageFlagsThatRequireBackingBuffer) ||
    // We only need a backing buffer for the storage buffer if there is an unordered access bind flag (aka RW structured buffers).
    // Read-only storage buffers (aka structured buffers) don't need a backing buffer.
    ((vkBufferCreateInfo.usage & vk::BufferUsageFlagBits::eStorageBuffer) && m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));

  if (m_Description.m_ResourceUsage == xiiGALResourceUsage::Sparse)
  {
    vkBufferCreateInfo.flags = vk::BufferCreateFlagBits::eSparseBinding | vk::BufferCreateFlagBits::eSparseResidency | (m_Description.m_MiscFlags.IsSet(xiiGALMiscBufferFlags::SparseAlias) ? vk::BufferCreateFlagBits::eSparseAliased : static_cast<vk::BufferCreateFlagBits>(0U));

    VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
    vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    vmaAllocationCreateInfo.flags                   = {}; // TODO

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateBuffer(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&m_vkBuffer), &m_BufferMemoryAllocation, nullptr));

    SetResourceState(xiiGALResourceStateFlags::Undefined);
  }
  else if (m_Description.m_ResourceUsage == xiiGALResourceUsage::Dynamic && !bRequiresBackingBuffer)
  {
    XII_ASSERT_DEV(vkBufferCreateInfo.sharingMode == vk::SharingMode::eExclusive, "Sharing mode is not supported for dynamic buffers, this should have caused buffer creation failure.");

    // Dynamic constant/vertex/index/structured buffers are suballocated in the upload heap when Map() is called.
    // Dynamic formatted buffers or writable buffers need to be allocated in GPU-local memory.
    xiiBitflags<xiiGALResourceStateFlags> state = xiiGALResourceStateFlags::VertexBuffer | xiiGALResourceStateFlags::IndexBuffer | xiiGALResourceStateFlags::ConstantBuffer | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::IndirectArgument;
    SetResourceState(state);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    {
      constexpr vk::AccessFlags vkAccessFlags = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead;

      XII_ASSERT_DEBUG(xiiVulkanTypeConversions::GetAccessFlags(state) == vkAccessFlags, "");
    }
#endif

    VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
    vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    vmaAllocationCreateInfo.requiredFlags           = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateBuffer(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&m_vkBuffer), &m_BufferMemoryAllocation, nullptr));

    // Dynamic buffer memory is always host-coherent.
    m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
  }
  else
  {
    XII_ASSERT_DEV(m_Description.m_ResourceUsage != xiiGALResourceUsage::Dynamic && xiiMath::CountBits(m_Description.m_uiCommandQueueMask) <= 1U, "The command queue mask must contain a single set bit, this error should have been caught in buffer validation.");

    VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
    vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    vmaAllocationCreateInfo.requiredFlags           = {}; // TODO

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateBuffer(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&m_vkBuffer), &m_BufferMemoryAllocation, nullptr));
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBufferVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkBuffer != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkBuffer, m_BufferMemoryAllocation);

    m_vkBuffer               = VK_NULL_HANDLE;
    m_BufferMemoryAllocation = VK_NULL_HANDLE;
  }
  return XII_SUCCESS;
}

void xiiGALBufferVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkBuffer, sName.GetData(tmp));
}

void xiiGALBufferVulkan::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_vkBuffer == VK_NULL_HANDLE)
    return;

  VerifyInvalidateMappedRangeArguments(uiStartOffset, uiSize);

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  VK_ASSERT_DEV(vmaFlushAllocation(pDeviceVulkan->GetVulkanMemoryAllocator(), m_BufferMemoryAllocation, uiStartOffset, uiSize));
}

void xiiGALBufferVulkan::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_vkBuffer == VK_NULL_HANDLE)
    return;

  VerifyInvalidateMappedRangeArguments(uiStartOffset, uiSize);

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  VK_ASSERT_DEV(vmaInvalidateAllocation(pDeviceVulkan->GetVulkanMemoryAllocator(), m_BufferMemoryAllocation, uiStartOffset, uiSize));
}

xiiGALSparseBufferProperties xiiGALBufferVulkan::GetSparseProperties() const
{
  XII_ASSERT_DEV(m_Description.m_ResourceUsage == xiiGALResourceUsage::Sparse, "xiiGALBuffer::GetSparseProperties() must be used for sparse buffer.");

  xiiGALDeviceVulkan*    pDeviceVulkan        = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::MemoryRequirements vkMemoryRequirements = pDeviceVulkan->GetVulkanLogicalDevice().getBufferMemoryRequirements(GetVulkanBuffer(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  xiiGALSparseBufferProperties sparseBufferProperties = {};
  sparseBufferProperties.m_uiAddressSpaceSize         = vkMemoryRequirements.size;
  sparseBufferProperties.m_uiBlockSize                = static_cast<xiiUInt32>(vkMemoryRequirements.alignment);

  return sparseBufferProperties;
}

vk::DeviceAddress xiiGALBufferVulkan::GetVulkanBufferDeviceAddress() const
{
  xiiBitflags<xiiGALBindFlags> deviceAddressFlags = xiiGALBindFlags::RayTracing;

  if (m_vkBuffer == VK_NULL_HANDLE || !m_Description.m_BindFlags.IsAnySet(deviceAddressFlags))
    return 0U;

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::BufferDeviceAddressInfo vkBufferInfo = {};
  vkBufferInfo.pNext                       = nullptr;
  vkBufferInfo.buffer                      = m_vkBuffer;

  vk::DeviceAddress vkBufferDeviceAddress = pDeviceVulkan->GetVulkanLogicalDevice().getBufferAddress(&vkBufferInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  XII_ASSERT_DEV(vkBufferDeviceAddress > 0, "Invalid buffer device address!");

  return vkBufferDeviceAddress;
}

void xiiGALBufferVulkan::SetAccessFlags(vk::AccessFlags accessFlags)
{
  SetResourceState(xiiVulkanTypeConversions::GetResourceState(accessFlags));
}

vk::AccessFlags xiiGALBufferVulkan::GetAccessFlags() const
{
  return xiiVulkanTypeConversions::GetAccessFlags(GetResourceState());
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BufferVulkan);
