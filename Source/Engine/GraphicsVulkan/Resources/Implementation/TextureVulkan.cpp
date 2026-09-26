/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/System/Process.h>
#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <errno.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  XII_ALWAYS_INLINE xiiBitflags<xiiVulkanMemoryPropertyFlags> FromVkMemoryPropertyFlags(vk::MemoryPropertyFlags vkFlags)
  {
    xiiBitflags<xiiVulkanMemoryPropertyFlags> flags;

    if (vkFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
      flags |= xiiVulkanMemoryPropertyFlags::DeviceLocal;
    if (vkFlags & vk::MemoryPropertyFlagBits::eHostVisible)
      flags |= xiiVulkanMemoryPropertyFlags::HostVisible;
    if (vkFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
      flags |= xiiVulkanMemoryPropertyFlags::HostCoherent;
    if (vkFlags & vk::MemoryPropertyFlagBits::eHostCached)
      flags |= xiiVulkanMemoryPropertyFlags::HostCached;
    if (vkFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
      flags |= xiiVulkanMemoryPropertyFlags::LazilyAllocated;
    if (vkFlags & vk::MemoryPropertyFlagBits::eProtected)
      flags |= xiiVulkanMemoryPropertyFlags::Protected;

    return flags;
  }
} // namespace

vk::ImageLayout xiiGALTextureVulkan::GetVulkanImageLayout() const
{
  xiiSharedPtr<xiiGALDeviceVulkan>                       pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const vk::PhysicalDeviceFragmentDensityMapFeaturesEXT& fragmentDensityMap = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures().m_FragmentDensityMap;
  return xiiVulkanTypeConversions::GetImageLayout(GetResourceState(), false, fragmentDensityMap.fragmentDensityMap != vk::False);
}

void xiiGALTextureVulkan::SetVulkanImageLayout(vk::ImageLayout vkImageLayout)
{
  SetResourceState(xiiVulkanTypeConversions::GetResourceState(vkImageLayout));
}

xiiGALTextureVulkan::xiiGALTextureVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(std::move(pDeviceVulkan), creationDescription), m_vkImage(VK_NULL_HANDLE), m_ImageMemoryAllocation(VK_NULL_HANDLE), m_vkStagingBuffer(VK_NULL_HANDLE), m_StagingBufferMemoryAllocation(VK_NULL_HANDLE)
{
}

xiiGALTextureVulkan::~xiiGALTextureVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkStagingBuffer), std::move(m_StagingBufferMemoryAllocation));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkExternalMemorySemaphore));

  // Prevent releasing the native object.
  if (m_vkImage != VK_NULL_HANDLE && m_Description.m_pExistingNativeObject == nullptr)
  {
    if (m_ExternalMemoryDescription.m_Type == xiiGALExternalMemoryKind::Imported)
    {
      pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImage), VK_NULL_HANDLE, std::move(m_ImageMemoryAllocationInfo.m_vkDeviceMemory));
    }
    else
    {
      pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImage), std::move(m_ImageMemoryAllocation));
    }
  }
}

