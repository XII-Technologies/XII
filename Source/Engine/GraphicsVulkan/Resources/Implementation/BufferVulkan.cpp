/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Utilities/ExternalMemoryVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferVulkan::xiiGALBufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkExternalMemorySemaphore));
  if (m_bOwnsExternalHandles)
  {
    m_ExternalMemoryDescription.CloseHandles();
  }

  if (m_vkBuffer)
  {
    if (m_ExternalMemoryKind == xiiGALExternalMemoryKind::Imported)
    {
      if (m_vkImportedMemory)
      {
        pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkBuffer), VK_NULL_HANDLE, std::move(m_vkImportedMemory));
      }
      else
      {
        pDeviceVulkan->GetVulkanLogicalDevice().destroyBuffer(m_vkBuffer, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
    }
    else
    {
      pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkBuffer), std::move(m_BufferMemoryAllocation));
    }
  }
}

xiiUInt64 xiiGALBufferVulkan::GetMemoryConsumption() const
{
  const auto* device = static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow());
  return m_BufferMemoryAllocation ? device->GetVulkanMemoryAllocator()->GetAllocationInfo(m_BufferMemoryAllocation).m_uiSize : m_Description.m_uiSize;
}

xiiResult xiiGALBufferVulkan::InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
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

  if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
  {
    if (pInitialData || m_Description.m_Usage != xiiGALResourceUsage::Mutable ||
        m_Description.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing) ||
        (externalMemoryKind != xiiGALExternalMemoryKind::Imported && externalMemoryKind != xiiGALExternalMemoryKind::Exportable))
    {
      return XII_FAILURE;
    }
    return InitializeExternalBuffer(vkBufferCreateInfo, externalMemoryKind);
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
    allocationCreateInfo.m_MemoryCategory = m_Description.m_MemoryCategory;
    allocationCreateInfo.m_Usage          = xiiVulkanMemoryUsage::Auto;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    SetResourceState(xiiGALResourceStateFlags::Undefined);
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Staging)
  {
    xiiVulkanAllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.m_MemoryCategory = m_Description.m_MemoryCategory;
    allocationCreateInfo.m_Usage          = xiiVulkanMemoryUsage::Auto;
    allocationCreateInfo.m_MemoryCategory = m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read) ? xiiGALMemoryCategory::Readback : xiiGALMemoryCategory::Upload;
    allocationCreateInfo.m_RequiredFlags  = xiiVulkanMemoryPropertyFlags::HostVisible;
    allocationCreateInfo.m_Flags          = m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read) ? xiiVulkanAllocationCreateFlags::StrategyHostRandom : xiiVulkanAllocationCreateFlags::StrategyHostSequential;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    SetResourceState(m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read) ? xiiGALResourceStateFlags::CopyDestination : xiiGALResourceStateFlags::CopySource);
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
    allocationCreateInfo.m_MemoryCategory = m_Description.m_MemoryCategory;
    allocationCreateInfo.m_Usage          = xiiVulkanMemoryUsage::Auto;
    allocationCreateInfo.m_RequiredFlags  = xiiVulkanMemoryPropertyFlags::HostCoherent;
    allocationCreateInfo.m_Flags          = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation));

    // Dynamic buffer memory is always host-coherent.
    m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
  }
  else
  {
    bool* pStreamingDeferred = pInitialData != nullptr ? pInitialData->m_pStreamingDeferred : nullptr;
    {
      xiiVulkanAllocationCreateInfo allocationCreateInfo;
      allocationCreateInfo.m_MemoryCategory = m_Description.m_MemoryCategory;
      allocationCreateInfo.m_Usage          = xiiVulkanMemoryUsage::Auto;

      xiiUInt64 uiAllocationBytes = 0U;
      if (pStreamingDeferred != nullptr)
      {
        const auto& dispatch = pDeviceVulkan->GetVulkanDynamicDispatchLoader();
        const auto& device   = pDeviceVulkan->GetVulkanLogicalDevice();
        if (pDeviceVulkan->GetVulkanVersion() >= VK_API_VERSION_1_3 &&
            pDeviceVulkan->GetVulkanPhysicalDeviceProperties().apiVersion >= VK_API_VERSION_1_3 &&
            dispatch.vkGetDeviceBufferMemoryRequirements != nullptr)
        {
          vk::DeviceBufferMemoryRequirements query;
          query.pCreateInfo = &vkBufferCreateInfo;
          vk::MemoryRequirements2 requirements;
          device.getBufferMemoryRequirements(&query, &requirements, dispatch);
          uiAllocationBytes = requirements.memoryRequirements.size;
        }
        else
        {
          // Vulkan 1.2 exposes the exact requirements once the unbound buffer exists.
          vk::Buffer buffer;
          VK_SUCCEED_OR_RETURN_XII_FAILURE(device.createBuffer(&vkBufferCreateInfo, nullptr, &buffer, dispatch));
          vk::MemoryRequirements requirements;
          device.getBufferMemoryRequirements(buffer, &requirements, dispatch);
          device.destroyBuffer(buffer, nullptr, dispatch);
          uiAllocationBytes = requirements.size;
        }
        if (uiAllocationBytes > pInitialData->m_uiMaxStreamingAllocationBytes || !pDeviceVulkan->TryReserveStreamingAllocation(uiAllocationBytes))
        {
          *pStreamingDeferred = true;
          return XII_FAILURE;
        }
        allocationCreateInfo.m_RequiredFlags = xiiVulkanMemoryPropertyFlags::DeviceLocal;
        allocationCreateInfo.m_Flags.Add(xiiVulkanAllocationCreateFlags::StrategyWithinBudget);
      }
      XII_SCOPE_EXIT(if (pStreamingDeferred != nullptr) pDeviceVulkan->ReleaseStreamingAllocationReservation(uiAllocationBytes));

      const vk::Result result = pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, m_vkBuffer, m_BufferMemoryAllocation);
      if (pStreamingDeferred != nullptr && result == vk::Result::eErrorOutOfDeviceMemory)
      {
        *pStreamingDeferred = true;
        return XII_FAILURE;
      }
      VK_SUCCEED_OR_RETURN_XII_FAILURE(result);
    }

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

