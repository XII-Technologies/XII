#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

vk::ImageLayout xiiGALTextureVulkan::GetVulkanImageLayout() const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const auto&                      fragmentDensityMap = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures().m_FragmentDensityMap;
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

  // Prevent releasing the native object.
  if (m_vkImage != VK_NULL_HANDLE && m_Description.m_pExistingNativeObject == nullptr)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImage), std::move(m_ImageMemoryAllocation));
  }
}

xiiResult xiiGALTextureVulkan::InitPlatform(const xiiGALTextureData* pInitialData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

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

  const auto& resourceFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);

  if (m_Description.m_pExistingNativeObject != nullptr)
  {
    m_vkImage = static_cast<VkImage>(m_Description.m_pExistingNativeObject);

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    return XII_SUCCESS;
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Immutable || m_Description.m_Usage == xiiGALResourceUsage::Default || m_Description.m_Usage == xiiGALResourceUsage::Dynamic || m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    vk::ImageCreateInfo vkImageCreateInfo = {};
    ComputeVkImageCreateInfo(pDeviceVulkan, m_Description, vkImageCreateInfo);

    /// \todo GraphicsVulkan: Selectively utilize vk::SharingMode::eConcurrent for multiple queue family's ownership of the Vulkan image.

    // initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED (11.4).
    // If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host while using this layout, and the transition away from this layout will preserve that data.
    // If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined, and the transition away from this layout is not guaranteed to preserve that data.
    vkImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    {
      VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
      vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateImage(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&m_vkImage), &m_ImageMemoryAllocation, nullptr));

      SetResourceState(xiiGALResourceStateFlags::Undefined);

      InitializeSparseTextureProperties();
    }
    else
    {
      VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
      vmaAllocationCreateInfo.requiredFlags           = bIsMemoryLess ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateImage(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&m_vkImage), &m_ImageMemoryAllocation, nullptr));

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
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const bool                       bInitializeTexture = (pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty());

  vk::BufferCreateInfo vkStagingBufferCreateInfo = {};
  vkStagingBufferCreateInfo.pNext                = nullptr;
  vkStagingBufferCreateInfo.flags                = {};
  vkStagingBufferCreateInfo.size                 = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(m_Description, m_Description.GetArraySize(), 0, s_uiStagingBufferOffsetAlignment);

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

  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(vkMemoryPropertyFlags);
  vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
  vmaAllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo stagingBufferAllocationInfo;
  VK_SUCCEED_OR_RETURN_LOG((vk::Result)vmaCreateBuffer(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkStagingBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&m_vkStagingBuffer), &m_StagingBufferMemoryAllocation, &stagingBufferAllocationInfo));

  XII_ASSERT_DEV(stagingBufferAllocationInfo.pMappedData != nullptr, "");

  if (bInitializeTexture)
  {
    xiiUInt32 uiSubresourceIndex = 0;

    for (xiiUInt32 uiLayer = 0; uiLayer < m_Description.GetArraySize(); ++uiLayer)
    {
      for (xiiUInt32 uiMip = 0; uiMip < m_Description.m_uiMipLevels; ++uiMip)
      {
        const xiiGALTextureSubResourceData& subresourceData                = pInitialData->m_pSubResources[uiSubresourceIndex++];
        const xiiGALMipLevelProperties      mipLevelProperty               = xiiGALTextureUtilities::GetMipLevelProperties(m_Description, uiMip);
        const xiiUInt64                     uiDestinationSubresourceOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(m_Description, uiLayer, uiMip, s_uiStagingBufferOffsetAlignment);

        xiiGALTextureUtilities::CopyTextureSubresource(subresourceData, mipLevelProperty.m_StorageSize.height / formatProperties.m_uiBlockHeight, mipLevelProperty.m_uiDepth, mipLevelProperty.m_uiRowSize, xiiMemoryUtils::AddByteOffset(stagingBufferAllocationInfo.pMappedData, uiDestinationSubresourceOffset), mipLevelProperty.m_uiRowSize, mipLevelProperty.m_uiDepthSliceSize);
      }
    }
  }
  return vk::Result::eSuccess;
}