xiiResult xiiGALTextureVulkan::InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();

  if (m_Description.m_Usage == xiiGALResourceUsage::Immutable && (pInitialData == nullptr || pInitialData->m_pSubResources.IsEmpty()))
  {
    xiiLog::Error("Immutable textures must be initialized with data at creation time. The given subresources cannot be empty.");
    return XII_FAILURE;
  }

  const bool bIsMemoryLess = m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless);
  if (bIsMemoryLess && pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty())
  {
    xiiLog::Error("Memoryless textures cannot be initialized with initial data.");
    return XII_FAILURE;
  }

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse && m_Description.Is3D() && (m_Description.m_BindFlags.IsAnySet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil)))
  {
    xiiLog::Error("Sparse 3D texture with xiiGALBindFlags::RenderTarget or xiiGALBindFlags::DepthStencil is not supported in Vulkan.");
    return XII_FAILURE;
  }

  const xiiGALResourceFormatDescription& resourceFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);

  if (m_Description.m_pExistingNativeObject != nullptr)
  {
    m_vkImage = static_cast<VkImage>(m_Description.m_pExistingNativeObject);

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    return XII_SUCCESS;
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Immutable || m_Description.m_Usage == xiiGALResourceUsage::Mutable || m_Description.m_Usage == xiiGALResourceUsage::Dynamic || m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    vk::ImageCreateInfo vkImageCreateInfo = {};
    ComputeVkImageCreateInfo(pDeviceVulkan, m_Description, vkImageCreateInfo);

    vk::ExternalMemoryImageCreateInfo vkExternalMemoryImageCreateInfo = {};
    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      if (pDeviceVulkan->GetFeatures().m_NativeFence != xiiGALDeviceFeatureState::Enabled)
      {
        xiiLog::Error("Exportable external memory for sparse textures requires the NativeFence device feature to be enabled.");
        return XII_FAILURE;
      }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      vkExternalMemoryImageCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
      vkExternalMemoryImageCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#endif

      vkExternalMemoryImageCreateInfo.pNext = vkImageCreateInfo.pNext;
      vkImageCreateInfo.pNext               = &vkExternalMemoryImageCreateInfo;
    }

    // initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED (11.4).
    // If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host while using this layout, and the transition away from this layout will preserve that data.
    // If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined, and the transition away from this layout is not guaranteed to preserve that data.
    vkImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    vk::ImageFormatProperties vkImageFormatProperties;
    VK_ASSERT_DEBUG(pDeviceVulkan->GetVulkanPhysicalDevice().getImageFormatProperties(vkImageCreateInfo.format, vkImageCreateInfo.imageType, vkImageCreateInfo.tiling, vkImageCreateInfo.usage, vkImageCreateInfo.flags, &vkImageFormatProperties, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
#endif

    if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    {
      xiiVulkanAllocationCreateInfo allocationCreateInfo;
      allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;

      if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Exportable))
      {
        allocationCreateInfo.m_bExportSharedAllocation = true;
      }

      if (externalMemoryKind == xiiGALExternalMemoryKind::None || externalMemoryKind == xiiGALExternalMemoryKind::Imported)
      {
        VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateImage(vkImageCreateInfo, allocationCreateInfo, m_vkImage, m_ImageMemoryAllocation, &m_ImageMemoryAllocationInfo));
      }
      else if (externalMemoryKind == xiiGALExternalMemoryKind::Imported)
      {
        vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

        VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createImage(&vkImageCreateInfo, nullptr, &m_vkImage, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
      }

      XII_SUCCEED_OR_RETURN(InitializeImageExternalMemoryProperties(externalMemoryKind));

      SetResourceState(xiiGALResourceStateFlags::Undefined);

      InitializeSparseTextureProperties();
    }
    else
    {
      xiiVulkanAllocationCreateInfo allocationCreateInfo;
      allocationCreateInfo.m_Usage         = xiiVulkanMemoryUsage::Auto;
      allocationCreateInfo.m_RequiredFlags = bIsMemoryLess ? xiiVulkanMemoryPropertyFlags::LazilyAllocated : xiiVulkanMemoryPropertyFlags::DeviceLocal;

      if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Exportable))
      {
        allocationCreateInfo.m_bExportSharedAllocation = true;
      }

      if (externalMemoryKind == xiiGALExternalMemoryKind::None || externalMemoryKind == xiiGALExternalMemoryKind::Imported)
      {
        VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->CreateImage(vkImageCreateInfo, allocationCreateInfo, m_vkImage, m_ImageMemoryAllocation, &m_ImageMemoryAllocationInfo));
      }
      else if (externalMemoryKind == xiiGALExternalMemoryKind::Imported)
      {
        vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

        VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createImage(&vkImageCreateInfo, nullptr, &m_vkImage, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
      }

      XII_SUCCEED_OR_RETURN(InitializeImageExternalMemoryProperties(externalMemoryKind));

      if (pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty())
      {
        InitializeImageContent(vkImageCreateInfo, resourceFormatProperties, pInitialData);
      }
      else
      {
        SetResourceState(xiiGALResourceStateFlags::Undefined);
      }
    }
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Staging)
  {
    VK_SUCCEED_OR_RETURN_XII_FAILURE(CreateVulkanStagingBuffer(pInitialData, resourceFormatProperties));
  }
  else
  {
    xiiLog::Error("Unsupported usage ({}) in creating Vulkan image.", m_Description.m_Usage);
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(IsInKnownState(), "The Vulkan image is not in known state.");

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALTextureView> xiiGALTextureVulkan::CreateViewPlatform(const xiiGALTextureViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan>                  pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiInternal::NewInstance<xiiGALTextureViewVulkan> pTextureViewVulkan = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALTextureViewVulkan, pDeviceVulkan, xiiSharedPtr<xiiGALTexture>(this, pDeviceVulkan->GetAllocator()), description);

  if (pTextureViewVulkan->InitPlatform().Succeeded())
    return pTextureViewVulkan;

  XII_DELETE(pTextureViewVulkan.m_pAllocator, pTextureViewVulkan.m_pInstance);

  return pTextureViewVulkan;
}

void xiiGALTextureVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkImage, sName.GetData(tmp));
}

vk::Result xiiGALTextureVulkan::CreateVulkanStagingBuffer(const xiiGALTextureData* pInitialData, const xiiGALResourceFormatDescription& formatProperties)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  const bool                       bInitializeTexture     = (pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty());

  vk::BufferCreateInfo vkStagingBufferCreateInfo = {};
  vkStagingBufferCreateInfo.pNext                = nullptr;
  vkStagingBufferCreateInfo.flags                = {};
  vkStagingBufferCreateInfo.size                 = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(m_Description, m_Description.m_uiArraySizeOrDepth, 0, s_uiStagingBufferOffsetAlignment);

  XII_ASSERT_DEV(m_Description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Read) || m_Description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Write), "Exactly one of xiiGALCPUAccessFlag::Read or xiiGALCPUAccessFlag::Write CPU access flags must be specified.");

  vk::MemoryPropertyFlags vkMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible;

  if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
  {
    XII_ASSERT_DEV(!bInitializeTexture, "Readback textures should not be initialized with data.");

    vkStagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst;

    vkMemoryPropertyFlags |= vk::MemoryPropertyFlagBits::eHostCached;

    SetResourceState(xiiGALResourceStateFlags::CopyDestination);

    // We do not set HOST_COHERENT bit, so we will have to use InvalidateMappedMemoryRanges, which requires the ranges to be aligned by nonCoherentAtomSize.
    const auto& deviceLimits = pDeviceVulkan->GetVulkanPhysicalDeviceProperties().limits;

    // Align the buffer size to ensure that any aligned range is always in bounds.
    vkStagingBufferCreateInfo.size = xiiMemoryUtils::AlignSize(vkStagingBufferCreateInfo.size, deviceLimits.nonCoherentAtomSize);
  }
  else if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
  {
    vkStagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
    // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible to the host (10.2).

    vkMemoryPropertyFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;

    SetResourceState(xiiGALResourceStateFlags::CopySource);
  }
  else
  {
    XII_REPORT_FAILURE("Unexpected CPU access flags.");
  }

  vkStagingBufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
  vkStagingBufferCreateInfo.pQueueFamilyIndices   = nullptr;
  vkStagingBufferCreateInfo.queueFamilyIndexCount = 0;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage         = xiiVulkanMemoryUsage::Auto;
  allocationCreateInfo.m_RequiredFlags = FromVkMemoryPropertyFlags(vkMemoryPropertyFlags);
  allocationCreateInfo.m_Flags         = xiiVulkanAllocationCreateFlags::StrategyHostSequential | xiiVulkanAllocationCreateFlags::Mapped;

  xiiVulkanAllocationInfo stagingBufferAllocationInfo;
  VK_SUCCEED_OR_RETURN_LOG(pVulkanMemoryAllocator->CreateBuffer(vkStagingBufferCreateInfo, allocationCreateInfo, m_vkStagingBuffer, m_StagingBufferMemoryAllocation, &stagingBufferAllocationInfo));
  XII_ASSERT_DEV(stagingBufferAllocationInfo.m_pMappedData != nullptr, "");

  if (bInitializeTexture)
  {
    xiiUInt32 uiSubResourceIndex = 0;

    for (xiiUInt32 uiLayer = 0; uiLayer < m_Description.m_uiArraySizeOrDepth; ++uiLayer)
    {
      for (xiiUInt32 uiMip = 0; uiMip < m_Description.m_uiMipLevels; ++uiMip)
      {
        const xiiGALTextureSubResourceData& subResourceData                = pInitialData->m_pSubResources[uiSubResourceIndex++];
        const xiiGALMipLevelProperties      mipLevelProperty               = xiiGALTextureUtilities::GetMipLevelProperties(m_Description, uiMip);
        const xiiUInt64                     uiDestinationSubresourceOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(m_Description, uiLayer, uiMip, s_uiStagingBufferOffsetAlignment);

        xiiGALTextureUtilities::CopyTextureSubresource(subResourceData, mipLevelProperty.m_StorageSize.height / formatProperties.m_uiBlockHeight, mipLevelProperty.m_uiDepth, mipLevelProperty.m_uiRowSize, xiiMemoryUtils::AddByteOffset(stagingBufferAllocationInfo.m_pMappedData, uiDestinationSubresourceOffset), mipLevelProperty.m_uiRowSize, mipLevelProperty.m_uiDepthSliceSize);
      }
    }
  }
  return vk::Result::eSuccess;
}

