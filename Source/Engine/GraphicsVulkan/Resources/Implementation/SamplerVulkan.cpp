#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSamplerVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALSamplerVulkan::xiiGALSamplerVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(pDeviceVulkan, creationDescription)
{
}

xiiGALSamplerVulkan::~xiiGALSamplerVulkan() = default;

xiiResult xiiGALSamplerVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*                 pDeviceVulkan              = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::PhysicalDevice                  vkPhysicalDevice           = pDeviceVulkan->GetVulkanPhysicalDevice();
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
  vkSamplerCreateInfo.borderColor             = xiiVulkanTypeConversions::GetBorderColor(m_Description.m_BorderColor);
  vkSamplerCreateInfo.unnormalizedCoordinates = m_Description.m_bUnormalizedCoords;

  if (m_Description.m_Flags.IsSet(xiiGALSamplerFlags::Subsampled))
  {
    vkSamplerCreateInfo.flags |= vk::SamplerCreateFlagBits::eSubsampledEXT;
  }
  if (m_Description.m_Flags.IsSet(xiiGALSamplerFlags::SubsampledCoarseReconstruction))
  {
    vkSamplerCreateInfo.flags |= vk::SamplerCreateFlagBits::eSubsampledCoarseReconstructionEXT;
  }

  m_vkDescriptorImageInfo.imageLayout = vk::ImageLayout::eUndefined;

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSampler(&vkSamplerCreateInfo, nullptr, &m_vkDescriptorImageInfo.sampler, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiResult xiiGALSamplerVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkDescriptorImageInfo.sampler != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkDescriptorImageInfo.sampler);

    m_vkDescriptorImageInfo = vk::DescriptorImageInfo{};
  }

  return XII_SUCCESS;
}

void xiiGALSamplerVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkDescriptorImageInfo.sampler, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_SamplerVulkan);
