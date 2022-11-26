#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiArraySize > 1;
}

const vk::DescriptorBufferInfo& xiiGALResourceViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const xiiGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

xiiGALResourceViewVulkan::xiiGALResourceViewVulkan(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) :
  xiiGALResourceView(pResource, Description)
{
}

xiiGALResourceViewVulkan::~xiiGALResourceViewVulkan() {}

xiiResult xiiGALResourceViewVulkan::InitPlatform(xiiGALDevice* pDevice)
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
    const bool bIsDepth     = xiiGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);

    xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo    viewCreateInfo;
    viewCreateInfo.format           = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image            = image;
    viewCreateInfo.subresourceRange = xiiConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
    viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;


    m_resourceImageInfo.imageLayout      = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
    m_resourceImageInfoArray.imageLayout = m_resourceImageInfo.imageLayout;

    if (pParentTexture->GetFormatOverrideEnabled())
    {
      XII_ASSERT_DEV(m_Description.m_OverrideViewFormat == xiiGALResourceFormat::Invalid, "Resource views on this texture can not use a override view format (not implemented)");
      viewCreateInfo.format = pParentTexture->GetImageFormat();
    }

    m_range = viewCreateInfo.subresourceRange;
    if (texDesc.m_Type == xiiGALTextureType::Texture3D) // no array support
    {
      viewCreateInfo.viewType = xiiConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    }
    else if (m_Description.m_uiArraySize == 1) // can be array or not
    {
      viewCreateInfo.viewType = xiiConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      viewCreateInfo.viewType = xiiConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
    else // Can only be array
    {
      viewCreateInfo.viewType = xiiConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
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
      xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == xiiGALResourceFormat::Invalid)
        viewFormat = xiiGALResourceFormat::RUInt;

      vk::BufferViewCreateInfo viewCreateInfo;
      viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
      viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
      viewCreateInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      viewCreateInfo.range  = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createBufferView(&viewCreateInfo, nullptr, &m_bufferView));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALResourceViewVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  pVulkanDevice->DeleteLater(m_resourceImageInfoArray.imageView);
  m_resourceImageInfo      = vk::DescriptorImageInfo();
  m_resourceImageInfoArray = vk::DescriptorImageInfo();
  m_resourceBufferInfo     = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_ResourceViewVulkan);
