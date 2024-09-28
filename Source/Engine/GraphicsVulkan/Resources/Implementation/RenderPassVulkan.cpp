#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>

xiiGALRenderPassVulkan::xiiGALRenderPassVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(pDeviceVulkan, creationDescription)
{
}

xiiGALRenderPassVulkan::~xiiGALRenderPassVulkan() = default;

xiiResult xiiGALRenderPassVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*                          pDeviceVulkan                    = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  const xiiGALDeviceVulkan::ExtensionFeatures& vkLogicalDeviceExtensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

  xiiUInt32 uiRenderPassVersion = 1U;
  if (vkLogicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate)
  {
    for (xiiUInt32 i = 0; i < m_Description.m_SubPasses.GetCount() && uiRenderPassVersion < 2U; ++i)
    {
      const auto& subpass = m_Description.m_SubPasses[i];

      if (subpass.m_ShadingRateAttachment.IsEmpty())
      {
        uiRenderPassVersion = 2U;
      }

      XII_ASSERT_DEV(uiRenderPassVersion < 2 || vkLogicalDeviceExtensionFeatures.m_bRenderPass2 != vk::False, "This render pass requires the RenderPass2 Vulkan feature but is not currently enabled.");
    }
  }
  else if (vkLogicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap)
  {
    // Fragment density map is defined through RenderPassCreateInfo.pNext.
  }

  switch (uiRenderPassVersion)
  {
    case 1:
      CreateRenderPassForVersion<1U>();
      break;

    case 2:
      CreateRenderPassForVersion<2U>();
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALRenderPassVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkRenderPass));

  return XII_SUCCESS;
}

