#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureViewVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTextureViewVulkan::xiiGALTextureViewVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pDeviceVulkan, pTexture, creationDescription)
{
}

xiiGALTextureViewVulkan::~xiiGALTextureViewVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkImageView));
}

xiiResult xiiGALTextureViewVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan>  pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiSharedPtr<xiiGALTextureVulkan> pTextureVulkan     = m_pTexture.Downcast<xiiGALTextureVulkan>();
  const auto&                       textureDescription = pTextureVulkan->GetDescription();

  if (m_Description.m_Format == xiiGALResourceFormat::Unknown)
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

  xiiEnum<xiiGALResourceFormat> correctedViewFormat = m_Description.m_Format;
  if (textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    correctedViewFormat = xiiGALTextureUtilities::GetDefaultTextureViewFormat(correctedViewFormat, xiiGALTextureViewType::DepthStencil, textureDescription.m_BindFlags);
  }
  vkImageViewCreateInfo.format = xiiVulkanTypeConversions::GetFormat(correctedViewFormat);

  if (m_Description.m_Format == xiiGALResourceFormat::A8UNormalized)
  {
    auto GetResourceFormatA8Swizzle = [](xiiGALTextureComponentSwizzle::Enum component, xiiGALTextureComponentSwizzle::Enum swizzle) -> vk::ComponentSwizzle {
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
      GetResourceFormatA8Swizzle(xiiGALTextureComponentSwizzle::R, m_Description.m_ComponentSwizzle.m_R),
      GetResourceFormatA8Swizzle(xiiGALTextureComponentSwizzle::G, m_Description.m_ComponentSwizzle.m_G),
      GetResourceFormatA8Swizzle(xiiGALTextureComponentSwizzle::B, m_Description.m_ComponentSwizzle.m_B),
      GetResourceFormatA8Swizzle(xiiGALTextureComponentSwizzle::A, m_Description.m_ComponentSwizzle.m_A),
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

  const auto& textureFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(correctedViewFormat);

  if (m_Description.m_ViewType == xiiGALTextureViewType::DepthStencil || m_Description.m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil)
  {
    // When an imageView of a depth/stencil image is used as a depth/stencil framebuffer attachment, the aspectMask is ignored and both depth and stencil image subresources are used. (11.5)
    if (textureFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (textureFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
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
    if (textureFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (textureFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      if (m_Description.m_Format == xiiGALResourceFormat::D32FloatS8X24UInt || m_Description.m_Format == xiiGALResourceFormat::D24UNormalizedS8UInt)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
      }
      else if (m_Description.m_Format == xiiGALResourceFormat::R32FloatX8X24Typeless || m_Description.m_Format == xiiGALResourceFormat::R24UNormalizedX8Typeless)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
      }
      else if (m_Description.m_Format == xiiGALResourceFormat::X32TypelessG8X24UInt || m_Description.m_Format == xiiGALResourceFormat::X24TypelessG8UInt)
      {
        vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
      }
      else
      {
        xiiLog::Error("Unexpected depth-stencil texture format.");
        return XII_FAILURE;
      }
    }
    else
    {
      vkImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
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

  m_vkDescriptorImageInfo.imageView   = m_vkImageView;
  m_vkDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral; // TODO

  return XII_SUCCESS;
}

void xiiGALTextureViewVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkImageView, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureViewVulkan);
