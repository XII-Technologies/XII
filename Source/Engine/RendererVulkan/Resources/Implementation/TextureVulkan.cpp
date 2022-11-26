#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

vk::Extent3D xiiGALTextureVulkan::GetMipLevelSize(xiiUInt32 uiMipLevel) const
{
  vk::Extent3D size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.width        = xiiMath::Max(1u, size.width >> uiMipLevel);
  size.height       = xiiMath::Max(1u, size.height >> uiMipLevel);
  size.depth        = xiiMath::Max(1u, size.depth >> uiMipLevel);
  return size;
}

vk::ImageSubresourceRange xiiGALTextureVulkan::GetFullRange() const
{
  vk::ImageSubresourceRange range;
  range.aspectMask     = GetAspectMask();
  range.baseArrayLayer = 0;
  range.baseMipLevel   = 0;
  range.layerCount     = m_Description.m_Type == xiiGALTextureType::TextureCube ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
  range.levelCount     = m_Description.m_uiMipLevelCount;
  return range;
}

vk::ImageAspectFlags xiiGALTextureVulkan::GetAspectMask() const
{
  vk::ImageAspectFlags mask = xiiConversionUtilsVulkan::IsDepthFormat(m_imageFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (xiiConversionUtilsVulkan::IsStencilFormat(m_imageFormat))
    mask |= vk::ImageAspectFlagBits::eStencil;
  return mask;
}

xiiGALTextureVulkan::xiiGALTextureVulkan(const xiiGALTextureCreationDescription& Description) :
  xiiGALTexture(Description), m_image(nullptr), m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
}

xiiGALTextureVulkan::xiiGALTextureVulkan(const xiiGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU) :
  xiiGALTexture(Description), m_image(nullptr), m_pExisitingNativeObject(Description.m_pExisitingNativeObject), m_imageFormat(OverrideFormat), m_formatOverride(true), m_bLinearCPU(bLinearCPU)
{
}

xiiGALTextureVulkan::~xiiGALTextureVulkan() {}

xiiResult xiiGALTextureVulkan::InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);

  vk::ImageCreateInfo createInfo = {};
  //#TODO_VULKAN VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT / VkImageFormatListCreateInfoKHR to allow changing the format in a view is slow.
  createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  if (m_imageFormat == vk::Format::eUndefined)
  {
    m_imageFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eResourceViewType;
  }

  if ((m_imageFormat == vk::Format::eR8G8B8A8Srgb || m_imageFormat == vk::Format::eB8G8R8A8Unorm) && m_Description.m_bCreateRenderTarget)
  {
    // printf("");
  }
  createInfo.format = m_imageFormat;
  if (createInfo.format == vk::Format::eUndefined)
  {
    xiiLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
    return XII_FAILURE;
  }
  const bool bIsDepth = xiiConversionUtilsVulkan::IsDepthFormat(m_imageFormat);

  m_stages          = vk::PipelineStageFlagBits::eTransfer;
  m_access          = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  m_preferredLayout = vk::ImageLayout::eGeneral;

  createInfo.initialLayout         = vk::ImageLayout::eUndefined;
  createInfo.sharingMode           = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices   = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.tiling                = vk::ImageTiling::eOptimal;
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_ResourceAccess.m_bReadBack)
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  createInfo.extent.width  = m_Description.m_uiWidth;
  createInfo.extent.height = m_Description.m_uiHeight;
  createInfo.extent.depth  = m_Description.m_uiDepth;
  createInfo.mipLevels     = m_Description.m_uiMipLevelCount;

  createInfo.samples = static_cast<vk::SampleCountFlagBits>(m_Description.m_SampleCount.GetValue());

  // m_bAllowDynamicMipGeneration has to be emulated via a shader so we need to enable shader resource view and render target support.
  if (m_Description.m_bAllowShaderResourceView || m_Description.m_bAllowDynamicMipGeneration)
  {
    // Needed for blit-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
    // Needed for shader-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
    m_preferredLayout = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
  }
  //VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT
  if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowDynamicMipGeneration)
  {
    if (bIsDepth)
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
      m_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
      m_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
      m_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
      m_access |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  if (m_Description.m_bAllowUAV)
  {
    createInfo.usage |= vk::ImageUsageFlagBits::eStorage;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    m_preferredLayout = vk::ImageLayout::eGeneral;
  }

  switch (m_Description.m_Type)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::TextureCube:
    {
      createInfo.imageType   = vk::ImageType::e2D;
      createInfo.arrayLayers = (m_Description.m_Type == xiiGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));

      if (m_Description.m_Type == xiiGALTextureType::TextureCube)
        createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }
    break;

    case xiiGALTextureType::Texture3D:
    {
      createInfo.arrayLayers = 1;
      createInfo.imageType   = vk::ImageType::e3D;
      if (m_Description.m_bCreateRenderTarget)
      {
        createInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
      }
    }
    break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return XII_FAILURE;
  }

  if (m_pExisitingNativeObject == nullptr)
  {
    xiiVulkanAllocationCreateInfo allocInfo;
    allocInfo.m_usage = xiiVulkanMemoryUsage::Auto;
    if (m_bLinearCPU)
    {
      createInfo.tiling = vk::ImageTiling::eLinear;
      createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
      m_stages |= vk::PipelineStageFlagBits::eHost;
      m_access |= vk::AccessFlagBits::eHostRead;
      createInfo.flags = {}; // Clear all flags as we don't need them and they usually are not supported on NVidia in linear mode.

      allocInfo.m_flags = xiiVulkanAllocationCreateFlags::HostAccessRandom;
    }
    vk::ImageFormatProperties props2;
    VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));

    VK_SUCCEED_OR_RETURN_XII_FAILURE(xiiMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));
  }
  else
  {
    m_image = static_cast<VkImage>(m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    return CreateStagingBuffer(createInfo);
  }

  return XII_SUCCESS;
}

