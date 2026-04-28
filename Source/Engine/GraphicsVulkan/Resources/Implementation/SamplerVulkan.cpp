/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSamplerVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALSamplerVulkan::xiiGALSamplerVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALSamplerVulkan::~xiiGALSamplerVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkSampler));
}

xiiResult xiiGALSamplerVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*                 pDeviceVulkan              = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                          vkLogicalDevice            = pDeviceVulkan->GetVulkanLogicalDevice();
  const vk::PhysicalDeviceProperties& vkPhysicalDeviceProperties = pDeviceVulkan->GetVulkanPhysicalDeviceProperties();

  vk::SamplerCreateInfo vkSamplerCreateInfo = {};
  vkSamplerCreateInfo.pNext                 = nullptr;
  vkSamplerCreateInfo.flags                 = {};
  vkSamplerCreateInfo.magFilter             = xiiVulkanTypeConversions::GetFilter(m_Description.m_MagFilter);
  vkSamplerCreateInfo.minFilter             = xiiVulkanTypeConversions::GetFilter(m_Description.m_MinFilter);
  vkSamplerCreateInfo.mipmapMode            = xiiVulkanTypeConversions::GetSamplerMipMapMode(m_Description.m_MipFilter);
  vkSamplerCreateInfo.addressModeU          = xiiVulkanTypeConversions::GetSamplerAddressMode(m_Description.m_AddressU);
  vkSamplerCreateInfo.addressModeV          = xiiVulkanTypeConversions::GetSamplerAddressMode(m_Description.m_AddressV);
  vkSamplerCreateInfo.addressModeW          = xiiVulkanTypeConversions::GetSamplerAddressMode(m_Description.m_AddressW);
  vkSamplerCreateInfo.mipLodBias            = m_Description.m_fMipLODBias;

  vkSamplerCreateInfo.anisotropyEnable = xiiGALFilterType::IsAnisotropicFilter(m_Description.m_MinFilter);
  vkSamplerCreateInfo.maxAnisotropy    = vkSamplerCreateInfo.anisotropyEnable ? xiiMath::Clamp(static_cast<float>(m_Description.m_uiMaxAnisotropy), 1.0f, vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy) : 0.0f;

  XII_ASSERT_DEV((vkSamplerCreateInfo.anisotropyEnable != vk::False) == xiiGALFilterType::IsAnisotropicFilter(m_Description.m_MagFilter), "Min and Mag filters must be both either anisotropic filters, or non-anisotropic filters.");

  vkSamplerCreateInfo.compareEnable = xiiGALFilterType::IsComparisonFilter(m_Description.m_MinFilter);

  XII_ASSERT_DEV((vkSamplerCreateInfo.compareEnable != vk::False) == xiiGALFilterType::IsComparisonFilter(m_Description.m_MagFilter), "Min and Mag filters must be both either comparison filters, or non-comparison filters.");

  vkSamplerCreateInfo.compareOp               = xiiVulkanTypeConversions::GetCompareOp(m_Description.m_ComparisonFunction);
  vkSamplerCreateInfo.minLod                  = m_Description.m_bUnormalizedCoords ? 0U : m_Description.m_fMinLOD;
  vkSamplerCreateInfo.maxLod                  = m_Description.m_bUnormalizedCoords ? 0U : m_Description.m_fMaxLOD;
  vkSamplerCreateInfo.borderColor             = vk::BorderColor::eFloatTransparentBlack;
  vkSamplerCreateInfo.unnormalizedCoordinates = m_Description.m_bUnormalizedCoords;

  vk::SamplerCustomBorderColorCreateInfoEXT vkSamplerCustomBorderColor = {};
  if (vkSamplerCreateInfo.addressModeU == vk::SamplerAddressMode::eClampToBorder || vkSamplerCreateInfo.addressModeV == vk::SamplerAddressMode::eClampToBorder || vkSamplerCreateInfo.addressModeW == vk::SamplerAddressMode::eClampToBorder)
  {
    const xiiColor color = m_Description.m_BorderColor;

    if (color == xiiColor(0, 0, 0, 0))
    {
      vkSamplerCreateInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
    }
    else if (color == xiiColor(0, 0, 0, 1))
    {
      vkSamplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    }
    else if (color == xiiColor(1, 1, 1, 1))
    {
      vkSamplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    }
    else if (pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures().m_CustomBorderColor.customBorderColors == vk::True)
    {
      vkSamplerCustomBorderColor.customBorderColor.float32[0] = color.r;
      vkSamplerCustomBorderColor.customBorderColor.float32[1] = color.g;
      vkSamplerCustomBorderColor.customBorderColor.float32[2] = color.b;
      vkSamplerCustomBorderColor.customBorderColor.float32[3] = color.a;

      vkSamplerCreateInfo.borderColor = vk::BorderColor::eFloatCustomEXT;
      vkSamplerCreateInfo.pNext       = &vkSamplerCustomBorderColor;
    }
    else
    {
      // Fallback to close enough.
      const bool bTransparent = m_Description.m_BorderColor.a == 0.0f;
      const bool bBlack       = m_Description.m_BorderColor.r == 0.0f;

      if (bBlack)
      {
        vkSamplerCreateInfo.borderColor = bTransparent ? vk::BorderColor::eFloatTransparentBlack : vk::BorderColor::eFloatOpaqueBlack;
      }
      else
      {
        vkSamplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
      }
    }
  }

  if (m_Description.m_Flags.IsSet(xiiGALSamplerFlags::Subsampled))
  {
    vkSamplerCreateInfo.flags |= vk::SamplerCreateFlagBits::eSubsampledEXT;
  }
  if (m_Description.m_Flags.IsSet(xiiGALSamplerFlags::SubsampledCoarseReconstruction))
  {
    vkSamplerCreateInfo.flags |= vk::SamplerCreateFlagBits::eSubsampledCoarseReconstructionEXT;
  }

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSampler(&vkSamplerCreateInfo, nullptr, &m_vkSampler, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

void xiiGALSamplerVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkSampler, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_SamplerVulkan);