void xiiGALTextureVulkan::InitializeSparseTextureProperties()
{
  XII_ASSERT_DEV(m_Description.m_Usage == xiiGALResourceUsage::Sparse, "");

  xiiGALDeviceVulkan*    pDeviceVulkan        = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device             vkLogicalDevice      = pDeviceVulkan->GetVulkanLogicalDevice();
  vk::MemoryRequirements vkMemoryRequirements = vkLogicalDevice.getImageMemoryRequirements(m_vkImage, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

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

  if (m_Description.GetArraySize() == 1)
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

    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailStride * m_Description.GetArraySize()) == vkMemoryRequirements.size, "");
    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailStride % vkMemoryRequirements.alignment) == 0, "");
    XII_ASSERT_DEV(m_SparseTextureProperties.m_uiMipTailOffset < m_SparseTextureProperties.m_uiMipTailStride, "");
    XII_ASSERT_DEV((m_SparseTextureProperties.m_uiMipTailOffset + m_SparseTextureProperties.m_uiMipTailSize) <= m_SparseTextureProperties.m_uiMipTailStride, "");
  }

  m_SparseTextureProperties.m_uiAddressSpaceSize = vkMemoryRequirements.size;
  m_SparseTextureProperties.m_uiBlockSize        = static_cast<xiiUInt32>(vkMemoryRequirements.alignment);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    const auto&     formatProperties    = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);
    const xiiUInt32 uiByteCountPerBlock = formatProperties.GetElementSize();
    const xiiUInt32 uiByteCountPerTile  = (m_SparseTextureProperties.m_vTileSize.x / formatProperties.m_uiBlockWidth) * (m_SparseTextureProperties.m_vTileSize.y / formatProperties.m_uiBlockHeight) * (m_SparseTextureProperties.m_vTileSize.z * m_Description.m_uiSampleCount * uiByteCountPerBlock);

    XII_ASSERT_DEBUG(uiByteCountPerTile == m_SparseTextureProperties.m_uiBlockSize, "Expected memory alignment equivalent to the block size.");
  }
#endif
}