xiiResult xiiGALBufferVulkan::InitializeExternalBuffer(const vk::BufferCreateInfo& createInfo, xiiBitflags<xiiGALExternalMemoryKind> kind)
{
  auto       pDevice    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const auto handleType = xiiVulkanExternalMemory::GetHandleType();
  if (handleType == vk::ExternalMemoryHandleTypeFlagBits{} ||
      !m_Description.m_ExternalMemory.m_Flags.IsSet(xiiGALExternalMemoryFlags::BinarySemaphore))
  {
    return XII_FAILURE;
  }

  vk::PhysicalDeviceExternalBufferInfo query;
  query.flags      = createInfo.flags;
  query.usage      = createInfo.usage;
  query.handleType = handleType;
  vk::ExternalBufferProperties properties;
  const auto&                  dispatch = pDevice->GetVulkanDynamicDispatchLoader();
  pDevice->GetVulkanPhysicalDevice().getExternalBufferProperties(&query, &properties, dispatch);
  const auto required = kind == xiiGALExternalMemoryKind::Imported ? vk::ExternalMemoryFeatureFlagBits::eImportable : vk::ExternalMemoryFeatureFlagBits::eExportable;
  if (!(properties.externalMemoryProperties.externalMemoryFeatures & required))
  {
    return XII_FAILURE;
  }

  vk::ExternalMemoryBufferCreateInfo external;
  external.handleTypes                     = handleType;
  auto externalCreateInfo                  = createInfo;
  external.pNext                           = externalCreateInfo.pNext;
  externalCreateInfo.pNext                 = &external;
  externalCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
  externalCreateInfo.queueFamilyIndexCount = 0;
  externalCreateInfo.pQueueFamilyIndices   = nullptr;

  xiiVulkanAllocationInfo allocationInfo = {};
  m_ExternalMemoryKind                   = kind;
  m_ExternalMemoryDescription            = m_Description.m_ExternalMemory;
  if (kind == xiiGALExternalMemoryKind::Imported)
  {
    VK_SUCCEED_OR_RETURN_XII_FAILURE(pDevice->GetVulkanLogicalDevice().createBuffer(&externalCreateInfo, nullptr, &m_vkBuffer, dispatch));
  }
  else
  {
    xiiVulkanAllocationCreateInfo allocation;
    allocation.m_MemoryCategory          = m_Description.m_MemoryCategory;
    allocation.m_Usage                   = xiiVulkanMemoryUsage::Auto;
    allocation.m_RequiredFlags           = xiiVulkanMemoryPropertyFlags::DeviceLocal;
    allocation.m_bExportSharedAllocation = true;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(pDevice->GetVulkanMemoryAllocator()->CreateBuffer(externalCreateInfo, allocation, m_vkBuffer, m_BufferMemoryAllocation, &allocationInfo));
  }

  vk::MemoryDedicatedAllocateInfo dedicated;
  dedicated.buffer = m_vkBuffer;
  XII_SUCCEED_OR_RETURN(xiiVulkanExternalMemory::Initialize(*pDevice, kind, allocationInfo, dedicated, m_ExternalMemoryDescription,
                                                            m_vkExternalMemorySemaphore, m_vkImportedMemory, m_bOwnsExternalHandles));
  SetResourceState(xiiGALResourceStateFlags::Common);
  return XII_SUCCESS;
}

