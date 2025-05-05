#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/GraphicsPipelineStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALGraphicsPipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALGraphicsPipelineStateVulkan::xiiGALGraphicsPipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription) :
  xiiGALGraphicsPipelineState(pDeviceVulkan, creationDescription), m_vkPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
{
}

xiiGALGraphicsPipelineStateVulkan::~xiiGALGraphicsPipelineStateVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineCache));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipeline));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineLayout));
}

xiiResult xiiGALGraphicsPipelineStateVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan>                    pDeviceVulkan                    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                                          vkLogicalDevice                  = pDeviceVulkan->GetVulkanLogicalDevice();
  xiiSharedPtr<xiiGALPipelineResourceSignatureVulkan> pPipelineResourceSignatureVulkan = m_Description.m_pPipelineResourceSignature.Downcast<xiiGALPipelineResourceSignatureVulkan>();

  vk::GraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo = {};
  vkGraphicsPipelineCreateInfo.pNext                          = nullptr;
  vkGraphicsPipelineCreateInfo.flags                          = {};
  vkGraphicsPipelineCreateInfo.basePipelineHandle             = nullptr; // A pipeline to derive from.
  vkGraphicsPipelineCreateInfo.basePipelineIndex              = {};      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  vkGraphicsPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

  xiiDynamicArray<vk::PipelineShaderStageCreateInfo> vkShaderStages(pDeviceVulkan->GetAllocator());
  {
#define DEFINE_VULKAN_SHADER_IF_EXISTS(shaderType, shaderStageFlagBits)                                              \
  if (xiiSharedPtr<xiiGALShaderVulkan> pShaderVulkan = m_Description.m_p##shaderType.Downcast<xiiGALShaderVulkan>()) \
  {                                                                                                                  \
    vk::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};                                          \
    vkPipelineShaderStageCreateInfo.pNext                             = nullptr;                                     \
    vkPipelineShaderStageCreateInfo.flags                             = {};                                          \
    vkPipelineShaderStageCreateInfo.stage                             = shaderStageFlagBits;                         \
    vkPipelineShaderStageCreateInfo.pName                             = "main";                                      \
    vkPipelineShaderStageCreateInfo.module                            = pShaderVulkan->GetVulkanShaderModule();      \
    vkPipelineShaderStageCreateInfo.pSpecializationInfo               = nullptr;                                     \
                                                                                                                     \
    vkShaderStages.PushBack(vkPipelineShaderStageCreateInfo);                                                        \
  }

    DEFINE_VULKAN_SHADER_IF_EXISTS(VertexShader, vk::ShaderStageFlagBits::eVertex);
    DEFINE_VULKAN_SHADER_IF_EXISTS(PixelShader, vk::ShaderStageFlagBits::eFragment);
    DEFINE_VULKAN_SHADER_IF_EXISTS(DomainShader, vk::ShaderStageFlagBits::eTessellationEvaluation);
    DEFINE_VULKAN_SHADER_IF_EXISTS(HullShader, vk::ShaderStageFlagBits::eTessellationControl);
    DEFINE_VULKAN_SHADER_IF_EXISTS(GeometryShader, vk::ShaderStageFlagBits::eGeometry);
    DEFINE_VULKAN_SHADER_IF_EXISTS(AmplificationShader, vk::ShaderStageFlagBits::eTaskEXT);
    DEFINE_VULKAN_SHADER_IF_EXISTS(MeshShader, vk::ShaderStageFlagBits::eMeshEXT);

#undef DEFINE_VULKAN_SHADER_IF_EXISTS
  }
  vkGraphicsPipelineCreateInfo.stageCount = vkShaderStages.GetCount();
  vkGraphicsPipelineCreateInfo.pStages    = vkShaderStages.IsEmpty() ? nullptr : vkShaderStages.GetData();

  vk::PipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo = {};
  if (xiiSharedPtr<xiiGALInputLayoutVulkan> pInputLayoutVulkan = m_Description.m_GraphicsPipeline.m_pInputLayout.Downcast<xiiGALInputLayoutVulkan>())
  {
    auto pVertexAttributes = pInputLayoutVulkan->GetVulkanVertexAttributes();
    auto pInputBindings    = pInputLayoutVulkan->GetVulkanVertexInputBindings();

    vkPipelineVertexInputStateCreateInfo.pNext                           = nullptr;
    vkPipelineVertexInputStateCreateInfo.flags                           = {};
    vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = pVertexAttributes.GetCount();
    vkPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions    = pVertexAttributes.GetPtr();
    vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount   = pInputBindings.GetCount();
    vkPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions      = pInputBindings.GetPtr();
  }
  vkGraphicsPipelineCreateInfo.pVertexInputState = &vkPipelineVertexInputStateCreateInfo;

  vk::PipelineInputAssemblyStateCreateInfo vkPipelineInputAssemblyStateCreateInfo = {};
  {
    vkPipelineInputAssemblyStateCreateInfo.pNext                  = nullptr;
    vkPipelineInputAssemblyStateCreateInfo.flags                  = {};
    vkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = (m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStrip || m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStripAdjacent || m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStrip || m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStripAdjacent) ? vk::True : vk::False;
  }
  vkGraphicsPipelineCreateInfo.pInputAssemblyState = &vkPipelineInputAssemblyStateCreateInfo;

  vk::PipelineTessellationStateCreateInfo vkPipelineTessellationStateCreateInfo = {};
  {
    vkPipelineTessellationStateCreateInfo.pNext = nullptr;
    vkPipelineTessellationStateCreateInfo.flags = {};

    if (m_Description.m_PipelineType == xiiGALPipelineType::Mesh)
    {
      // Input assembly is not used in the mesh pipeline, so topology may contain any value.
      // Validation layers may generate a warning if vk::ePointList topology is used, so use MAX_ENUM value.
      vkPipelineInputAssemblyStateCreateInfo.topology = (vk::PrimitiveTopology)VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

      // Vertex input state and tessellation state are ignored in a mesh pipeline and should be null, but there is a bug in validation layers that makes them crash.
      // vkGraphicsPipelineCreateInfo.pVertexInputState = nullptr;
      vkGraphicsPipelineCreateInfo.pTessellationState = nullptr;
    }
    else
    {
      xiiVulkanTypeConversions::GetPrimitiveTopologyAndControlPatchPointsCount(m_Description.m_GraphicsPipeline.m_PrimitiveTopology, vkPipelineInputAssemblyStateCreateInfo.topology, vkPipelineTessellationStateCreateInfo.patchControlPoints);

      vkGraphicsPipelineCreateInfo.pTessellationState = &vkPipelineTessellationStateCreateInfo;
    }
  }

  bool bScissorEnabled = false;
  if (xiiSharedPtr<xiiGALRasterizerStateVulkan> pRasterizerStateVulkan = m_Description.m_GraphicsPipeline.m_pRasterizerState.Downcast<xiiGALRasterizerStateVulkan>())
  {
    vkGraphicsPipelineCreateInfo.pRasterizationState = pRasterizerStateVulkan->GetRasterizerState();
    bScissorEnabled                                  = pRasterizerStateVulkan->GetDescription().m_bScissorEnable;
  }

  vk::PipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo = {};
  vk::Rect2D                          vkScissorRect                     = {};
  {
    vkPipelineViewportStateCreateInfo.pNext         = {};
    vkPipelineViewportStateCreateInfo.flags         = {};
    vkPipelineViewportStateCreateInfo.viewportCount = m_Description.m_GraphicsPipeline.m_uiViewportCount; // Even though we use dynamic viewports, the number of viewports used by the pipeline is still specified by the viewportCount member (23.5).
    vkPipelineViewportStateCreateInfo.pViewports    = nullptr;                                            // We will be using dynamic viewport & scissor states.
    vkPipelineViewportStateCreateInfo.scissorCount  = m_Description.m_GraphicsPipeline.m_uiViewportCount; // the number of scissors must match the number of viewports (23.5).

    if (bScissorEnabled)
    {
      vkPipelineViewportStateCreateInfo.pScissors = nullptr; // Ignored if the scissor state is dynamic.
    }
    else
    {
      const vk::PhysicalDeviceProperties& physicalDeviceProperties = pDeviceVulkan->GetVulkanPhysicalDeviceProperties();

      // There are limitations on the viewport width and height (23.5), but it is not clear if there are limitations on the scissor rect width and height.
      vkScissorRect.extent.width                  = physicalDeviceProperties.limits.maxViewportDimensions[0];
      vkScissorRect.extent.height                 = physicalDeviceProperties.limits.maxViewportDimensions[1];
      vkPipelineViewportStateCreateInfo.pScissors = &vkScissorRect;
    }
  }
  vkGraphicsPipelineCreateInfo.pViewportState = &vkPipelineViewportStateCreateInfo;

  vk::PipelineMultisampleStateCreateInfo vkPipelineMultisampleStateCreateInfo = {};
  xiiUInt32                              sampleMask[]                         = {m_Description.m_GraphicsPipeline.m_uiSampleMask, 0U}; // Vulkan spec allows up to 64 samples.
  {
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of pMultisampleState must be the same as the sample count for those subpass attachments.

    vkPipelineMultisampleStateCreateInfo.pNext                 = nullptr;
    vkPipelineMultisampleStateCreateInfo.flags                 = {};
    vkPipelineMultisampleStateCreateInfo.rasterizationSamples  = static_cast<vk::SampleCountFlagBits>(m_Description.m_GraphicsPipeline.m_SampleDescription.m_uiCount);
    vkPipelineMultisampleStateCreateInfo.sampleShadingEnable   = vk::False;
    vkPipelineMultisampleStateCreateInfo.minSampleShading      = 0U;         // A minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
    vkPipelineMultisampleStateCreateInfo.pSampleMask           = sampleMask; // An array of static coverage information that is ANDed with the coverage information generated during rasterization (25.3).
    vkPipelineMultisampleStateCreateInfo.alphaToCoverageEnable = vk::False;  // Whether a temporary coverage value is generated based on the alpha component of the fragment's first color output.
    vkPipelineMultisampleStateCreateInfo.alphaToOneEnable      = vk::False;  // Whether the alpha component of the fragment's first color output is replaced with one.
  }
  vkGraphicsPipelineCreateInfo.pMultisampleState = &vkPipelineMultisampleStateCreateInfo;

  if (xiiSharedPtr<xiiGALDepthStencilStateVulkan> pDepthStencilStateVulkan = m_Description.m_GraphicsPipeline.m_pDepthStencilState.Downcast<xiiGALDepthStencilStateVulkan>())
  {
    vkGraphicsPipelineCreateInfo.pDepthStencilState = pDepthStencilStateVulkan->GetDepthStencilState();
  }

  vk::PipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo = {};
  {
    xiiArrayPtr<const vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    if (xiiSharedPtr<xiiGALBlendStateVulkan> pBlendStateVulkan = m_Description.m_GraphicsPipeline.m_pBlendState.Downcast<xiiGALBlendStateVulkan>())
    {
      vkPipelineColorBlendStateCreateInfo = *pBlendStateVulkan->GetBlendState();
      colorBlendAttachmentStates          = pBlendStateVulkan->GetBlendAttachmentStates();
    }

    xiiSharedPtr<xiiGALRenderPassVulkan> pRenderPassVulkan     = m_Description.m_GraphicsPipeline.m_pRenderPass.Downcast<xiiGALRenderPassVulkan>();
    const auto&                          renderPassDescription = pRenderPassVulkan->GetDescription();

    vkPipelineColorBlendStateCreateInfo.attachmentCount = renderPassDescription.m_SubPasses[m_Description.m_GraphicsPipeline.m_uiSubpassIndex].m_RenderTargetAttachments.GetCount();
    vkPipelineColorBlendStateCreateInfo.pAttachments    = nullptr;

    if (vkPipelineColorBlendStateCreateInfo.attachmentCount > 0)
    {
      XII_ASSERT_DEV(colorBlendAttachmentStates.GetCount() >= vkPipelineColorBlendStateCreateInfo.attachmentCount, "");

      vkPipelineColorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.GetPtr();
    }
  }
  vkGraphicsPipelineCreateInfo.pColorBlendState = &vkPipelineColorBlendStateCreateInfo;

  vk::PipelineDynamicStateCreateInfo   vkPipelineDynamicStateCreateInfo = {};
  xiiStaticArray<vk::DynamicState, 5U> dynamicStates;
  {
    vkPipelineDynamicStateCreateInfo.pNext = nullptr;
    vkPipelineDynamicStateCreateInfo.flags = {};

    // pViewports state in VkPipelineViewportStateCreateInfo will be ignored and must be set dynamically with vkCmdSetViewport before any draw commands.
    // The number of viewports used by a pipeline is still specified by the viewportCount member of VkPipelineViewportStateCreateInfo.
    dynamicStates.PushBack(vk::DynamicState::eViewport);

    // blendConstants state in VkPipelineColorBlendStateCreateInfo will be ignored and must be set dynamically with vkCmdSetBlendConstants.
    dynamicStates.PushBack(vk::DynamicState::eBlendConstants);

    // Specifies that the reference state in VkPipelineDepthStencilStateCreateInfo for both front and back will be ignored and must be set dynamically with vkCmdSetStencilReference.
    dynamicStates.PushBack(vk::DynamicState::eStencilReference);

    if (bScissorEnabled)
    {
      // pScissors state in VkPipelineViewportStateCreateInfo will be ignored and must be set dynamically with vkCmdSetScissor before any draw commands.
      // The number of scissor rectangles used by a pipeline is still specified by the scissorCount member of VkPipelineViewportStateCreateInfo.
      dynamicStates.PushBack(vk::DynamicState::eScissor);
    }

    if (m_Description.m_GraphicsPipeline.m_ShadingRateFlags.IsAnyFlagSet() && pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures().m_ShadingRate.attachmentFragmentShadingRate != vk::False)
    {
      // VkPipelineFragmentShadingRateStateCreateInfoKHR will be ignored and must be set dynamically with vkCmdSetFragmentShadingRateKHR before any drawing commands.
      dynamicStates.PushBack(vk::DynamicState::eFragmentShadingRateKHR);
    }

    vkPipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStates.GetCount();
    vkPipelineDynamicStateCreateInfo.pDynamicStates    = dynamicStates.GetData();
  }
  vkGraphicsPipelineCreateInfo.pDynamicState = &vkPipelineDynamicStateCreateInfo;

  vk::PipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
  {
    auto pDescriptorSetLayouts = pPipelineResourceSignatureVulkan->GetVulkanDescriptorSetLayouts();

    vkPipelineLayoutCreateInfo.pNext                  = nullptr;
    vkPipelineLayoutCreateInfo.flags                  = {};
    vkPipelineLayoutCreateInfo.setLayoutCount         = pDescriptorSetLayouts.GetCount();
    vkPipelineLayoutCreateInfo.pSetLayouts            = pDescriptorSetLayouts.GetPtr();
    vkPipelineLayoutCreateInfo.pushConstantRangeCount = 0;       // TODO.
    vkPipelineLayoutCreateInfo.pPushConstantRanges    = nullptr; // TODO.

    VK_ASSERT_DEV(vkLogicalDevice.createPipelineLayout(&vkPipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  vkGraphicsPipelineCreateInfo.layout = m_vkPipelineLayout;

  if (xiiSharedPtr<xiiGALRenderPassVulkan> pRenderPassVulkan = m_Description.m_GraphicsPipeline.m_pRenderPass.Downcast<xiiGALRenderPassVulkan>())
  {
    vkGraphicsPipelineCreateInfo.renderPass = pRenderPassVulkan->GetVulkanRenderPass();
    vkGraphicsPipelineCreateInfo.subpass    = m_Description.m_GraphicsPipeline.m_uiSubpassIndex;
  }

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createGraphicsPipelines(m_vkPipelineCache, 1U, &vkGraphicsPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

void xiiGALGraphicsPipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_GraphicsPipelineStateVulkan);
