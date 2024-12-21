#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>

xiiGALTextureVulkan::xiiGALTextureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceVulkan, creationDescription)
{
}

xiiGALTextureVulkan::~xiiGALTextureVulkan() = default;

xiiResult xiiGALTextureVulkan::InitPlatform(const xiiGALTextureData* pInitialData)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_Description.m_Usage == xiiGALResourceUsage::Immutable && pInitialData == nullptr || pInitialData->m_SubResources.IsEmpty())
  {
    xiiLog::Error("Immutable textures must be initialized with data at creation time. The given subresources cannot be empty.");
    return XII_FAILURE;
  }

  const bool bIsMemoryLess = m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless);
  if (bIsMemoryLess && pInitialData != nullptr && !pInitialData->m_SubResources.IsEmpty())
  {
    xiiLog::Error("Memoryless textures cannot be initialized with initial data.");
    return XII_FAILURE;
  }

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse && m_Description.Is3D() && (m_Description.m_BindFlags.IsAnySet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil)))
  {
    xiiLog::Error("Sparse 3D texture with xiiGALBindFlags::RenderTarget or xiiGALBindFlags::DepthStencil is not supported in Vulkan.");
    return XII_FAILURE;
  }

  vk::Device  vkLogicalDevice          = pDeviceVulkan->GetVulkanLogicalDevice();
  const auto& resourceFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);

  if (m_Description.m_Usage == xiiGALResourceUsage::Immutable || m_Description.m_Usage == xiiGALResourceUsage::Default || m_Description.m_Usage == xiiGALResourceUsage::Dynamic || m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    vk::ImageCreateInfo vkImageCreateInfo = {};
    ComputeVkImageCreateInfo(pDeviceVulkan, m_Description, vkImageCreateInfo);

    /// \todo GraphicsVulkan: Selectively utilize vk::SharingMode::eConcurrent for multiple queue family's ownership of the vulkan image.

    // initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED (11.4).
    // If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host while using this layout, and the transition away from this layout will preserve that data.
    // If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined, and the transition away from this layout is not guaranteed to preserve that data.
    vkImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    {
      VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
      vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateImage(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&m_vkImage), &m_ImageMemoryAllocation, nullptr));

      SetResourceState(xiiGALResourceStateFlags::Undefined);

      InitializeSparseTextureProperties();
    }
    else
    {
      VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
      vmaAllocationCreateInfo.requiredFlags           = bIsMemoryLess ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vmaCreateImage(pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&m_vkImage), &m_ImageMemoryAllocation, nullptr));

      if (pInitialData != nullptr && !pInitialData->m_SubResources.IsEmpty())
      {
        InitializeImageContent(pDeviceVulkan, vkImageCreateInfo, m_vkImage, pInitialData);
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

xiiResult xiiGALTextureVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkStagingBuffer != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkStagingBuffer), m_StagingBufferMemoryAllocation);

    m_StagingBufferMemoryAllocation = {};
  }
  m_vkStagingBuffer = VK_NULL_HANDLE;

  // Prevent releasing the native object.
  if (m_vkImage != VK_NULL_HANDLE && m_Description.m_pExisitingNativeObject != nullptr)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImage), m_ImageMemoryAllocation);

    m_ImageMemoryAllocation = {};
  }
  m_vkImage = VK_NULL_HANDLE;

  return XII_SUCCESS;
}

void xiiGALTextureVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkImage, sName.GetData(tmp));
}

vk::Result xiiGALTextureVulkan::CreateVulkanStagingBuffer(const xiiGALTextureData* pInitialData, const xiiGALResourceFormatDescription& formatProperties)
{
  xiiGALDeviceVulkan* pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice    = pDeviceVulkan->GetVulkanLogicalDevice();
  const bool          bInitializeTexture = (pInitialData != nullptr && !pInitialData->m_SubResources.IsEmpty());

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

  VK_SUCCEED_OR_RETURN_LOG(vkLogicalDevice.createBuffer(&vkStagingBufferCreateInfo, nullptr, &m_vkStagingBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader()), "Failed to create Vulkan staging buffer.");

  vk::MemoryRequirements vkStagingBufferMemoryRequirements = vkLogicalDevice.getBufferMemoryRequirements(m_vkStagingBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  XII_ASSERT_DEV(xiiMath::IsPowerOf2(vkStagingBufferMemoryRequirements.alignment), "Alignment is not a power of 2!");

  /// \todo GraphicsVulkan: Allocate and bind memory for buffer.
}

void xiiGALTextureVulkan::InitializeSparseTextureProperties()
{
  XII_ASSERT_DEV(m_Description.m_Usage == xiiGALResourceUsage::Sparse, "");

  xiiGALDeviceVulkan*    pDeviceVulkan        = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
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

void xiiGALTextureVulkan::ComputeVkImageCreateInfo(const xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTextureCreationDescription& creationDescription, vk::ImageCreateInfo& ref_vkImageCreateInfo)
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
    XII_ASSERT_DEV(!bIsMemoryLess, "");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    {
      vk::PhysicalDevice   vkPhysicalDevice   = pDeviceVulkan->GetVulkanPhysicalDevice();
      vk::FormatProperties vkFormatProperties = vkPhysicalDevice.getFormatProperties(ref_vkImageCreateInfo.format);

      XII_ASSERT_DEV((vkFormatProperties.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst)) == (vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst), "Automatic mipmap generation is not supported for {} as the format does not support blitting.", internalTextureFormat);
      XII_ASSERT_DEV((vkFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear), "Automatic mipmap generation is not supported for {} as the format does not support linear filtering.", internalTextureFormat);
#endif
    }
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

void xiiGALTextureVulkan::InitializeImageContent(const xiiGALDeviceVulkan* pDeviceVulkan, const vk::ImageCreateInfo& vkImageCreateInfo, const vk::Image& vkImage, const xiiGALTextureData* pInitialData)
{
  vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  // Vulkan validation layers do not like uninitialized memory, so if no initial data is provided, we will clear the memory.

  if (auto pGraphicsQueue = pDeviceVulkan->GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics, false))
  {
    if (auto pCommandList = pGraphicsQueue->BeginCommandList())
    {

    }
    else
    {
      xiiLog::Error("Failed to retrieve a command list to initialize the Vulkan image content.");
    }
  }
  else
  {
    xiiLog::Error("Failed to retrieve Vulkan default graphics queue to initialize the Vulkan image content.");
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
