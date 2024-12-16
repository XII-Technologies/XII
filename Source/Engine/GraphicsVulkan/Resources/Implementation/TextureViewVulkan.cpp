#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALTextureViewVulkan::xiiGALTextureViewVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pDeviceVulkan, pTexture, creationDescription)
{
}

xiiGALTextureViewVulkan::~xiiGALTextureViewVulkan() = default;

xiiResult xiiGALTextureViewVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*  pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALTextureVulkan* pTextureVulkan     = static_cast<xiiGALTextureVulkan*>(pDeviceVulkan->GetTexture(m_Description.m_hTexture));
  const auto&          textureDescription = pTextureVulkan->GetDescription();

  if (m_Description.m_Format == xiiGALTextureFormat::Unknown)
  {
    m_Description.m_Format = textureDescription.m_Format;
  }

  vk::ImageViewCreateInfo vkImageViewCreateInfo = {};
  vkImageViewCreateInfo.flags                   = {};
  vkImageViewCreateInfo.pNext                   = nullptr;
  vkImageViewCreateInfo.image                   = pTextureVulkan->GetVulkanImage();

  switch (m_Description.m_ResourceDimension)
  {
    case xiiGALResourceDimension::Texture1D:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::e1D;
    }
    break;
    case xiiGALResourceDimension::Texture1DArray:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::e1DArray;
    }
    break;
    case xiiGALResourceDimension::Texture2D:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    }
    break;
    case xiiGALResourceDimension::Texture2DArray:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
    }
    break;
    case xiiGALResourceDimension::Texture3D:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::e3D;
      if (m_Description.m_ViewType == xiiGALTextureViewType::RenderTarget || m_Description.m_ViewType == xiiGALTextureViewType::DepthStencil || m_Description.m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil)
      {
        if (!pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_bTextureView2DOn3DSupported)
        {
          xiiLog::Error("The device does not support 2D texture views on 3D textures.");
          return XII_FAILURE;
        }

        if (textureDescription.m_Usage == xiiGALResourceUsage::Sparse)
        {
          xiiLog::Error("Can not create 2D texture view on a 3D sparse texture.");
          return XII_FAILURE;
        }

        m_Description.m_ResourceDimension = xiiGALResourceDimension::Texture2DArray;
        vkImageViewCreateInfo.viewType    = vk::ImageViewType::e2DArray;
      }
      else
      {
        vkImageViewCreateInfo.viewType = vk::ImageViewType::e3D;

        xiiUInt32 uiMipDepth = xiiMath::Max(textureDescription.m_uiArraySizeOrDepth >> m_Description.m_uiMostDetailedMip, 1U);
        if (m_Description.m_uiFirstArrayOrDepthSlice != 0 || m_Description.m_uiArrayOrDepthSlicesCount != uiMipDepth)
        {
          xiiLog::Error("3D texture view '{}' (most detailed mip: {}; mip levels: {}; first slice: {}, depth slice count: {}) of texture '{}' does not reference all depth slices ({}) in the mip level. 3D texture views in Vulkan must address all depth slices.",
                        GetDebugName(), m_Description.m_uiMostDetailedMip, m_Description.m_uiMipLevelCount, m_Description.m_uiFirstArrayOrDepthSlice, m_Description.m_uiArrayOrDepthSlicesCount, pTextureVulkan->GetDebugName(), uiMipDepth);

          m_Description.m_uiFirstArrayOrDepthSlice  = 0;
          m_Description.m_uiArrayOrDepthSlicesCount = uiMipDepth;
        }
      }
    }
    break;
    case xiiGALResourceDimension::TextureCube:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::eCube;
    }
    break;
    case xiiGALResourceDimension::TextureCubeArray:
    {
      vkImageViewCreateInfo.viewType = vk::ImageViewType::eCubeArray;
    }
    break;

    default:
      XII_REPORT_FAILURE("Unexpected texture view dimension.");
      return XII_FAILURE;
  }

  xiiEnum<xiiGALTextureFormat> correctedViewFormat = m_Description.m_Format;
  if (textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    correctedViewFormat = xiiGALTextureUtilities::GetDefaultTextureViewFormat(correctedViewFormat, xiiGALTextureViewType::DepthStencil, textureDescription.m_BindFlags);
  }
  vkImageViewCreateInfo.format = xiiVulkanTypeConversions::GetFormat(correctedViewFormat);

  if (m_Description.m_Format == xiiGALTextureFormat::A8UNormalized)
  {
    auto GetTextureFormatA8Swizzle = [](xiiGALTextureComponentSwizzle::Enum component, xiiGALTextureComponentSwizzle::Enum swizzle) -> vk::ComponentSwizzle {
      if (swizzle == xiiGALTextureComponentSwizzle::Zero || swizzle == xiiGALTextureComponentSwizzle::One)
      {
        return xiiVulkanTypeConversions::GetComponentSwizzle(swizzle);
      }
      if (swizzle == xiiGALTextureComponentSwizzle::A || (component == xiiGALTextureComponentSwizzle::A && swizzle == xiiGALTextureComponentSwizzle::Identity))
      {
        return vk::ComponentSwizzle::eA;
      }
      return vk::ComponentSwizzle::eZero;
    };

    vkImageViewCreateInfo.components = {
      GetTextureFormatA8Swizzle(xiiGALTextureComponentSwizzle::R, m_Description.m_ComponentSwizzle.m_R),
      GetTextureFormatA8Swizzle(xiiGALTextureComponentSwizzle::G, m_Description.m_ComponentSwizzle.m_G),
      GetTextureFormatA8Swizzle(xiiGALTextureComponentSwizzle::B, m_Description.m_ComponentSwizzle.m_B),
      GetTextureFormatA8Swizzle(xiiGALTextureComponentSwizzle::A, m_Description.m_ComponentSwizzle.m_A),
    };
  }
  else
  {
    vkImageViewCreateInfo.components = xiiVulkanTypeConversions::GetComponentMapping(m_Description.m_ComponentSwizzle);
  }

  vkImageViewCreateInfo.subresourceRange.baseMipLevel = m_Description.m_uiMostDetailedMip;
  vkImageViewCreateInfo.subresourceRange.levelCount   = m_Description.m_uiMipLevelCount;

  if (textureDescription.IsArray())
  {
    vkImageViewCreateInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstArrayOrDepthSlice;
    vkImageViewCreateInfo.subresourceRange.layerCount     = m_Description.m_uiArrayOrDepthSlicesCount;
  }
  else
  {
    vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    vkImageViewCreateInfo.subresourceRange.layerCount     = 1;
  }

  const auto& textureFormatProperties = xiiGALTextureUtilities::GetTextureFormatProperties(correctedViewFormat);

  if (m_Description.m_ViewType == xiiGALTextureViewType::DepthStencil || m_Description.m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil)
  {
    // When an imageView of a depth/stencil image is used as a depth/stencil framebuffer attachment, the aspectMask is ignored and both depth and stencil image subresources are used. (11.5)
    if (textureFormatProperties.m_ComponentType == xiiGALTextureFormatComponentType::Depth)
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (textureFormatProperties.m_ComponentType == xiiGALTextureFormatComponentType::DepthStencil)
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }
    else
    {
      xiiLog::Error("Unexpected component type for a depth-stencil view format.");
      return XII_FAILURE;
    }
  }
  else
  {
    // The aspectMask must be only VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT if format is a color, depth-only or stencil-only format, respectively. (11.5)
    if (textureFormatProperties.m_ComponentType == xiiGALTextureFormatComponentType::Depth)
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (textureFormatProperties.m_ComponentType == xiiGALTextureFormatComponentType::DepthStencil)
    {
      if (m_Description.m_Format == xiiGALTextureFormat::D32FloatS8X24UInt || m_Description.m_Format == xiiGALTextureFormat::D24UNormalizedS8UInt)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
      }
      else if (m_Description.m_Format == xiiGALTextureFormat::R32FloatX8X24Typeless || m_Description.m_Format == xiiGALTextureFormat::R24UNormalizedX8Typeless)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
      }
      else if (m_Description.m_Format == xiiGALTextureFormat::X32TypelessG8X24UInt || m_Description.m_Format == xiiGALTextureFormat::X24TypelessG8UInt)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
      }
      else
      {
        xiiLog::Error("Unexpected depth-stencil texture format.");
        return XII_FAILURE;
      }
    }
  }

  if (m_Description.m_ViewType == xiiGALTextureViewType::ShadingRate)
  {
    if (pDeviceVulkan->GetPhysicalDeviceExtensionFeatures().m_FragmentDensityMap.fragmentDensityMap != vk::False)
    {
      xiiEnum<xiiGALShadingRateTextureAccess> shadingRateTextureAccess = pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_ShadingRateProperties.m_TextureAccess;

      switch (shadingRateTextureAccess)
      {
        case xiiGALShadingRateTextureAccess::OnGPU:
        {
          vkImageViewCreateInfo.flags |= vk::ImageViewCreateFlagBits::eFragmentDensityMapDynamicEXT;
        }
        break;
        case xiiGALShadingRateTextureAccess::OnSubmit:
        {
          vkImageViewCreateInfo.flags |= vk::ImageViewCreateFlagBits::eFragmentDensityMapDeferredEXT;
        }
        break;
        case xiiGALShadingRateTextureAccess::OnSetRenderTarget:
        {
          // Nothing to set.
        }
        break;

        default:
        {
          xiiLog::Error("Unexpected shading rate access type.");
          return XII_FAILURE;
        }
      }
    }
  }

  vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();
  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createImageView(&vkImageViewCreateInfo, nullptr, &m_vkImageView, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImageView));

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureViewVulkan);
