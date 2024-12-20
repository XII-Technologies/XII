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
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

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

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