template <xiiUInt8 RenderPassVersion>
void xiiGALRenderPassVulkan::CreateRenderPassForVersion()
{
  using RenderPassCIType          = std::conditional_t<RenderPassVersion == 2, vk::RenderPassCreateInfo2, vk::RenderPassCreateInfo>;
  using SubpassDescriptionType    = std::conditional_t<RenderPassVersion == 2, vk::SubpassDescription2, vk::SubpassDescription>;
  using AttachmentDescriptionType = std::conditional_t<RenderPassVersion == 2, vk::AttachmentDescription2, vk::AttachmentDescription>;
  using AttachmentReferenceType   = std::conditional_t<RenderPassVersion == 2, vk::AttachmentReference2, vk::AttachmentReference>;
  using SubpassDependencyType     = std::conditional_t<RenderPassVersion == 2, vk::SubpassDependency2, vk::SubpassDependency>;

  xiiGALDeviceVulkan*                          pDeviceVulkan                    = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device                                   vkLogicalDevice                  = pDeviceVulkan->GetVulkanLogicalDevice();
  const xiiGALDeviceVulkan::ExtensionFeatures& vkLogicalDeviceExtensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();
  const bool                                   bShadingRateEnabled              = vkLogicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False;
  const bool                                   bFragmentDensityMapEnabled       = vkLogicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False;

  RenderPassCIType vkRenderPassCreateInfo = {};
  vkRenderPassCreateInfo.pNext            = nullptr;
  vkRenderPassCreateInfo.flags            = {};

  xiiHybridArray<AttachmentDescriptionType, 2U> vkAttachments(pDeviceVulkan->GetAllocator());
  for (xiiUInt32 i = 0; i < m_Description.m_Attachments.GetCount(); ++i)
  {
    const auto& xiiAttachment = m_Description.m_Attachments[i];
    auto&       vkAttachment  = vkAttachments.ExpandAndGetRef();

    vkAttachment.flags          = {};
    vkAttachment.format         = xiiVulkanTypeConversions::GetFormat(xiiAttachment.m_Format);
    vkAttachment.samples        = static_cast<vk::SampleCountFlagBits>(xiiAttachment.m_uiSampleCount);
    vkAttachment.loadOp         = xiiVulkanTypeConversions::GetAttachmentLoadOperation(xiiAttachment.m_LoadOperation);
    vkAttachment.storeOp        = xiiVulkanTypeConversions::GetAttachmentStoreOperation(xiiAttachment.m_StoreOperation);
    vkAttachment.stencilLoadOp  = xiiVulkanTypeConversions::GetAttachmentLoadOperation(xiiAttachment.m_StencilLoadOperation);
    vkAttachment.stencilStoreOp = xiiVulkanTypeConversions::GetAttachmentStoreOperation(xiiAttachment.m_StencilStoreOperation);
    vkAttachment.initialLayout  = xiiVulkanTypeConversions::GetImageLayout(xiiAttachment.m_InitialStateFlags, false, bFragmentDensityMapEnabled);
    vkAttachment.finalLayout    = xiiVulkanTypeConversions::GetImageLayout(xiiAttachment.m_FinalStateFlags, true, bFragmentDensityMapEnabled);
  }
  vkRenderPassCreateInfo.attachmentCount = m_Description.m_Attachments.GetCount();
  vkRenderPassCreateInfo.pAttachments    = vkAttachments.GetData();

  xiiUInt32 uiTotalAttachmentReferencesCount   = 0U;
  xiiUInt32 uiTotalPreserveAttachmentsCount    = 0U;
  xiiUInt32 uiTotalShadingRateAttachmentsCount = 0U;
  for (xiiUInt32 i = 0; i < m_Description.m_SubPasses.GetCount(); ++i)
  {
    const auto& xiiSubPass = m_Description.m_SubPasses[i];

    uiTotalAttachmentReferencesCount += xiiSubPass.m_InputAttachments.GetCount();
    uiTotalAttachmentReferencesCount += xiiSubPass.m_RenderTargetAttachments.GetCount();

    if (!xiiSubPass.m_ResolveAttachments.IsEmpty())
    {
      uiTotalAttachmentReferencesCount += xiiSubPass.m_RenderTargetAttachments.GetCount();
    }
    if (!xiiSubPass.m_DepthStencilAttachment.IsEmpty())
    {
      uiTotalAttachmentReferencesCount += 1;
    }
    if (!xiiSubPass.m_ShadingRateAttachment.IsEmpty() && bShadingRateEnabled)
    {
      uiTotalShadingRateAttachmentsCount += 1;
    }
    uiTotalPreserveAttachmentsCount += xiiSubPass.m_PreserveAttachments.GetCount();
  }

  xiiDynamicArray<AttachmentReferenceType> vkAttachmentReference(pDeviceVulkan->GetAllocator());
  vkAttachmentReference.SetCount(uiTotalAttachmentReferencesCount + uiTotalShadingRateAttachmentsCount);

  xiiDynamicArray<xiiUInt32> vkPreserveAttachments(pDeviceVulkan->GetAllocator());
  vkPreserveAttachments.SetCount(uiTotalPreserveAttachmentsCount);

  xiiDynamicArray<vk::FragmentShadingRateAttachmentInfoKHR> vkShadingRate(pDeviceVulkan->GetAllocator());
  vkShadingRate.SetCount(uiTotalShadingRateAttachmentsCount);

  const xiiGALShadingRateAttachmentDescription* pMainShadingRateAttachment = nullptr;

  xiiUInt32 uiCurrentAttachmentReferenceIndex = 0;
  xiiUInt32 uiCurrentPreserveAttachmentIndex  = 0;

  // State flags for every attachment in each subpass.
  // This array is used to detect attachments that are used as render target or depth-stencil, but also as input attachment in the same subpass. Such attachments need to use GENERAL layout.
  xiiDynamicArray<xiiBitflags<xiiGALResourceStateFlags>> attachmentStates(pDeviceVulkan->GetAllocator());
  attachmentStates.SetCount(m_Description.m_Attachments.GetCount());

  xiiDynamicArray<SubpassDescriptionType> vkSubPasses(pDeviceVulkan->GetAllocator());
  vkSubPasses.SetCount(m_Description.m_SubPasses.GetCount());

  for (xiiUInt32 i = 0; i < m_Description.m_SubPasses.GetCount(); ++i)
  {
    const auto& xiiSubPass = m_Description.m_SubPasses[i];
    auto&       vkSubPass  = vkSubPasses[i];

    vkSubPass.flags             = {};
    vkSubPass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    if constexpr (std::is_same_v<SubpassDescriptionType, vk::SubpassDescription2>)
    {
      vkSubPass.pNext = nullptr;
    }

    for (xiiUInt32 j = 0; j < attachmentStates.GetCount(); ++j)
    {
      attachmentStates[i] = xiiGALResourceStateFlags::Unknown;
    }

    auto UpdateAttachmentsStates = [&attachmentStates](xiiUInt32 uiAttachmentCount, const xiiGALAttachmentReferenceDescription* pSourceAttachments) -> void {
      if (pSourceAttachments == nullptr)
        return;

      for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < uiAttachmentCount; ++uiAttachmentIndex)
      {
        const auto& sourceAttachmentReference = pSourceAttachments[uiAttachmentIndex];

        if (sourceAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
        {
          attachmentStates[sourceAttachmentReference.m_uiAttachmentIndex] |= sourceAttachmentReference.m_ResourceStateFlags;
        }
      }
    };
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_RenderPassVulkan);