xiiResult xiiGALBufferVulkan::AcquireExternalMemory(xiiGALCommandList* pCommandList)
{
  return TransferExternalBuffer(pCommandList, true);
}

xiiResult xiiGALBufferVulkan::ReleaseExternalMemory(xiiGALCommandList* pCommandList)
{
  return TransferExternalBuffer(pCommandList, false);
}

xiiResult xiiGALBufferVulkan::TransferExternalBuffer(xiiGALCommandList* pCommandList, bool bAcquire)
{
  auto  pDevice  = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  auto* pCommand = xiiVulkanExternalMemory::GetTransferCommandList(pCommandList, *pDevice, m_ExternalMemoryDescription, m_vkExternalMemorySemaphore);
  if (!pCommand)
  {
    return XII_FAILURE;
  }

  const auto queueFlags = pCommand->GetDescription().m_QueueFlags;
  if (!queueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute | xiiGALCommandQueueFlags::Transfer))
  {
    return XII_FAILURE;
  }

  const xiiUInt32         uiQueueFamily = pDevice->GetCommandQueueInformation(queueFlags).m_uiQueueFamilyIndex;
  constexpr auto          access        = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
  vk::BufferMemoryBarrier barrier;
  barrier.buffer              = m_vkBuffer;
  barrier.offset              = 0;
  barrier.size                = VK_WHOLE_SIZE;
  barrier.srcQueueFamilyIndex = bAcquire ? VK_QUEUE_FAMILY_EXTERNAL : uiQueueFamily;
  barrier.dstQueueFamilyIndex = bAcquire ? uiQueueFamily : VK_QUEUE_FAMILY_EXTERNAL;
  barrier.srcAccessMask       = bAcquire ? vk::AccessFlags{} : access;
  barrier.dstAccessMask       = bAcquire ? access : vk::AccessFlags{};
  if (bAcquire)
  {
    pCommand->AddWaitSemaphore(m_vkExternalMemorySemaphore, vk::PipelineStageFlagBits::eAllCommands);
  }
  pCommand->FlushBarriers();
  pCommand->GetVulkanCommandBuffer().pipelineBarrier(
    bAcquire ? vk::PipelineStageFlagBits::eTopOfPipe : vk::PipelineStageFlagBits::eAllCommands,
    bAcquire ? vk::PipelineStageFlagBits::eAllCommands : vk::PipelineStageFlagBits::eBottomOfPipe,
    {}, 0, nullptr, 1, &barrier, 0, nullptr, pDevice->GetVulkanDynamicDispatchLoader());
  if (!bAcquire)
  {
    pCommand->AddSignalSemaphore(m_vkExternalMemorySemaphore);
  }
  SetResourceState(xiiGALResourceStateFlags::Common);
  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALBufferView> xiiGALBufferVulkan::CreateViewPlatform(const xiiGALBufferViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan>                 pDeviceVulkan     = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiInternal::NewInstance<xiiGALBufferViewVulkan> pBufferViewVulkan = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALBufferViewVulkan, pDeviceVulkan, xiiSharedPtr<xiiGALBuffer>(this, pDeviceVulkan->GetAllocator()), description);

  if (pBufferViewVulkan->InitPlatform().Succeeded())
    return pBufferViewVulkan;

  XII_DELETE(pBufferViewVulkan.m_pAllocator, pBufferViewVulkan.m_pInstance);

  return nullptr;
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
