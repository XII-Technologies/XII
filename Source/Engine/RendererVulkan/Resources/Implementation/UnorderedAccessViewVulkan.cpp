#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

const vk::DescriptorBufferInfo& xiiGALUnorderedAccessViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const xiiGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

xiiGALUnorderedAccessViewVulkan::xiiGALUnorderedAccessViewVulkan(
  xiiGALResourceBase*                                 pResource,
  const xiiGALUnorderedAccessViewCreationDescription& Description) :
  xiiGALUnorderedAccessView(pResource, Description)
{
}

xiiGALUnorderedAccessViewVulkan::~xiiGALUnorderedAccessViewVulkan() {}

xiiResult xiiGALUnorderedAccessViewVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);

  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const xiiGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return XII_FAILURE;
  }

  if (pTexture)
  {
    auto                                    pParentTexture = static_cast<const xiiGALTextureVulkan*>(pTexture->GetParentResource());
    auto                                    image          = pParentTexture->GetImage();
    const xiiGALTextureCreationDescription& texDesc        = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo    viewCreateInfo;
    viewCreateInfo.format           = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image            = image;
    viewCreateInfo.subresourceRange = xiiConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
    viewCreateInfo.viewType         = xiiConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, bIsArrayView);
    if (texDesc.m_Type == xiiGALTextureType::TextureCube)
      viewCreateInfo.viewType = vk::ImageViewType::e2DArray; // There is no RWTextureCube / RWTextureCubeArray in HLSL

    m_resourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

    m_range = viewCreateInfo.subresourceRange;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    pVulkanDevice->SetDebugName("UAV-Texture", m_resourceImageInfo.imageView);
  }
  else if (pBuffer)
  {
    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      xiiLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return XII_FAILURE;
    }

    auto pParentBuffer = static_cast<const xiiGALBufferVulkan*>(pBuffer);
    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
    {
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range  = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else if (m_Description.m_bRawView)
    {
      m_resourceBufferInfo.offset = sizeof(xiiUInt32) * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range  = sizeof(xiiUInt32) * m_Description.m_uiNumElements;
    }
    else
    {
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range  = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALUnorderedAccessViewVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  m_resourceImageInfo  = vk::DescriptorImageInfo();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_UnorderedAccessViewVulkan);