xiiGALTextureVulkan::StagingMode xiiGALTextureVulkan::ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const
{
  if (!m_Description.m_ResourceAccess.m_bReadBack)
    return StagingMode::None;

  // We want the staging texture to always have the intended format and not the override format given by the parent texture.
  vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

  XII_ASSERT_DEV(!xiiConversionUtilsVulkan::IsStencilFormat(m_imageFormat), "Stencil read-back not implemented.");
  XII_ASSERT_DEV(!xiiConversionUtilsVulkan::IsDepthFormat(stagingFormat), "Depth read-back should use a color format for CPU staging.");

  const vk::FormatProperties srcFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(m_imageFormat);
  const vk::FormatProperties dstFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(stagingFormat);

  const bool bFormatsEqual = m_imageFormat == stagingFormat && createInfo.samples == vk::SampleCountFlagBits::e1;
  const bool bSupportsCopy = bFormatsEqual && (srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc) && (dstFormatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);
  if (bFormatsEqual)
  {
    XII_ASSERT_DEV(srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc, "Source format can't be read, readback impossible.");
    return StagingMode::Buffer;
  }
  else
  {
    vk::ImageFormatProperties props;
    vk::Result                res = m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(stagingFormat, createInfo.imageType, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eColorAttachment, {}, &props);

    const bool bCanUseDirectTexture = (res == vk::Result::eSuccess) && createInfo.arrayLayers <= props.maxArrayLayers && createInfo.mipLevels <= props.maxMipLevels && createInfo.extent.depth <= props.maxExtent.depth && createInfo.extent.width <= props.maxExtent.width && createInfo.extent.height <= props.maxExtent.height && (createInfo.samples & props.sampleCounts);
    return bCanUseDirectTexture ? StagingMode::Texture : StagingMode::TextureAndBuffer;
  }
}

