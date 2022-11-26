#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

xiiGALRenderTargetViewVulkan::xiiGALRenderTargetViewVulkan(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) :
  xiiGALRenderTargetView(pTexture, Description)
{
}

xiiGALRenderTargetViewVulkan::~xiiGALRenderTargetViewVulkan() {}

xiiResult xiiGALRenderTargetViewVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    xiiLog::Error("No valid texture handle given for render target view creation!");
    return XII_FAILURE;
  }

  const xiiGALTextureCreationDescription& texDesc    = pTexture->GetDescription();
  xiiGALResourceFormat::Enum              viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != xiiGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  xiiGALDeviceVulkan* pVulkanDevice  = static_cast<xiiGALDeviceVulkan*>(pDevice);
  auto                pTextureVulkan = static_cast<const xiiGALTextureVulkan*>(pTexture->GetParentResource());
  vk::Format          vkViewFormat   = pTextureVulkan->GetImageFormat();

  const bool bIsDepthFormat = xiiConversionUtilsVulkan::IsDepthFormat(vkViewFormat);

  if (vkViewFormat == vk::Format::eUndefined)
  {
    xiiLog::Error("Couldn't get Vulkan format for view!");
    return XII_FAILURE;
  }


  vk::Image  vkImage      = pTextureVulkan->GetImage();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  if (pTextureVulkan->GetFormatOverrideEnabled())
  {
    vkViewFormat = pTextureVulkan->GetImageFormat();
  }

  vk::ImageViewCreateInfo imageViewCreationInfo;
  if (bIsDepthFormat)
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (texDesc.m_Format == xiiGALResourceFormat::D24S8)
    {
      imageViewCreationInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  }
  else
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  imageViewCreationInfo.image  = vkImage;
  imageViewCreationInfo.format = vkViewFormat;

  if (!bIsArrayView)
  {
    imageViewCreationInfo.viewType                      = vk::ImageViewType::e2D;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount   = 1;
    imageViewCreationInfo.subresourceRange.layerCount   = 1;
  }
  else
  {
    imageViewCreationInfo.viewType                        = vk::ImageViewType::e2DArray;
    imageViewCreationInfo.subresourceRange.baseMipLevel   = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount     = 1;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.layerCount     = m_Description.m_uiSliceCount;
  }
  m_range      = imageViewCreationInfo.subresourceRange;
  m_bfullRange = m_range == pTextureVulkan->GetFullRange();

  VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&imageViewCreationInfo, nullptr, &m_imageView));
  pVulkanDevice->SetDebugName("RTV", m_imageView);
  return XII_SUCCESS;
}

xiiResult xiiGALRenderTargetViewVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_imageView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_RenderTargetViewVulkan);