void xiiGALTextureVulkan::InitializeSparseTextureProperties()
{
  XII_ASSERT_DEV(m_Description.m_Usage == xiiGALResourceUsage::Sparse, "");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::MemoryRequirements vkMemoryRequirements;
  vkLogicalDevice.getImageMemoryRequirements(m_vkImage, &vkMemoryRequirements, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  // If the image was not created with VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT, then pSparseMemoryRequirementCount will be set to zero.
  xiiUInt32 uiSparseRequirementCount = 0U;
  vkLogicalDevice.getImageSparseMemoryRequirements(m_vkImage, &uiSparseRequirementCount, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  if (uiSparseRequirementCount != 1)
  {
    XII_REPORT_FAILURE("Sparse memory requirements for texture must be 1.");
  }

  // Texture with depth-stencil format may be implemented with two memory blocks per tile.
  vk::SparseImageMemoryRequirements vkSparseRequirements[2] = {};
  vkLogicalDevice.getImageSparseMemoryRequirements(m_vkImage, &uiSparseRequirementCount, vkSparseRequirements, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_SparseTextureProperties.m_uiMipTailOffset  = vkSparseRequirements[0].imageMipTailOffset;
  m_SparseTextureProperties.m_uiMipTailSize    = vkSparseRequirements[0].imageMipTailSize;
  m_SparseTextureProperties.m_uiMipTailStride  = vkSparseRequirements[0].imageMipTailStride;
  m_SparseTextureProperties.m_uiFirstMipInTail = vkSparseRequirements[0].imageMipTailFirstLod;
  m_SparseTextureProperties.m_vTileSize.x      = vkSparseRequirements[0].formatProperties.imageGranularity.width;
  m_SparseTextureProperties.m_vTileSize.y      = vkSparseRequirements[0].formatProperties.imageGranularity.height;
  m_SparseTextureProperties.m_vTileSize.z      = vkSparseRequirements[0].formatProperties.imageGranularity.depth;
  m_SparseTextureProperties.m_Flags            = xiiVulkanTypeConversions::GetSparseTextureFlags(vkSparseRequirements[0].formatProperties.flags);

  if (m_Description.m_uiArraySizeOrDepth == 1)
  {
    XII_ASSERT_DEV(m_SparseTextureProperties.m_uiMipTailOffset < vkMemoryRequirements.size || (m_SparseTextureProperties.m_uiMipTailOffset == vkMemoryRequirements.size && m_SparseTextureProperties.m_uiMipTailSize == 0), "");
    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailOffset + m_SparseTextureProperties.m_uiMipTailSize) <= vkMemoryRequirements.size, "");

    m_SparseTextureProperties.m_uiMipTailSize = 0U;
  }
  else
  {
    if (m_SparseTextureProperties.m_Flags.IsSet(xiiGALSparseTextureFlags::SingleMipTail))
    {
      m_SparseTextureProperties.m_uiMipTailSize = 0U;
    }
    else
    {
      XII_ASSERT_DEV(m_SparseTextureProperties.m_uiMipTailStride > 0, "");
    }

    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailStride * m_Description.m_uiArraySizeOrDepth) == vkMemoryRequirements.size, "");
    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailStride % vkMemoryRequirements.alignment) == 0, "");
    XII_ASSERT_DEV(m_SparseTextureProperties.m_uiMipTailOffset < m_SparseTextureProperties.m_uiMipTailStride, "");
    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailOffset + m_SparseTextureProperties.m_uiMipTailSize) <= m_SparseTextureProperties.m_uiMipTailStride, "");
  }

  m_SparseTextureProperties.m_uiAddressSpaceSize = vkMemoryRequirements.size;
  m_SparseTextureProperties.m_uiBlockSize        = static_cast<xiiUInt32>(vkMemoryRequirements.alignment);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    const xiiGALResourceFormatDescription& formatProperties    = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);
    const xiiUInt32                        uiByteCountPerBlock = formatProperties.GetElementSize();
    const xiiUInt32                        uiByteCountPerTile  = (m_SparseTextureProperties.m_vTileSize.x / formatProperties.m_uiBlockWidth) * (m_SparseTextureProperties.m_vTileSize.y / formatProperties.m_uiBlockHeight) * (m_SparseTextureProperties.m_vTileSize.z * m_Description.m_uiSampleCount * uiByteCountPerBlock);

    XII_ASSERT_DEBUG(uiByteCountPerTile == m_SparseTextureProperties.m_uiBlockSize, "Expected memory alignment equivalent to the block size.");
  }
