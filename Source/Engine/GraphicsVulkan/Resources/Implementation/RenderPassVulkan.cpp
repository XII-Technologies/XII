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
    {
      VK_SUCCEED_OR_RETURN_XII_FAILURE(CreateRenderPassForVersion<1U>());
    }
    break;

    case 2:
    {
      VK_SUCCEED_OR_RETURN_XII_FAILURE(CreateRenderPassForVersion<2U>());
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALRenderPassVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(m_vkRenderPass);

  m_vkRenderPass = VK_NULL_HANDLE;

  return XII_SUCCESS;
}

void xiiGALRenderPassVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkRenderPass, sName.GetData(tmp));
}

template <xiiUInt8 RenderPassVersion>
vk::Result xiiGALRenderPassVulkan::CreateRenderPassForVersion()
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

  xiiDynamicArray<AttachmentReferenceType> vkAttachmentReferences(pDeviceVulkan->GetAllocator());
  vkAttachmentReferences.SetCount(uiTotalAttachmentReferencesCount + uiTotalShadingRateAttachmentsCount);

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

  for (xiiUInt32 i = 0, uiShadingRateIndex = 0; i < m_Description.m_SubPasses.GetCount(); ++i)
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

    auto UpdateAttachmentsStates = [&attachmentStates](xiiArrayPtr<const xiiGALAttachmentReferenceDescription> pSourceAttachments) -> void {
      if (pSourceAttachments.IsEmpty())
        return;

      for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < pSourceAttachments.GetCount(); ++uiAttachmentIndex)
      {
        const auto& sourceAttachmentReference = pSourceAttachments[uiAttachmentIndex];

        if (sourceAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
        {
          attachmentStates[sourceAttachmentReference.m_uiAttachmentIndex] |= sourceAttachmentReference.m_ResourceStateFlags;
        }
      }
    };

    UpdateAttachmentsStates(xiiSubPass.m_InputAttachments);
    UpdateAttachmentsStates(xiiSubPass.m_RenderTargetAttachments);
    UpdateAttachmentsStates(xiiSubPass.m_DepthStencilAttachment);

    auto ConvertAttachmentReferences = [&](xiiArrayPtr<const xiiGALAttachmentReferenceDescription> pSourceAttachments, vk::ImageAspectFlags aspectFlags) -> AttachmentReferenceType* {
      auto* pCurrentVkAttachmentReference = &vkAttachmentReferences[uiCurrentAttachmentReferenceIndex];

      for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < pSourceAttachments.GetCount(); ++uiAttachmentIndex, ++uiCurrentAttachmentReferenceIndex)
      {
        const auto& xiiSourceAttachmentReference     = pSourceAttachments[uiAttachmentIndex];
        auto&       vkDestinationAttachmentReference = vkAttachmentReferences[uiCurrentAttachmentReferenceIndex];

        if constexpr (std::is_same_v<AttachmentReferenceType, vk::AttachmentReference2>)
        {
          vkDestinationAttachmentReference.pNext      = nullptr;
          vkDestinationAttachmentReference.aspectMask = aspectFlags;
        }
        vkDestinationAttachmentReference.attachment = xiiSourceAttachmentReference.m_uiAttachmentIndex;

        xiiBitflags<xiiGALResourceStateFlags> state = xiiSourceAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED ? attachmentStates[xiiSourceAttachmentReference.m_uiAttachmentIndex] : xiiSourceAttachmentReference.m_ResourceStateFlags;

        if (xiiMath::CountBits(state.GetValue()) >= 2)
        {
          // The same attachment is used in different ways in this subpass (e.g. color and input attachments).
          // It must use COMMON layout.
          state = xiiGALResourceStateFlags::Common;
        }
        else
        {
          XII_ASSERT_DEV(state == xiiGALResourceStateFlags::Unknown || state == xiiSourceAttachmentReference.m_ResourceStateFlags, "");

          state = xiiSourceAttachmentReference.m_ResourceStateFlags;
        }

        vkDestinationAttachmentReference.layout = xiiVulkanTypeConversions::GetImageLayout(state, true, bFragmentDensityMapEnabled);
      }
      return pCurrentVkAttachmentReference;
    };

    vkSubPass.inputAttachmentCount = xiiSubPass.m_InputAttachments.GetCount();
    if (!xiiSubPass.m_InputAttachments.IsEmpty())
    {
      vkSubPass.pInputAttachments = ConvertAttachmentReferences(xiiSubPass.m_InputAttachments, vk::ImageAspectFlagBits::eColor);
    }

    vkSubPass.colorAttachmentCount = xiiSubPass.m_RenderTargetAttachments.GetCount();
    if (!xiiSubPass.m_RenderTargetAttachments.IsEmpty())
    {
      vkSubPass.pColorAttachments = ConvertAttachmentReferences(xiiSubPass.m_RenderTargetAttachments, vk::ImageAspectFlagBits::eColor);

      if (!xiiSubPass.m_ResolveAttachments.IsEmpty())
      {
        vkSubPass.pResolveAttachments = ConvertAttachmentReferences(xiiSubPass.m_ResolveAttachments, vk::ImageAspectFlagBits::eColor);
      }
    }

    if (!xiiSubPass.m_DepthStencilAttachment.IsEmpty())
    {
      vkSubPass.pDepthStencilAttachment = ConvertAttachmentReferences(xiiSubPass.m_DepthStencilAttachment, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
    }

    vkSubPass.preserveAttachmentCount = xiiSubPass.m_PreserveAttachments.GetCount();
    if (!xiiSubPass.m_PreserveAttachments.IsEmpty())
    {
      vkSubPass.pPreserveAttachments = &vkPreserveAttachments[uiCurrentPreserveAttachmentIndex];

      for (xiiUInt32 uiPreserveAttachmentIndex = 0; uiPreserveAttachmentIndex < xiiSubPass.m_PreserveAttachments.GetCount(); ++uiPreserveAttachmentIndex, ++uiCurrentPreserveAttachmentIndex)
      {
        vkPreserveAttachments[uiCurrentPreserveAttachmentIndex] = xiiSubPass.m_PreserveAttachments[uiPreserveAttachmentIndex];
      }
    }

    if (!xiiSubPass.m_ShadingRateAttachment.IsEmpty())
    {
      if (bShadingRateEnabled)
      {
        const auto& xiiShadingRateAttachment = xiiSubPass.m_ShadingRateAttachment[0];
        auto&       vkShadingRateAttachment  = vkShadingRate[uiShadingRateIndex];

        if constexpr (std::is_same_v<SubpassDescriptionType, vk::SubpassDescription2>)
        {
          vkSubPass.pNext = &vkShadingRateAttachment;
        }

        vkShadingRateAttachment.pNext                          = nullptr;
        vkShadingRateAttachment.pFragmentShadingRateAttachment = reinterpret_cast<const vk::AttachmentReference2*>(ConvertAttachmentReferences(xiiMakeArrayPtr(&xiiShadingRateAttachment.m_AttachmentReference, 1U), vk::ImageAspectFlagBits::eColor));
        vkShadingRateAttachment.shadingRateAttachmentTexelSize = vk::Extent2D{xiiShadingRateAttachment.m_TileSize.width, xiiShadingRateAttachment.m_TileSize.height};
      }
      else
      {
        XII_ASSERT_DEV(bFragmentDensityMapEnabled, "");

        pMainShadingRateAttachment = pMainShadingRateAttachment ? pMainShadingRateAttachment : xiiSubPass.m_ShadingRateAttachment.GetData();
      }
    }
  }

  if (bFragmentDensityMapEnabled && pMainShadingRateAttachment != nullptr)
  {
    for (xiiUInt32 i = 0; i < m_Description.m_SubPasses.GetCount(); ++i)
    {
      const auto& xiiSubPass = m_Description.m_SubPasses[i];

      XII_VERIFY(!xiiSubPass.m_ShadingRateAttachment.IsEmpty(), "Vk_EXT_fragment_density_map extension requires that shading rate attachment is specified for all subpasses!");
      XII_VERIFY(*pMainShadingRateAttachment == xiiSubPass.m_ShadingRateAttachment[0], "Vk_EXT_fragment_density_map extension requires that shading rate attachment is the same for all subpasses!");
    }
  }

  XII_ASSERT_DEV(uiCurrentAttachmentReferenceIndex == vkAttachmentReferences.GetCount(), "");
  XII_ASSERT_DEV(uiCurrentPreserveAttachmentIndex == vkPreserveAttachments.GetCount(), "");

  vkRenderPassCreateInfo.subpassCount = m_Description.m_SubPasses.GetCount();
  vkRenderPassCreateInfo.pSubpasses   = vkSubPasses.GetData();

  xiiDynamicArray<SubpassDependencyType> vkSubPassDependencies(pDeviceVulkan->GetAllocator());
  vkSubPassDependencies.SetCount(m_Description.m_Dependencies.GetCount());

  for (xiiUInt32 i = 0; i < m_Description.m_Dependencies.GetCount(); ++i)
  {
    const auto& xiiDependencyDescription = m_Description.m_Dependencies[i];
    auto&       vkDependencyDescription  = vkSubPassDependencies[i];

    if constexpr (std::is_same_v<SubpassDependencyType, vk::SubpassDependency2>)
    {
      vkDependencyDescription.pNext      = nullptr;
      vkDependencyDescription.viewOffset = 0; // For multi-view.
    }

    vkDependencyDescription.srcSubpass    = xiiDependencyDescription.m_uiSourceSubPass;
    vkDependencyDescription.dstSubpass    = xiiDependencyDescription.m_uiDestinationSubPass;
    vkDependencyDescription.srcStageMask  = xiiVulkanTypeConversions::GetPipelineStageFlags(xiiDependencyDescription.m_SourceStageFlags);
    vkDependencyDescription.dstStageMask  = xiiVulkanTypeConversions::GetPipelineStageFlags(xiiDependencyDescription.m_DestinationStageFlags);
    vkDependencyDescription.srcAccessMask = xiiVulkanTypeConversions::GetAccessFlags(xiiDependencyDescription.m_SourceAccessFlags);
    vkDependencyDescription.dstAccessMask = xiiVulkanTypeConversions::GetAccessFlags(xiiDependencyDescription.m_DestinationAccessFlags);

    // VK_DEPENDENCY_BY_REGION_BIT specifies that dependencies will be framebuffer-local.
    // Framebuffer-local dependencies are more optimal for most architectures; particularly tile-based architectures - which can keep framebuffer-regions entirely in on-chip registers
    // and thus avoid external bandwidth across such a dependency. Including a framebuffer-global dependency in your rendering will usually force all implementations to flush data to memory,
    // or to a higher level cache, breaking any potential locality optimizations.
    vkDependencyDescription.dependencyFlags = vk::DependencyFlagBits::eByRegion;
  }
  vkRenderPassCreateInfo.dependencyCount = m_Description.m_Dependencies.GetCount();
  vkRenderPassCreateInfo.pDependencies   = vkSubPassDependencies.GetData();

  // Enable fragment density map.
  vk::RenderPassFragmentDensityMapCreateInfoEXT vkFragmentDensityMapCreateInfo = {};
  if (bFragmentDensityMapEnabled && pMainShadingRateAttachment != nullptr)
  {
    vkRenderPassCreateInfo.pNext = &vkFragmentDensityMapCreateInfo;

    vkFragmentDensityMapCreateInfo.pNext                                   = nullptr;
    vkFragmentDensityMapCreateInfo.fragmentDensityMapAttachment.attachment = pMainShadingRateAttachment->m_AttachmentReference.m_uiAttachmentIndex;
    vkFragmentDensityMapCreateInfo.fragmentDensityMapAttachment.layout     = vk::ImageLayout::eFragmentDensityMapOptimalEXT;
  }

  if constexpr (std::is_same_v<RenderPassCIType, vk::RenderPassCreateInfo2>)
  {
    XII_VERIFY(vkLogicalDeviceExtensionFeatures.m_bRenderPass2 != false, "");

    return vkLogicalDevice.createRenderPass2(&vkRenderPassCreateInfo, nullptr, &m_vkRenderPass, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    return vkLogicalDevice.createRenderPass(&vkRenderPassCreateInfo, nullptr, &m_vkRenderPass, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_RenderPassVulkan);