xiiUInt32 xiiGALTextureVulkan::ComputeSubResourceOffsets(xiiDynamicArray<SubResourceOffset>& subResourceSizes) const
{
  const xiiUInt32  alignment     = (xiiUInt32)xiiGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eTransferDst);
  const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;
  const xiiUInt8   uiBlockSize   = vk::blockSize(stagingFormat);
  const auto       blockExtent   = vk::blockExtent(stagingFormat);
  const xiiUInt32  arrayLayers   = (m_Description.m_Type == xiiGALTextureType::TextureCube ? (m_Description.m_uiArraySize * 6) : m_Description.m_uiArraySize);
  const xiiUInt32  mipLevels     = m_Description.m_uiMipLevelCount;

  subResourceSizes.Reserve(arrayLayers * mipLevels);
  xiiUInt32 uiOffset = 0;
  for (xiiUInt32 uiLayer = 0; uiLayer < arrayLayers; uiLayer++)
  {
    for (xiiUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const xiiUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      XII_ASSERT_DEBUG(subResourceSizes.GetCount() == uiSubresourceIndex, "");

      const vk::Extent3D imageExtent = GetMipLevelSize(uiMipLevel);
      const VkExtent3D   blockCount  = {
        (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
        (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
        (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

      const xiiUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
      subResourceSizes.PushBack({uiOffset, uiTotalSize, blockCount.width / blockExtent[0], blockCount.height / blockExtent[1]});
      uiOffset += xiiMemoryUtils::AlignSize(uiTotalSize, alignment);
    }
  }
  return uiOffset;
}

xiiResult xiiGALTextureVulkan::CreateStagingBuffer(const vk::ImageCreateInfo& createInfo)
{
  m_stagingMode = xiiGALTextureVulkan::ComputeStagingMode(createInfo);
  if (m_stagingMode == StagingMode::Texture || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    xiiGALTextureCreationDescription stagingDesc = m_Description;
    stagingDesc.m_SampleCount                    = xiiGALMSAASampleCount::None;
    stagingDesc.m_bAllowShaderResourceView       = false;
    stagingDesc.m_bAllowUAV                      = false;
    stagingDesc.m_bCreateRenderTarget            = true;
    stagingDesc.m_bAllowDynamicMipGeneration     = false;
    stagingDesc.m_ResourceAccess.m_bReadBack     = false;
    stagingDesc.m_ResourceAccess.m_bImmutable    = false;
    stagingDesc.m_pExisitingNativeObject         = nullptr;

    const bool       bLinearCPU    = m_stagingMode == StagingMode::Texture;
    const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

    m_hStagingTexture = m_pDevice->CreateTextureInternal(stagingDesc, {}, stagingFormat, bLinearCPU);
    if (m_hStagingTexture.IsInvalidated())
    {
      xiiLog::Error("Failed to create staging texture for read-back");
      return XII_FAILURE;
    }
  }
  if (m_stagingMode == StagingMode::Buffer || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    xiiGALBufferCreationDescription stagingBuffer;
    stagingBuffer.m_BufferType = xiiGALBufferType::Generic;

    xiiHybridArray<SubResourceOffset, 8> subResourceSizes;
    stagingBuffer.m_uiTotalSize                 = ComputeSubResourceOffsets(subResourceSizes);
    stagingBuffer.m_uiStructSize                = 1;
    stagingBuffer.m_bAllowRawViews              = true;
    stagingBuffer.m_ResourceAccess.m_bImmutable = false;

    m_hStagingBuffer = m_pDevice->CreateBufferInternal(stagingBuffer, {}, true);
    if (m_hStagingBuffer.IsInvalidated())
    {
      xiiLog::Error("Failed to create staging buffer for read-back");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}
xiiResult xiiGALTextureVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);
  if (m_image && !m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
  }
  m_image = nullptr;

  if (!m_hStagingTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hStagingTexture);
    m_hStagingTexture.Invalidate();
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(m_hStagingBuffer);
    m_hStagingBuffer.Invalidate();
  }
  return XII_SUCCESS;
}

void xiiGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_image, m_alloc);
  if (!m_hStagingTexture.IsInvalidated())
  {
    auto pStagingTexture = static_cast<const xiiGALTextureVulkan*>(m_pDevice->GetTexture(m_hStagingTexture));
    pStagingTexture->SetDebugName(szName);
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    auto pStagingBuffer = static_cast<const xiiGALBufferVulkan*>(m_pDevice->GetBuffer(m_hStagingBuffer));
    pStagingBuffer->SetDebugName(szName);
  }
}



XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_TextureVulkan);
