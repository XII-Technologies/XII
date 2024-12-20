#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

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
  }

  return XII_SUCCESS;
}

xiiResult xiiGALTextureVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  // Prevent releasing native objects.
  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImage));
  }
  return XII_SUCCESS;
}

void xiiGALTextureVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkImage, sName.GetData(tmp));
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

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