void xiiGALTextureVulkan::ComputeVkImageCreateInfo(const xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo)
{
  const bool  bIsMemoryLess         = creationDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless);
  const auto& formatProperties      = xiiGALTextureUtilities::GetResourceFormatProperties(creationDescription.m_Format);
  const bool  bImageView2DSupported = !creationDescription.Is3D() || pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_bTextureView2DOn3DSupported;
  const auto& vkExtensionFeatures   = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

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
  ref_vkImageCreateInfo.extent.height = creationDescription.Is1D() ? 1U : creationDescription.m_Size.height;
  ref_vkImageCreateInfo.extent.depth  = creationDescription.Is3D() ? creationDescription.m_uiArraySizeOrDepth : 1U;
  ref_vkImageCreateInfo.mipLevels     = creationDescription.m_uiMipLevels;
  ref_vkImageCreateInfo.arrayLayers   = creationDescription.GetArraySize();
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
    XII_ASSERT_DEV(!bIsMemoryLess, "");

    {
      vk::PhysicalDevice   vkPhysicalDevice   = pDeviceVulkan->GetVulkanPhysicalDevice();
      vk::FormatProperties vkFormatProperties = vkPhysicalDevice.getFormatProperties(ref_vkImageCreateInfo.format, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

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

  ref_vkImageCreateInfo.sharingMode           = vk::SharingMode::eExclusive;
  ref_vkImageCreateInfo.queueFamilyIndexCount = 0;
  ref_vkImageCreateInfo.pQueueFamilyIndices   = nullptr;

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

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

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

    xiiUInt32 uiSubresourceIndex = 0;

    for (xiiUInt32 uiLayer = 0; uiLayer < vkImageCreateInfo.arrayLayers; ++uiLayer)
    {
      for (xiiUInt32 uiMip = 0; uiMip < vkImageCreateInfo.mipLevels; ++uiMip)
      {
        const auto&              subresourceData  = pInitialData->m_pSubResources[uiSubresourceIndex];
        vk::BufferImageCopy      vkCopyRegion     = {};
        xiiGALMipLevelProperties mipLevelProperty = xiiGALTextureUtilities::GetMipLevelProperties(m_Description, uiMip);

        // The allocation will stay in the upload heap until the command list is reset, at which point all upload pages will be discarded.
        auto stagingBufferAllocation = pCommandListVulkan->GetVulkanUploadStagingBufferPool()->Allocate(mipLevelProperty.m_uiMipSize);

        void* pMappedMemory = nullptr;
        VK_SUCCEED_OR_RETURN(vmaMapMemory(pDeviceVulkan->GetVulkanMemoryAllocator(), stagingBufferAllocation.m_VmaAllocation, &pMappedMemory));
        VK_ASSERT_DEV(vmaInvalidateAllocation(pDeviceVulkan->GetVulkanMemoryAllocator(), stagingBufferAllocation.m_VmaAllocation, stagingBufferAllocation.m_uiOffset, mipLevelProperty.m_uiMipSize));

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

        XII_ASSERT_DEV(subresourceData.m_uiStride == 0 || subresourceData.m_uiStride >= mipLevelProperty.m_uiRowSize, "Stride is too small.");
        // For compressed-block formats, mipLevelProperty.m_uiRowSize is the size of one row of blocks
        XII_ASSERT_DEV(subresourceData.m_uiDepthStride == 0 || subresourceData.m_uiDepthStride >= (mipLevelProperty.m_StorageSize.height / formatProperties.m_uiBlockHeight) * mipLevelProperty.m_uiRowSize, "Depth stride is too small");

        for (xiiUInt32 uiZ = 0; uiZ < mipLevelProperty.m_uiDepth; ++uiZ)
        {
          for (xiiUInt32 uiY = 0; uiY < mipLevelProperty.m_StorageSize.height; uiY += formatProperties.m_uiBlockHeight)
          {
            // The subresourceData.m_uiStride must be the stride of one row of compressed blocks.
            memcpy(xiiMemoryUtils::AddByteOffset(pMappedMemory, ((uiY + uiZ * mipLevelProperty.m_StorageSize.height) / xiiUInt32{formatProperties.m_uiBlockHeight}) * mipLevelProperty.m_uiRowSize),
                   xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), (uiY / xiiUInt32{formatProperties.m_uiBlockHeight}) * subresourceData.m_uiStride + uiZ * subresourceData.m_uiDepthStride),
                   mipLevelProperty.m_uiRowSize);
          }
        }

        VK_ASSERT_DEV(vmaFlushAllocation(pDeviceVulkan->GetVulkanMemoryAllocator(), stagingBufferAllocation.m_VmaAllocation, stagingBufferAllocation.m_uiOffset, mipLevelProperty.m_uiMipSize));

        vmaUnmapMemory(pDeviceVulkan->GetVulkanMemoryAllocator(), stagingBufferAllocation.m_VmaAllocation);

        pCommandListVulkan->MemoryBarrier(vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer);

        // Copy commands MUST be recorded outside of a render pass instance. This is OK here as copy will be the only command in the command buffer.
        // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
        pCommandListVulkan->CopyBufferToImage(stagingBufferAllocation.m_vkBuffer, m_vkImage, vkCurrentImageLayout, xiiMakeArrayPtr(&vkCopyRegion, 1U));

        ++uiSubresourceIndex;
      }
    }

    XII_ASSERT_DEV(uiSubresourceIndex == pInitialData->m_pSubResources.GetCount(), "");
  };

	if (auto pCommandListVulkan = static_cast<xiiGALCommandListVulkan*>(pInitialData->m_pCommandList))
  {
    UploadStagingData(pCommandListVulkan);
  }
  else if (auto pCommandQueue = pDeviceVulkan->GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics))
  {
    if (auto pImmediateCommandListVulkan = pCommandQueue->BeginCommandList().Downcast<xiiGALCommandListVulkan>())
    {
      UploadStagingData(pImmediateCommandListVulkan);

      pImmediateCommandListVulkan->Submit();
    }
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
