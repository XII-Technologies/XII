/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferVulkan::xiiGALBufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (m_vkBuffer != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkBuffer), std::move(m_BufferMemoryAllocation));

    m_vkBuffer               = VK_NULL_HANDLE;
    m_BufferMemoryAllocation = VK_NULL_HANDLE;
  }
}

xiiResult xiiGALBufferVulkan::InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  XII_IGNORE_UNUSED(externalMemoryKind);

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();

  vk::BufferCreateInfo vkBufferCreateInfo                = {};
  vkBufferCreateInfo.pNext                               = nullptr;
  vkBufferCreateInfo.flags                               = {};
  vkBufferCreateInfo.size                                = m_Description.m_uiSize;
  vkBufferCreateInfo.usage                               = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
  const xiiArrayPtr<const xiiUInt32> activeQueueFamilies = pDeviceVulkan->GetActiveQueueFamilyIndices();
  vkBufferCreateInfo.sharingMode                         = activeQueueFamilies.GetCount() > 1U ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
  vkBufferCreateInfo.pQueueFamilyIndices                 = activeQueueFamilies.GetCount() > 1U ? activeQueueFamilies.GetPtr() : nullptr;
  vkBufferCreateInfo.queueFamilyIndexCount               = activeQueueFamilies.GetCount() > 1U ? activeQueueFamilies.GetCount() : 0U;

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

  constexpr vk::BufferUsageFlags usageFlagsThatRequireBackingBuffer = vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
  const bool                     bRequiresBackingBuffer             = (vkBufferCreateInfo.usage & usageFlagsThatRequireBackingBuffer) ||
    // We only need a backing buffer for the storage buffer if there is an unordered access bind flag (aka RW structured buffers).
    // Read-only storage buffers (aka structured buffers) don't need a backing buffer.
    ((vkBufferCreateInfo.usage & vk::BufferUsageFlagBits::eStorageBuffer) && m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    vkBufferCreateInfo.flags = vk::BufferCreateFlagBits::eSparseBinding | vk::BufferCreateFlagBits::eSparseResidency | (m_Description.m_MiscFlags.IsSet(xiiGALMiscBufferFlags::SparseAlias) ? vk::BufferCreateFlagBits::eSparseAliased : static_cast<vk::BufferCreateFlagBits>(0U));

    xiiVulkanAllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    SetResourceState(xiiGALResourceStateFlags::Undefined);
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Dynamic && !bRequiresBackingBuffer)
  {
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

    xiiVulkanAllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.m_Usage         = xiiVulkanMemoryUsage::Auto;
    allocationCreateInfo.m_RequiredFlags = xiiVulkanMemoryPropertyFlags::HostCoherent;
    allocationCreateInfo.m_Flags         = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    // Dynamic buffer memory is always host-coherent.
    m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
  }
  else
  {
    xiiVulkanAllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    SetResourceState(xiiVulkanTypeConversions::GetResourceStateFromBindFlags(m_Description.m_BindFlags));

    if (pInitialData != nullptr && pInitialData->m_pData != nullptr && pInitialData->m_uiDataSize > 0)
    {
      auto UploadStagingData = [&](xiiGALCommandListVulkan* pCommandListVulkan) -> xiiResult {
        // The allocation will stay in the upload heap until the end of the frame at which point all upload pages will be discarded.
        xiiGALStagingBufferAllocationVulkan stagingBufferAllocation = pCommandListVulkan->GetVulkanUploadStagingBufferPool()->Allocate(pInitialData->m_uiDataSize);
        void*                               pMappedMemory           = nullptr;

        VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(stagingBufferAllocation.m_VulkanAllocation, &pMappedMemory));
        VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, pInitialData->m_uiDataSize));

        pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);
        xiiMemoryUtils::RawByteCopy(pMappedMemory, pInitialData->m_pData, pInitialData->m_uiDataSize);

        VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, pInitialData->m_uiDataSize));
        pVulkanMemoryAllocator->UnmapMemory(stagingBufferAllocation.m_VulkanAllocation);

        pCommandListVulkan->UpdateBufferRegion(this, stagingBufferAllocation.m_vkBuffer, stagingBufferAllocation.m_uiOffset, 0U, pInitialData->m_uiDataSize);

        return XII_SUCCESS;
      };

      if (auto pCommandListVulkan = xiiDynamicCast<xiiGALCommandListVulkan*>(pInitialData->m_pCommandList))
      {
        XII_SUCCEED_OR_RETURN(UploadStagingData(pCommandListVulkan));
      }
      else if (auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics))
      {
        if (auto pImmediateCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
        {
          pImmediateCommandListVulkan->Begin();
          {
            XII_SUCCEED_OR_RETURN(UploadStagingData(pImmediateCommandListVulkan));
          }
          pImmediateCommandListVulkan->End();

          pCommandQueue->Submit(pImmediateCommandListVulkan);
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALBufferView> xiiGALBufferVulkan::CreateViewPlatform(const xiiGALBufferViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan>                 pDeviceVulkan     = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiInternal::NewInstance<xiiGALBufferViewVulkan> pBufferViewVulkan = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALBufferViewVulkan, pDeviceVulkan, xiiSharedPtr<xiiGALBuffer>(this, pDeviceVulkan->GetAllocator()), description);

  if (pBufferViewVulkan->InitPlatform().Succeeded())
    return pBufferViewVulkan;

  XII_DELETE(pBufferViewVulkan.m_pAllocator, pBufferViewVulkan.m_pInstance);

  return pBufferViewVulkan;
}

void xiiGALBufferVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkBuffer, sName.GetData(tmp), m_BufferMemoryAllocation);
}

void xiiGALBufferVulkan::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_vkBuffer == VK_NULL_HANDLE)
    return;

  VerifyInvalidateMappedRangeArguments(uiStartOffset, uiSize);

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanMemoryAllocator()->FlushAllocation(m_BufferMemoryAllocation, uiStartOffset, uiSize));
}

void xiiGALBufferVulkan::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_vkBuffer == VK_NULL_HANDLE)
    return;

  VerifyInvalidateMappedRangeArguments(uiStartOffset, uiSize);

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanMemoryAllocator()->InvalidateAllocation(m_BufferMemoryAllocation, uiStartOffset, uiSize));
}

xiiGALSparseBufferProperties xiiGALBufferVulkan::GetSparseProperties() const
{
  XII_ASSERT_DEV(m_Description.m_Usage == xiiGALResourceUsage::Sparse, "xiiGALBuffer::GetSparseProperties() must be used for sparse buffer.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::MemoryRequirements vkMemoryRequirements;
  vkLogicalDevice.getBufferMemoryRequirements(m_vkBuffer, &vkMemoryRequirements, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

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

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

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