#endif
}

xiiResult xiiGALTextureVulkan::InitializeImageExternalMemoryProperties(xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  if (!externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    return XII_SUCCESS;

  m_ExternalMemoryDescription.m_Type = xiiGALExternalMemoryKind::None;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Exportable))
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    xiiVulkanAllocationInfo allocationInfo = pDeviceVulkan->GetVulkanMemoryAllocator()->GetAllocationInfo(m_ImageMemoryAllocation);

    vk::MemoryGetWin32HandleInfoKHR vkGetMemoryHandleInfo = {};
    vkGetMemoryHandleInfo.memory                          = allocationInfo.m_vkDeviceMemory;
    vkGetMemoryHandleInfo.handleType                      = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;

    HANDLE hNativeHandle;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getMemoryWin32HandleKHR(&vkGetMemoryHandleInfo, &hNativeHandle, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ExternalMemoryDescription.m_Type              = xiiGALExternalMemoryKind::Exportable;
    m_ExternalMemoryDescription.m_Flags             = xiiGALExternalMemoryFlags::SharedAccess;
    m_ExternalMemoryDescription.m_uiNativeHandle    = reinterpret_cast<uintptr_t>(hNativeHandle);
    m_ExternalMemoryDescription.m_uiProcessId       = xiiProcess::GetCurrentProcessID();
    m_ExternalMemoryDescription.m_uiSize            = allocationInfo.m_uiSize;
    m_ExternalMemoryDescription.m_uiMemoryTypeIndex = allocationInfo.m_uiMemoryType;

    vk::ExportSemaphoreWin32HandleInfoKHR vkExportSemaphoreHandleInfo = {};
    vkExportSemaphoreHandleInfo.dwAccess                              = GENERIC_READ | GENERIC_WRITE;

    vk::ExportSemaphoreCreateInfo vkExportSemaphoreCreateInfo = {};
    vkExportSemaphoreCreateInfo.pNext                         = &vkExportSemaphoreHandleInfo;
    vkExportSemaphoreCreateInfo.handleTypes                   = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;

    vk::SemaphoreTypeCreateInfoKHR vkSemaphoreTypeCreateInfo = {};
    vkExportSemaphoreCreateInfo.pNext                        = &vkExportSemaphoreCreateInfo;
    vkSemaphoreTypeCreateInfo.semaphoreType                  = vk::SemaphoreType::eTimeline;
    vkSemaphoreTypeCreateInfo.initialValue                   = 0;

    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.pNext                   = &vkSemaphoreTypeCreateInfo;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_vkExternalMemorySemaphore, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    HANDLE                             hSemaphoreHandle;
    vk::SemaphoreGetWin32HandleInfoKHR vkSemaphoreGetHandleInfo = {};
    vkSemaphoreGetHandleInfo.semaphore                          = m_vkExternalMemorySemaphore;
    vkSemaphoreGetHandleInfo.handleType                         = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSemaphoreWin32HandleKHR(&vkSemaphoreGetHandleInfo, &hSemaphoreHandle, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = reinterpret_cast<uintptr_t>(hNativeHandle);

#elif XII_ENABLED(XII_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
    xiiVulkanAllocationInfo allocationInfo = pDeviceVulkan->GetVulkanMemoryAllocator()->GetAllocationInfo(m_ImageMemoryAllocation);

    vk::MemoryGetFdInfoKHR vkGetMemoryFdInfo = {};
    vkGetMemoryFdInfo.memory                 = allocationInfo.m_vkDeviceMemory;
    vkGetMemoryFdInfo.handleType             = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;

    xiiInt32 iFD;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getMemoryFdKHR(&vkGetMemoryFdInfo, &iFD, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ExternalMemoryDescription.m_Type              = xiiGALExternalMemoryKind::Exportable;
    m_ExternalMemoryDescription.m_Flags             = xiiGALExternalMemoryFlags::SharedAccess;
    m_ExternalMemoryDescription.m_uiNativeHandle    = static_cast<uintptr_t>(iFD);
    m_ExternalMemoryDescription.m_uiProcessId       = xiiProcess::GetCurrentProcessID();
    m_ExternalMemoryDescription.m_uiSize            = allocationInfo.m_uiSize;
    m_ExternalMemoryDescription.m_uiMemoryTypeIndex = allocationInfo.m_uiMemoryType;

    vk::ExportSemaphoreCreateInfo vkExportSemaphoreCreateInfo = {};
    vkExportSemaphoreCreateInfo.handleTypes                   = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;

    vk::SemaphoreTypeCreateInfoKHR vkSemaphoreTypeCreateInfo = {};
    vkSemaphoreTypeCreateInfo.semaphoreType                  = vk::SemaphoreType::eTimeline;
    vkSemaphoreTypeCreateInfo.initialValue                   = 0;
    vkSemaphoreTypeCreateInfo.pNext                          = &vkExportSemaphoreCreateInfo;

    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.pNext                   = &vkSemaphoreTypeCreateInfo;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_vkExternalMemorySemaphore, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    xiiInt32                  iSemaphoreFD;
    vk::SemaphoreGetFdInfoKHR vkSemaphoreGetFdInfo = {};
    vkSemaphoreGetFdInfo.semaphore                 = m_vkExternalMemorySemaphore;
    vkSemaphoreGetFdInfo.handleType                = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.getSemaphoreFdKHR(&vkSemaphoreGetFdInfo, &iSemaphoreFD, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = static_cast<uintptr_t>(iSemaphoreFD);

#else
    xiiLog::Error("Exporting Vulkan external memory is unsupported on this platform.");
    return XII_FAILURE;
#endif
  }
  else if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const bool bNeedsForeignFileDescriptorsImport = m_ExternalMemoryDescription.m_uiProcessId != xiiProcess::GetCurrentProcessID();
    if (bNeedsForeignFileDescriptorsImport)
    {
      HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, static_cast<DWORD>(m_ExternalMemoryDescription.m_uiProcessId));
      if (hProcess == nullptr)
      {
        xiiLog::Error("Failed to open process with ID {} to import external memory handle. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(GetLastError()));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0;
        m_ExternalMemoryDescription.m_uiNativeHandle          = 0;

        return XII_FAILURE;
      }

      HANDLE hDuplicatedHandleA;
      bool   bSuccess                              = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_ExternalMemoryDescription.m_uiNativeHandle), GetCurrentProcess(), &hDuplicatedHandleA, 0, FALSE, DUPLICATE_SAME_ACCESS);
      m_ExternalMemoryDescription.m_uiNativeHandle = reinterpret_cast<uintptr_t>(hDuplicatedHandleA);
      if (!bSuccess)
      {
        xiiLog::Error("Failed to duplicate external memory handle from process with ID {}. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(GetLastError()));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0;
        m_ExternalMemoryDescription.m_uiNativeHandle          = 0;

        CloseHandle(hProcess);
        return XII_FAILURE;
      }

      HANDLE hDuplicatedHandleB;
      bSuccess                                              = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle), GetCurrentProcess(), &hDuplicatedHandleB, 0, FALSE, DUPLICATE_SAME_ACCESS);
      m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = reinterpret_cast<uintptr_t>(hDuplicatedHandleB);
      if (!bSuccess)
      {
        xiiLog::Error("Failed to duplicate external semaphore handle from process with ID {}. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(GetLastError()));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0;

        CloseHandle(hProcess);
        return XII_FAILURE;
      }
    }

    // Import semaphore.
    vk::SemaphoreTypeCreateInfoKHR vkSemaphoreTypeCreateInfo = {};
    vkSemaphoreTypeCreateInfo.semaphoreType                  = vk::SemaphoreType::eTimeline;
    vkSemaphoreTypeCreateInfo.initialValue                   = 0;

    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.pNext                   = &vkSemaphoreTypeCreateInfo;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_vkExternalMemorySemaphore, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    vk::ImportSemaphoreWin32HandleInfoKHR vkImportSemaphoreHandleInfo = {};
    vkImportSemaphoreHandleInfo.semaphore                             = m_vkExternalMemorySemaphore;
    vkImportSemaphoreHandleInfo.handleType                            = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
    vkImportSemaphoreHandleInfo.handle                                = reinterpret_cast<HANDLE>(m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle);
    vkImportSemaphoreHandleInfo.flags                                 = {};

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.importSemaphoreWin32HandleKHR(&vkImportSemaphoreHandleInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    // Image is already created, so import memory.
    vk::ImageMemoryRequirementsInfo2 vkImageMemoryRequirementsInfo = {};
    vkImageMemoryRequirementsInfo.image                            = m_vkImage;

    vk::MemoryRequirements2 vkMemoryRequirements2 = {};
    vkLogicalDevice.getImageMemoryRequirements2(&vkImageMemoryRequirementsInfo, &vkMemoryRequirements2, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    XII_ASSERT_DEBUG(m_ExternalMemoryDescription.m_uiSize >= vkMemoryRequirements2.memoryRequirements.size, "Imported memory size is smaller than required.");

    vk::ImportMemoryWin32HandleInfoKHR vkImportMemoryHandleInfo = {};
    vkImportMemoryHandleInfo.handle                             = reinterpret_cast<HANDLE>(m_ExternalMemoryDescription.m_uiNativeHandle);
    vkImportMemoryHandleInfo.handleType                         = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
    vkImportMemoryHandleInfo.pNext                              = nullptr;

    vk::MemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.pNext                  = &vkImportMemoryHandleInfo;
    vkMemoryAllocateInfo.allocationSize         = vkMemoryRequirements2.memoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex        = m_ExternalMemoryDescription.m_uiMemoryTypeIndex;

    vk::DeviceMemory vkDeviceMemory;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.allocateMemory(&vkMemoryAllocateInfo, nullptr, &vkDeviceMemory, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ImageMemoryAllocation                      = {};
    m_ImageMemoryAllocationInfo.m_vkDeviceMemory = vkDeviceMemory;
    m_ImageMemoryAllocationInfo.m_uiOffset       = 0U;
    m_ImageMemoryAllocationInfo.m_uiSize         = vkMemoryRequirements2.memoryRequirements.size;
    m_ImageMemoryAllocationInfo.m_uiMemoryType   = vkMemoryAllocateInfo.memoryTypeIndex;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.bindImageMemory(m_vkImage, vkDeviceMemory, 0, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

#elif XII_ENABLED(XII_PLATFORM_LINUX)
    const bool bNeedsForeignFileDescriptorsImport = m_ExternalMemoryDescription.m_uiProcessId != xiiProcess::GetCurrentProcessID();
    if (bNeedsForeignFileDescriptorsImport)
    {
      // On Linux, file descriptors are shared between processes, so no duplication is necessary. However, we still need to check if the process that created the external memory is still alive to avoid importing stale file descriptors.
      xiiInt32 iProcessFD = syscall(SYS_pidfd_open, static_cast<pid_t>(m_ExternalMemoryDescription.m_uiProcessId), 0);
      if (iProcessFD == -1)
      {
        xiiLog::Error("Failed to open (SYS_pidfd_open) process with ID {} to import external memory handle. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(errno));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0U;
        m_ExternalMemoryDescription.m_uiNativeHandle          = 0U;
        return XII_FAILURE;
      }
      XII_SCOPE_EXIT(close(iProcessFD));

      m_ExternalMemoryDescription.m_uiNativeHandle = syscall(SYS_pidfd_getfd, iProcessFD, static_cast<xiiInt32>(m_ExternalMemoryDescription.m_uiNativeHandle), 0);
      if (m_ExternalMemoryDescription.m_uiNativeHandle == static_cast<uintptr_t>(-1))
      {
        xiiLog::Error("Failed to duplicate external memory file descriptor (SYS_pidfd_getfd) from process with ID {}. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(errno));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0U;
        m_ExternalMemoryDescription.m_uiNativeHandle          = 0U;
        return XII_FAILURE;
      }

      m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = syscall(SYS_pidfd_getfd, iProcessFD, static_cast<xiiInt32>(m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle), 0);
      if (m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle == static_cast<uintptr_t>(-1))
      {
        xiiLog::Error("Failed to duplicate external semaphore file descriptor (SYS_pidfd_getfd) from process with ID {}. Error code: {}", m_ExternalMemoryDescription.m_uiProcessId, xiiArgErrorCode(errno));

        m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0U;
        return XII_FAILURE;
      }
    }

    // Import semaphore.
    vk::SemaphoreTypeCreateInfoKHR vkSemaphoreTypeCreateInfo = {};
    vkSemaphoreTypeCreateInfo.semaphoreType                  = vk::SemaphoreType::eTimeline;
    vkSemaphoreTypeCreateInfo.initialValue                   = 0;

    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.pNext                   = &vkSemaphoreTypeCreateInfo;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_vkExternalMemorySemaphore, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    vk::ImportSemaphoreFdInfoKHR vkImportSemaphoreFdInfo = {};
    vkImportSemaphoreFdInfo.semaphore                    = m_vkExternalMemorySemaphore;
    vkImportSemaphoreFdInfo.handleType                   = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
    vkImportSemaphoreFdInfo.fd                           = static_cast<xiiInt32>(m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle);
    vkImportSemaphoreFdInfo.flags                        = {};

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.importSemaphoreFdKHR(&vkImportSemaphoreFdInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    // Note: Importing a semaphore payload from a file descriptor does not transfer ownership of the file descriptor to the Vulkan implementation, so the application is still responsible for closing the file descriptor when it is no longer needed.
    m_ExternalMemoryDescription.m_uiNativeSemaphoreHandle = 0U;

    // Image is already created, so import memory.
    vk::ImageMemoryRequirementsInfo2 vkImageMemoryRequirementsInfo = {};
    vkImageMemoryRequirementsInfo.image                            = m_vkImage;

    vk::MemoryRequirements2 vkMemoryRequirements2 = {};
    vkLogicalDevice.getImageMemoryRequirements2(&vkImageMemoryRequirementsInfo, &vkMemoryRequirements2, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    XII_ASSERT_DEBUG(m_ExternalMemoryDescription.m_uiSize >= vkMemoryRequirements2.memoryRequirements.size, "Imported memory size is smaller than required.");

    vk::ImportMemoryFdInfoKHR vkImportMemoryFdInfo = {};
    vkImportMemoryFdInfo.fd                        = static_cast<xiiInt32>(m_ExternalMemoryDescription.m_uiNativeHandle);
    vkImportMemoryFdInfo.handleType                = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    vkImportMemoryFdInfo.pNext                     = nullptr;

    vk::MemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.pNext                  = &vkImportMemoryFdInfo;
    vkMemoryAllocateInfo.allocationSize         = vkMemoryRequirements2.memoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex        = m_ExternalMemoryDescription.m_uiMemoryTypeIndex;

    vk::DeviceMemory vkDeviceMemory;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.allocateMemory(&vkMemoryAllocateInfo, nullptr, &vkDeviceMemory, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_ImageMemoryAllocation                      = {};
    m_ImageMemoryAllocationInfo.m_vkDeviceMemory = vkDeviceMemory;
    m_ImageMemoryAllocationInfo.m_uiOffset       = 0U;
    m_ImageMemoryAllocationInfo.m_uiSize         = vkMemoryRequirements2.memoryRequirements.size;
    m_ImageMemoryAllocationInfo.m_uiMemoryType   = vkMemoryAllocateInfo.memoryTypeIndex;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.bindImageMemory(m_vkImage, vkDeviceMemory, 0, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    // Note: Importing memory from a file descriptor does not transfer ownership of the file descriptor to the Vulkan implementation, so the application is still responsible for closing the file descriptor when it is no longer needed.
    m_ExternalMemoryDescription.m_uiNativeHandle = 0U;

#else
    xiiLog::Error("Importing Vulkan external memory is unsupported on this platform.");
    return XII_FAILURE;
#endif
  }

  return XII_SUCCESS;
}

void xiiGALTextureVulkan::ComputeVkImageCreateInfo(const xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo)
{
  const bool                             bIsMemoryLess         = creationDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless);
  const xiiGALResourceFormatDescription& formatProperties      = xiiGALTextureUtilities::GetResourceFormatProperties(creationDescription.m_Format);
  const bool                             bImageView2DSupported = !creationDescription.Is3D() || pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_bTextureView2DOn3DSupported;
  const auto&                            vkExtensionFeatures   = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

  ref_vkImageCreateInfo       = vk::ImageCreateInfo();
  ref_vkImageCreateInfo.pNext = nullptr;
  ref_vkImageCreateInfo.flags = {};

  if (creationDescription.m_Type == xiiGALResourceDimension::TextureCube || creationDescription.m_Type == xiiGALResourceDimension::TextureCubeArray)
  {
    ref_vkImageCreateInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
  }
  if (formatProperties.m_bIsTypeless)
  {
    ref_vkImageCreateInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat; // Specifies that the image can be used to create a vk::ImageView with a different format from the image.
  }

  if (creationDescription.Is1D())
  {
    ref_vkImageCreateInfo.imageType = vk::ImageType::e1D;
  }
  else if (creationDescription.Is2D())
  {
    ref_vkImageCreateInfo.imageType = vk::ImageType::e2D;
  }
  else if (creationDescription.Is3D())
  {
    ref_vkImageCreateInfo.imageType = vk::ImageType::e3D;

    if (bImageView2DSupported)
    {
      ref_vkImageCreateInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
    }
  }
  else
  {
    XII_REPORT_FAILURE("Unknown texture type.");
  }

  xiiEnum<xiiGALResourceFormat> internalTextureFormat = creationDescription.m_Format;
  if (formatProperties.m_bIsTypeless)
  {
    xiiGALTextureViewType::Enum primaryViewType;
    if (creationDescription.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
    {
      primaryViewType = xiiGALTextureViewType::DepthStencil;
    }
    else if (creationDescription.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
    {
      primaryViewType = xiiGALTextureViewType::UnorderedAccess;
    }
    else if (creationDescription.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
    {
      primaryViewType = xiiGALTextureViewType::RenderTarget;
    }
    else
    {
      primaryViewType = xiiGALTextureViewType::ShaderResource;
    }

    internalTextureFormat = xiiGALTextureUtilities::GetDefaultTextureViewFormat(creationDescription.m_Format, primaryViewType, creationDescription.m_BindFlags);
  }

  ref_vkImageCreateInfo.format        = xiiVulkanTypeConversions::GetFormat(internalTextureFormat);
  ref_vkImageCreateInfo.extent.width  = creationDescription.m_Size.width;
  ref_vkImageCreateInfo.extent.height = creationDescription.m_Size.height;
  ref_vkImageCreateInfo.extent.depth  = creationDescription.Is3D() ? creationDescription.m_uiArraySizeOrDepth : 1U;
  ref_vkImageCreateInfo.mipLevels     = creationDescription.m_uiMipLevels;
  ref_vkImageCreateInfo.arrayLayers   = creationDescription.Is3D() ? 1U : creationDescription.m_uiArraySizeOrDepth;
  ref_vkImageCreateInfo.samples       = static_cast<vk::SampleCountFlagBits>(creationDescription.m_uiSampleCount);
  ref_vkImageCreateInfo.tiling        = vk::ImageTiling::eOptimal;
  ref_vkImageCreateInfo.usage         = xiiVulkanTypeConversions::GetImageUsageFlags(creationDescription.m_BindFlags, bIsMemoryLess, vkExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False);
  ref_vkImageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst; // These flags are required by CopyTexture commands.

  if (creationDescription.m_BindFlags.IsAnySet(xiiGALBindFlags::DepthStencil | xiiGALBindFlags::RenderTarget))
  {
    XII_ASSERT_ALWAYS(bImageView2DSupported, "imageView2DOn3DImage in VkPhysicalDevicePortabilitySubsetFeaturesKHR is not enabled, can not create depth-stencil target with 2D image view.");
  }

  if (creationDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips))
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    XII_ASSERT_DEV(!bIsMemoryLess, "Expected memory-less image.");

    {
      vk::PhysicalDevice vkPhysicalDevice = pDeviceVulkan->GetVulkanPhysicalDevice();

      vk::FormatProperties vkFormatProperties;
      vkPhysicalDevice.getFormatProperties(ref_vkImageCreateInfo.format, &vkFormatProperties, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

      XII_ASSERT_DEV((vkFormatProperties.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst)) == (vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst), "Automatic mipmap generation is not supported for {} as the format does not support blitting.", internalTextureFormat);
      XII_ASSERT_DEV((vkFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear), "Automatic mipmap generation is not supported for {} as the format does not support linear filtering.", internalTextureFormat);
    }
#endif
  }

  if (creationDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Subsampled))
  {
    ref_vkImageCreateInfo.usage &= ~(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage);
    ref_vkImageCreateInfo.flags |= vk::ImageCreateFlagBits::eSubsampledEXT;
  }

  const xiiArrayPtr<const xiiUInt32> activeQueueFamilies = pDeviceVulkan->GetActiveQueueFamilyIndices();
  ref_vkImageCreateInfo.sharingMode                      = activeQueueFamilies.GetCount() > 1U ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
  ref_vkImageCreateInfo.queueFamilyIndexCount            = activeQueueFamilies.GetCount() > 1U ? activeQueueFamilies.GetCount() : 0U;
  ref_vkImageCreateInfo.pQueueFamilyIndices              = activeQueueFamilies.GetCount() > 1U ? activeQueueFamilies.GetPtr() : nullptr;

  if (creationDescription.m_Usage == xiiGALResourceUsage::Sparse)
  {
    ref_vkImageCreateInfo.flags &= ~vk::ImageCreateFlagBits::e2DArrayCompatible; // Not compatible.
    ref_vkImageCreateInfo.flags |= vk::ImageCreateFlagBits::eSparseBinding;

    if (creationDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::SparseAlias))
    {
      ref_vkImageCreateInfo.flags = vk::ImageCreateFlagBits::eSparseAliased;
    }
  }
}

void xiiGALTextureVulkan::InitializeImageContent(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiGALResourceFormatDescription& formatProperties, const xiiGALTextureData* pInitialData)
{
  // Vulkan validation layers do not like uninitialized memory, so if no initial data is provided, we will clear the memory.

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();

  auto UploadStagingData = [&](xiiGALCommandListVulkan* pCommandListVulkan) -> void {
    vk::ImageAspectFlags imageAspectFlags = {};
    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
    {
      imageAspectFlags = vk::ImageAspectFlagBits::eDepth;
    }
    else if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      // Only single aspect bit must be specified when copying texture data.
      xiiLog::Error("Initializing Vulkan depth-stencil texture is not currently supported.");

      imageAspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }
    else
    {
      imageAspectFlags = vk::ImageAspectFlagBits::eColor;
    }

    // For either clear or copy command, dst layout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
    vk::ImageSubresourceRange vkSubresourceRange = {};
    vkSubresourceRange.aspectMask                = imageAspectFlags;
    vkSubresourceRange.baseArrayLayer            = 0U;
    vkSubresourceRange.layerCount                = vk::RemainingArrayLayers;
    vkSubresourceRange.baseMipLevel              = 0U;
    vkSubresourceRange.levelCount                = vk::RemainingMipLevels;

    pCommandListVulkan->TransitionImageLayout(m_vkImage, vkImageCreateInfo.initialLayout, vk::ImageLayout::eTransferDstOptimal, vkSubresourceRange, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer);

    SetResourceState(xiiGALResourceStateFlags::CopyDestination);

    vk::ImageLayout vkCurrentImageLayout = GetVulkanImageLayout();
    XII_ASSERT_DEV(vkCurrentImageLayout == vk::ImageLayout::eTransferDstOptimal, "");

    xiiUInt32 uiExpectedSubresourceCount = vkImageCreateInfo.mipLevels * vkImageCreateInfo.arrayLayers;
    if (pInitialData->m_pSubResources.GetCount() != uiExpectedSubresourceCount)
    {
      XII_REPORT_FAILURE("Incorrect number of subresources in Vulkan image initialization data. {} expected, while {} provided.", uiExpectedSubresourceCount, pInitialData->m_pSubResources.GetCount());
    }

    xiiUInt32 uiSubResourceIndex = 0;

    for (xiiUInt32 uiLayer = 0; uiLayer < vkImageCreateInfo.arrayLayers; ++uiLayer)
    {
      for (xiiUInt32 uiMip = 0; uiMip < vkImageCreateInfo.mipLevels; ++uiMip)
      {
        const xiiGALTextureSubResourceData& subResourceData  = pInitialData->m_pSubResources[uiSubResourceIndex];
        vk::BufferImageCopy                 vkCopyRegion     = {};
        xiiGALMipLevelProperties            mipLevelProperty = xiiGALTextureUtilities::GetMipLevelProperties(m_Description, uiMip);

        // The allocation will stay in the upload heap until the command list is reset, at which point all upload pages will be discarded.
        xiiGALStagingBufferAllocationVulkan stagingBufferAllocation = pCommandListVulkan->GetVulkanUploadStagingBufferPool()->Allocate(mipLevelProperty.m_uiMipSize);
        void*                               pMappedMemory           = nullptr;

        VK_SUCCEED_OR_RETURN(pVulkanMemoryAllocator->MapMemory(stagingBufferAllocation.m_VulkanAllocation, &pMappedMemory));
        VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, mipLevelProperty.m_uiMipSize));

        pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);

        // bufferOffset must be a multiple of 4 (18.4)
        vkCopyRegion.bufferOffset = stagingBufferAllocation.m_uiOffset; // Offset in bytes from the start of the buffer object.

        // bufferRowLength and bufferImageHeight specify the data in buffer memory as a subregion of a larger two- or three-dimensional image, and control the addressing calculations of
        // data in buffer memory. If either of these values is zero, that aspect of the buffer memory is considered to be tightly packed according to the imageExtent. (18.4)
        vkCopyRegion.bufferRowLength   = 0;
        vkCopyRegion.bufferImageHeight = 0;

        // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1)
        vkCopyRegion.imageOffset = vk::Offset3D{0, 0, 0};
        vkCopyRegion.imageExtent = vk::Extent3D{mipLevelProperty.m_LogicalSize.width, mipLevelProperty.m_LogicalSize.height, mipLevelProperty.m_uiDepth};

        vkCopyRegion.imageSubresource.aspectMask     = imageAspectFlags;
        vkCopyRegion.imageSubresource.mipLevel       = uiMip;
        vkCopyRegion.imageSubresource.baseArrayLayer = uiLayer;
        vkCopyRegion.imageSubresource.layerCount     = 1;

        XII_ASSERT_DEV(subResourceData.m_uiStride == 0 || subResourceData.m_uiStride >= mipLevelProperty.m_uiRowSize, "Stride is too small.");
        // For compressed-block formats, mipLevelProperty.m_uiRowSize is the size of one row of blocks
        XII_ASSERT_DEV(subResourceData.m_uiDepthStride == 0 || subResourceData.m_uiDepthStride >= (mipLevelProperty.m_StorageSize.height / formatProperties.m_uiBlockHeight) * mipLevelProperty.m_uiRowSize, "Depth stride is too small");

        for (xiiUInt32 uiZ = 0; uiZ < mipLevelProperty.m_uiDepth; ++uiZ)
        {
          for (xiiUInt32 uiY = 0; uiY < mipLevelProperty.m_StorageSize.height; uiY += formatProperties.m_uiBlockHeight)
          {
            // The subResourceData.m_uiStride must be the stride of one row of compressed blocks.
            memcpy(xiiMemoryUtils::AddByteOffset(pMappedMemory, ((uiY + uiZ * mipLevelProperty.m_StorageSize.height) / xiiUInt32{formatProperties.m_uiBlockHeight}) * mipLevelProperty.m_uiRowSize),
                   xiiMemoryUtils::AddByteOffset(subResourceData.m_pData.GetPtr(), (uiY / xiiUInt32{formatProperties.m_uiBlockHeight}) * subResourceData.m_uiStride + uiZ * subResourceData.m_uiDepthStride),
                   mipLevelProperty.m_uiRowSize);
          }
        }

        VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, mipLevelProperty.m_uiMipSize));
        pVulkanMemoryAllocator->UnmapMemory(stagingBufferAllocation.m_VulkanAllocation);

        pCommandListVulkan->MemoryBarrier(vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer);

        // Copy commands MUST be recorded outside of a render pass instance. This is OK here as copy will be the only command in the command buffer.
        // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
        pCommandListVulkan->CopyBufferToImage(stagingBufferAllocation.m_vkBuffer, m_vkImage, vkCurrentImageLayout, xiiMakeArrayPtr(&vkCopyRegion, 1U));

        ++uiSubResourceIndex;
      }
    }

    XII_ASSERT_DEV(uiSubResourceIndex == pInitialData->m_pSubResources.GetCount(), "");
  };

  if (auto pCommandListVulkan = xiiDynamicCast<xiiGALCommandListVulkan*>(pInitialData->m_pCommandList))
  {
    UploadStagingData(pCommandListVulkan);
  }
  else if (auto pCommandQueue = pDeviceVulkan->GetCommandQueue(xiiGALCommandQueueFlags::Graphics))
  {
    if (auto pImmediateCommandListVulkan = pDeviceVulkan->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListVulkan>())
    {
      pImmediateCommandListVulkan->Begin();
      {
        UploadStagingData(pImmediateCommandListVulkan);
      }
      pImmediateCommandListVulkan->End();

      pCommandQueue->Submit(pImmediateCommandListVulkan);
    }
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
