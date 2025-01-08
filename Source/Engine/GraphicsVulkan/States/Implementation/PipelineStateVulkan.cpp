#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineStateVulkan::xiiGALPipelineStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceVulkan, creationDescription)
{
}

xiiGALPipelineStateVulkan::~xiiGALPipelineStateVulkan() = default;

xiiResult xiiGALPipelineStateVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*                    pDeviceVulkan                    = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device                             vkLogicalDevice                  = pDeviceVulkan->GetVulkanLogicalDevice();
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<xiiGALPipelineResourceSignatureVulkan*>(pDeviceVulkan->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      const auto& graphicsPipeline = m_Description.m_GraphicsPipeline;

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
#define DEFINE_VULKAN_SHADER_IF_EXISTS(shaderType, shaderStageFlagBits)                                                                 \
  if (xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pDeviceVulkan->GetShader(graphicsPipeline.m_h##shaderType))) \
  {                                                                                                                                     \
    vk::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};                                                             \
    vkPipelineShaderStageCreateInfo.pNext                             = nullptr;                                                        \
    vkPipelineShaderStageCreateInfo.flags                             = {};                                                             \
    vkPipelineShaderStageCreateInfo.stage                             = shaderStageFlagBits;                                            \
    vkPipelineShaderStageCreateInfo.pName                             = "main";                                                         \
    vkPipelineShaderStageCreateInfo.module                            = pShaderVulkan->GetVulkanShaderModule();                         \
    vkPipelineShaderStageCreateInfo.pSpecializationInfo               = nullptr;                                                        \
                                                                                                                                        \
    vkShaderStages.PushBack(vkPipelineShaderStageCreateInfo);                                                                           \
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
      if (xiiGALInputLayoutVulkan* pInputLayoutVulkan = static_cast<xiiGALInputLayoutVulkan*>(pDeviceVulkan->GetInputLayout(graphicsPipeline.m_hInputLayout)))
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
        vkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = (graphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStrip || graphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStripAdjacent || graphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStrip || graphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStripAdjacent) ? vk::True : vk::False;
      }
      vkGraphicsPipelineCreateInfo.pInputAssemblyState = &vkPipelineInputAssemblyStateCreateInfo;

      vk::PipelineTessellationStateCreateInfo vkPipelineTessellationStateCreateInfo = {};
      {
        vkPipelineTessellationStateCreateInfo.pNext = nullptr;
        vkPipelineTessellationStateCreateInfo.flags = {};

        if (m_Description.m_PipelineType == xiiGALPipelineType::Mesh)
        {
          // Input assembly is not used in the mesh pipeline, so topology may contain any value.
          // Validation layers may generate a warning if point_list topology is used, so use MAX_ENUM value.
          vkPipelineInputAssemblyStateCreateInfo.topology = (vk::PrimitiveTopology)VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

          // Vertex input state and tessellation state are ignored in a mesh pipeline and should be null, but there is a bug in validation layers that makes them crash.
          // vkGraphicsPipelineCreateInfo.pVertexInputState = nullptr;
          vkGraphicsPipelineCreateInfo.pTessellationState = nullptr;
        }
        else
        {
          xiiVulkanTypeConversions::GetPrimitiveTopologyAndControlPatchPointsCount(graphicsPipeline.m_PrimitiveTopology, vkPipelineInputAssemblyStateCreateInfo.topology, vkPipelineTessellationStateCreateInfo.patchControlPoints);

          vkGraphicsPipelineCreateInfo.pTessellationState = &vkPipelineTessellationStateCreateInfo;
        }
      }

      bool bScissorEnabled = false;
      if (xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = static_cast<xiiGALRasterizerStateVulkan*>(pDeviceVulkan->GetRasterizerState(graphicsPipeline.m_hRasterizerState)))
      {
        vkGraphicsPipelineCreateInfo.pRasterizationState = pRasterizerStateVulkan->GetRasterizerState();
        bScissorEnabled                                  = pRasterizerStateVulkan->GetDescription().m_bScissorEnable;
      }

      vk::PipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo = {};
      vk::Rect2D                          vkScissorRect                     = {};
      {
        vkPipelineViewportStateCreateInfo.pNext         = {};
        vkPipelineViewportStateCreateInfo.flags         = {};
        vkPipelineViewportStateCreateInfo.viewportCount = graphicsPipeline.m_uiViewportCount; // Even though we use dynamic viewports, the number of viewports used by the pipeline is still specified by the viewportCount member (23.5)
        vkPipelineViewportStateCreateInfo.pViewports    = nullptr;                            // We will be using dynamic viewport & scissor states.
        vkPipelineViewportStateCreateInfo.scissorCount  = graphicsPipeline.m_uiViewportCount; // the number of scissors must match the number of viewports (23.5)

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
      xiiUInt32                              sampleMask[]                         = {graphicsPipeline.m_uiSampleMask, 0U}; // Vulkan spec allows up to 64 samples.
      {
        // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of pMultisampleState must be the same as the sample count for those subpass attachments.

        vkPipelineMultisampleStateCreateInfo.pNext                 = nullptr;
        vkPipelineMultisampleStateCreateInfo.flags                 = {};
        vkPipelineMultisampleStateCreateInfo.rasterizationSamples  = static_cast<vk::SampleCountFlagBits>(graphicsPipeline.m_SampleDescription.m_uiCount);
        vkPipelineMultisampleStateCreateInfo.sampleShadingEnable   = vk::False;
        vkPipelineMultisampleStateCreateInfo.minSampleShading      = 0U;         // A minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
        vkPipelineMultisampleStateCreateInfo.pSampleMask           = sampleMask; // An array of static coverage information that is ANDed with the coverage information generated during rasterization (25.3)
        vkPipelineMultisampleStateCreateInfo.alphaToCoverageEnable = vk::False;  // Whether a temporary coverage value is generated based on the alpha component of the fragment's first color output.
        vkPipelineMultisampleStateCreateInfo.alphaToOneEnable      = vk::False;  // Whether the alpha component of the fragment's first color output is replaced with one.
      }
      vkGraphicsPipelineCreateInfo.pMultisampleState = &vkPipelineMultisampleStateCreateInfo;

      if (xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = static_cast<xiiGALDepthStencilStateVulkan*>(pDeviceVulkan->GetDepthStencilState(graphicsPipeline.m_hDepthStencilState)))
      {
        vkGraphicsPipelineCreateInfo.pDepthStencilState = pDepthStencilStateVulkan->GetDepthStencilState();
      }

      vk::PipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo = {};
      {
        xiiArrayPtr<const vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
        if (xiiGALBlendStateVulkan* pBlendStateVulkan = static_cast<xiiGALBlendStateVulkan*>(pDeviceVulkan->GetBlendState(graphicsPipeline.m_hBlendState)))
        {
          vkPipelineColorBlendStateCreateInfo = *pBlendStateVulkan->GetBlendState();
          colorBlendAttachmentStates          = pBlendStateVulkan->GetBlendAttachmentStates();
        }

        xiiGALRenderPassVulkan* pRenderPassVulkan     = static_cast<xiiGALRenderPassVulkan*>(pDeviceVulkan->GetRenderPass(graphicsPipeline.m_hRenderPass));
        const auto&             renderPassDescription = pRenderPassVulkan->GetDescription();

        vkPipelineColorBlendStateCreateInfo.attachmentCount = renderPassDescription.m_SubPasses[graphicsPipeline.m_uiSubpassIndex].m_RenderTargetAttachments.GetCount();
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

        if (graphicsPipeline.m_ShadingRateFlags.IsAnyFlagSet() && pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures().m_ShadingRate.attachmentFragmentShadingRate != vk::False)
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

      if (xiiGALRenderPassVulkan* pRenderPassVulkan = static_cast<xiiGALRenderPassVulkan*>(pDeviceVulkan->GetRenderPass(graphicsPipeline.m_hRenderPass)))
      {
        vkGraphicsPipelineCreateInfo.renderPass = pRenderPassVulkan->GetVulkanRenderPass();
        vkGraphicsPipelineCreateInfo.subpass    = graphicsPipeline.m_uiSubpassIndex;
      }

      XII_ASSERT_DEV(vkGraphicsPipelineCreateInfo.renderPass != VK_NULL_HANDLE, "");

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createGraphicsPipelines(m_vkPipelineCache, 1U, &vkGraphicsPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      const auto& computePipeline = m_Description.m_ComputePipeline;

      vk::ComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
      vkComputePipelineCreateInfo.pNext                         = nullptr;
      vkComputePipelineCreateInfo.flags                         = {};
      vkComputePipelineCreateInfo.basePipelineHandle            = nullptr; // A pipeline to derive from.
      vkComputePipelineCreateInfo.basePipelineIndex             = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      vkComputePipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

      xiiHybridArray<vk::PipelineShaderStageCreateInfo, 1U> vkShaderStages(pDeviceVulkan->GetAllocator());
      {
#define DEFINE_VULKAN_SHADER_IF_EXISTS(shaderType, shaderStageFlagBits)                                                                \
  if (xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pDeviceVulkan->GetShader(computePipeline.m_h##shaderType))) \
  {                                                                                                                                    \
    vk::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};                                                            \
    vkPipelineShaderStageCreateInfo.pNext                             = nullptr;                                                       \
    vkPipelineShaderStageCreateInfo.flags                             = {};                                                            \
    vkPipelineShaderStageCreateInfo.stage                             = shaderStageFlagBits;                                           \
    vkPipelineShaderStageCreateInfo.pName                             = "main";                                                        \
    vkPipelineShaderStageCreateInfo.module                            = pShaderVulkan->GetVulkanShaderModule();                        \
    vkPipelineShaderStageCreateInfo.pSpecializationInfo               = nullptr;                                                       \
                                                                                                                                       \
    vkShaderStages.PushBack(vkPipelineShaderStageCreateInfo);                                                                          \
  }

        DEFINE_VULKAN_SHADER_IF_EXISTS(ComputeShader, vk::ShaderStageFlagBits::eCompute);

#undef DEFINE_VULKAN_SHADER_IF_EXISTS

        XII_ASSERT_DEV(!vkShaderStages.IsEmpty(), "");
      }
      vkComputePipelineCreateInfo.stage = vkShaderStages.PeekBack();

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
      vkComputePipelineCreateInfo.layout = m_vkPipelineLayout;

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createComputePipelines(m_vkPipelineCache, 1U, &vkComputePipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      XII_ASSERT_NOT_IMPLEMENTED;

      vk::RayTracingPipelineCreateInfoKHR vkRayTracingPipelineCreateInfo = {};
      vkRayTracingPipelineCreateInfo.pNext                               = nullptr;
      vkRayTracingPipelineCreateInfo.flags                               = {};
      vkRayTracingPipelineCreateInfo.basePipelineHandle                  = nullptr; // A pipeline to derive from.
      vkRayTracingPipelineCreateInfo.basePipelineIndex                   = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      vkRayTracingPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createRayTracingPipelinesKHR(VK_NULL_HANDLE, m_vkPipelineCache, 1U, &vkRayTracingPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      return XII_FAILURE;
    }
    break;

    default:
      xiiLog::Error("Unknown pipeline type.");
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkPipelineCache != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkPipelineCache);

    m_vkPipelineCache = nullptr;
  }
  if (m_vkPipeline != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkPipeline);

    m_vkPipeline = VK_NULL_HANDLE;
  }
  if (m_vkPipelineLayout != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkPipelineLayout);

    m_vkPipelineLayout = VK_NULL_HANDLE;
  }
  return XII_SUCCESS;
}

void xiiGALPipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

void xiiGALPipelineStateVulkan::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
}

void xiiGALPipelineStateVulkan::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateVulkan::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateVulkan::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateVulkan::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateVulkan::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
}

void xiiGALPipelineStateVulkan::ResetBoundResources()
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineStateVulkan);
