#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Basics.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

xiiGALDeviceVulkan* xiiResourceCacheVulkan::s_pDevice;
vk::Device          xiiResourceCacheVulkan::s_device;

xiiHashTable<xiiGALRenderingSetup, vk::RenderPass, xiiResourceCacheVulkan::ResourceCacheHash>                                             xiiResourceCacheVulkan::s_shallowRenderPasses;
xiiHashTable<xiiResourceCacheVulkan::RenderPassDesc, vk::RenderPass, xiiResourceCacheVulkan::ResourceCacheHash>                           xiiResourceCacheVulkan::s_renderPasses;
xiiHashTable<xiiResourceCacheVulkan::FramebufferKey, xiiResourceCacheVulkan::FrameBufferCache, xiiResourceCacheVulkan::ResourceCacheHash> xiiResourceCacheVulkan::s_frameBuffers;
xiiHashTable<xiiResourceCacheVulkan::PipelineLayoutDesc, vk::PipelineLayout, xiiResourceCacheVulkan::ResourceCacheHash>                   xiiResourceCacheVulkan::s_pipelineLayouts;
xiiResourceCacheVulkan::GraphicsPipelineMap                                                                                               xiiResourceCacheVulkan::s_graphicsPipelines;
xiiResourceCacheVulkan::ComputePipelineMap                                                                                                xiiResourceCacheVulkan::s_computePipelines;
xiiMap<const xiiRefCounted*, xiiHybridArray<xiiResourceCacheVulkan::GraphicsPipelineMap::Iterator, 1>>                                    xiiResourceCacheVulkan::s_graphicsPipelineUsedBy;
xiiMap<const xiiRefCounted*, xiiHybridArray<xiiResourceCacheVulkan::ComputePipelineMap::Iterator, 1>>                                     xiiResourceCacheVulkan::s_computePipelineUsedBy;

xiiHashTable<xiiGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, xiiResourceCacheVulkan::ResourceCacheHash> xiiResourceCacheVulkan::s_descriptorSetLayouts;

#define XII_LOG_VULKAN_RESOURCES

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALRenderTargetViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const xiiUInt32&>(Value);
    return Stream;
  }
} // namespace

void xiiResourceCacheVulkan::Initialize(xiiGALDeviceVulkan* pDevice, vk::Device device)
{
  s_pDevice = pDevice;
  s_device  = device;
}

void xiiResourceCacheVulkan::DeInitialize()
{
  for (auto it : s_renderPasses)
  {
    s_device.destroyRenderPass(it.Value(), nullptr);
  }
  s_renderPasses.Clear();
  s_renderPasses.Compact();
  s_shallowRenderPasses.Clear();
  s_shallowRenderPasses.Compact();

  for (auto it : s_frameBuffers)
  {
    s_device.destroyFramebuffer(it.Value().m_frameBuffer, nullptr);
  }
  s_frameBuffers.Clear();
  s_frameBuffers.Compact();

  for (auto it : s_graphicsPipelines)
  {
    s_device.destroyPipeline(it.Value(), nullptr);
  }
  s_graphicsPipelines.Clear();
  GraphicsPipelineMap tmp;
  s_graphicsPipelines.Swap(tmp);
  s_graphicsPipelineUsedBy.Clear();
  xiiMap<const xiiRefCounted*, xiiHybridArray<GraphicsPipelineMap::Iterator, 1>> tmp2;
  s_graphicsPipelineUsedBy.Swap(tmp2);


  for (auto it : s_pipelineLayouts)
  {
    s_device.destroyPipelineLayout(it.Value(), nullptr);
  }
  s_pipelineLayouts.Clear();
  s_pipelineLayouts.Compact();

  for (auto it : s_descriptorSetLayouts)
  {
    s_device.destroyDescriptorSetLayout(it.Value(), nullptr);
  }
  s_descriptorSetLayouts.Clear();
  s_descriptorSetLayouts.Compact();

  s_device = nullptr;
}

void xiiResourceCacheVulkan::GetRenderPassDesc(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc)
{
  const bool      bHasDepth    = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  out_desc.attachments.Clear();

  if (bHasDepth)
  {
    const xiiGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const xiiGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    auto  hTexture = pRenderTargetView->GetDescription().m_hTexture;
    auto* pTex     = static_cast<const xiiGALTextureVulkan*>(s_pDevice->GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& texDesc    = pTex->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format     = texDesc.m_Format;
    const auto&                             formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& depthAttachment = out_desc.attachments.ExpandAndGetRef();
    depthAttachment.format          = formatInfo.m_eRenderTarget;
    if (pTex->GetFormatOverrideEnabled())
    {
      depthAttachment.format = pTex->GetImageFormat();
    }

    depthAttachment.samples = xiiConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    if (renderingSetup.m_bDiscardDepth)
    {
      depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
      depthAttachment.loadOp        = vk::AttachmentLoadOp::eDontCare;
    }
    else
    {
      depthAttachment.initialLayout = renderingSetup.m_bClearDepth ? vk::ImageLayout::eUndefined : vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthAttachment.loadOp        = renderingSetup.m_bClearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    }
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    if (format == xiiGALResourceFormat::Enum::D24S8)
    {
      depthAttachment.stencilLoadOp  = renderingSetup.m_bClearStencil ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eStore;
    }
    else
    {
      depthAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    }
  }

  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto                                colorRT           = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));
    const xiiGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const xiiGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));

    auto  hTexture = pRenderTargetView->GetDescription().m_hTexture;
    auto* pTex     = static_cast<const xiiGALTextureVulkan*>(s_pDevice->GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& texDesc    = pTex->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format     = texDesc.m_Format;
    const auto&                             formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& colorAttachment = out_desc.attachments.ExpandAndGetRef();
    colorAttachment.format          = formatInfo.m_eRenderTarget;
    if (pTex->GetFormatOverrideEnabled())
    {
      colorAttachment.format = pTex->GetImageFormat();
    }

    colorAttachment.samples = xiiConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    if (renderingSetup.m_bDiscardColor)
    {
      colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
      colorAttachment.loadOp        = vk::AttachmentLoadOp::eDontCare;
    }
    else
    {
      if (renderingSetup.m_uiRenderTargetClearMask & (1u << i))
      {
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.loadOp        = vk::AttachmentLoadOp::eClear;
      }
      else
      {
        colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachment.loadOp        = vk::AttachmentLoadOp::eLoad;
      }
    }
    colorAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
  }
}

XII_DEFINE_AS_POD_TYPE(vk::AttachmentDescription);
XII_DEFINE_AS_POD_TYPE(vk::AttachmentReference);

vk::RenderPass xiiResourceCacheVulkan::RequestRenderPassInternal(const RenderPassDesc& desc)
{
  if (const vk::RenderPass* pPass = s_renderPasses.GetValue(desc))
  {
    return *pPass;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating RenderPass #{}", s_renderPasses.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  xiiHybridArray<vk::AttachmentDescription, 4> attachments;
  xiiHybridArray<vk::AttachmentReference, 1>   depthAttachmentRefs;
  xiiHybridArray<vk::AttachmentReference, 4>   colorAttachmentRefs;

  const xiiUInt32 uiCount = desc.attachments.GetCount();
  for (xiiUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc&      attachmentDesc = desc.attachments[i];
    vk::AttachmentDescription& vkAttachment   = attachments.ExpandAndGetRef();
    vkAttachment.format                       = attachmentDesc.format;
    vkAttachment.samples                      = attachmentDesc.samples;
    vkAttachment.loadOp                       = attachmentDesc.loadOp;
    vkAttachment.storeOp                      = attachmentDesc.storeOp;
    vkAttachment.stencilLoadOp                = attachmentDesc.stencilLoadOp;
    vkAttachment.stencilStoreOp               = attachmentDesc.stencilStoreOp;
    vkAttachment.initialLayout                = attachmentDesc.initialLayout;

    const bool bIsDepth = xiiConversionUtilsVulkan::IsDepthFormat(attachmentDesc.format);
    if (bIsDepth)
    {
      vkAttachment.finalLayout                 = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      vk::AttachmentReference& depthAttachment = depthAttachmentRefs.ExpandAndGetRef();
      depthAttachment.attachment               = i;
      depthAttachment.layout                   = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      vkAttachment.finalLayout                 = vk::ImageLayout::eColorAttachmentOptimal;
      vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
      colorAttachment.attachment               = i;
      colorAttachment.layout                   = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  XII_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1, "There can be no more than 1 depth attachment.");
  const bool             bHasColor = !colorAttachmentRefs.IsEmpty();
  const bool             bHasDepth = !depthAttachmentRefs.IsEmpty();
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount    = colorAttachmentRefs.GetCount();
  subpass.pColorAttachments       = bHasColor ? colorAttachmentRefs.GetData() : nullptr;
  subpass.pDepthStencilAttachment = bHasDepth ? depthAttachmentRefs.GetData() : nullptr;

  vk::SubpassDependency dependency;
  dependency.dstSubpass = 0;
  if (bHasColor)
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

  if (bHasDepth)
    dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  dependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.srcAccessMask = {};
  dependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = attachments.GetCount();
  renderPassCreateInfo.pAttachments    = attachments.GetData();
  renderPassCreateInfo.subpassCount    = 1;
  renderPassCreateInfo.pSubpasses      = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies   = &dependency;

  vk::RenderPass renderPass;
  VK_LOG_ERROR(s_device.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass));

  s_renderPasses.Insert(desc, renderPass);
  return renderPass;
}

vk::RenderPass xiiResourceCacheVulkan::RequestRenderPass(const xiiGALRenderingSetup& renderingSetup)
{
  if (const vk::RenderPass* pPass = s_shallowRenderPasses.GetValue(renderingSetup))
  {
    return *pPass;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Redirecting shallow RenderPass #{}", s_shallowRenderPasses.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  RenderPassDesc renderPassDesc;
  GetRenderPassDesc(renderingSetup, renderPassDesc);
  vk::RenderPass renderPass = RequestRenderPassInternal(renderPassDesc);

  s_shallowRenderPasses.Insert(renderingSetup, renderPass);
  return renderPass;
}

void xiiResourceCacheVulkan::GetFrameBufferDesc(vk::RenderPass renderPass, const xiiGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc)
{
  const bool      bHasDepth    = !renderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorCount = renderTargetSetup.GetRenderTargetCount();

  out_desc.renderPass = renderPass;
  if (bHasDepth)
  {
    const xiiGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const xiiGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderTargetSetup.GetDepthStencilTarget()));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    const xiiGALTextureVulkan*              pTex    = static_cast<const xiiGALTextureVulkan*>(pRenderTargetView->GetTexture()->GetParentResource());
    const xiiGALTextureCreationDescription& texDesc = pTex->GetDescription();
    vk::Extent3D                            extend  = pTex->GetMipLevelSize(pRenderTargetView->GetDescription().m_uiMipLevel);
    out_desc.m_msaa                                 = texDesc.m_SampleCount;
    out_desc.m_size                                 = {extend.width, extend.height};
    out_desc.layers                                 = texDesc.m_uiArraySize;
  }
  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto                                colorRT           = renderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));
    const xiiGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const xiiGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    const xiiGALTextureVulkan*              pTex    = static_cast<const xiiGALTextureVulkan*>(pRenderTargetView->GetTexture()->GetParentResource());
    const xiiGALTextureCreationDescription& texDesc = pTex->GetDescription();
    vk::Extent3D                            extend  = pTex->GetMipLevelSize(pRenderTargetView->GetDescription().m_uiMipLevel);
    out_desc.m_msaa                                 = texDesc.m_SampleCount;
    out_desc.m_size                                 = {extend.width, extend.height};
    out_desc.layers                                 = texDesc.m_uiArraySize;
  }

  // In some places rendering is started with an empty xiiGALRenderTargetSetup just to be able to run GPU commands.
  // An empty size is invalid in Vulkan so we just set it so 1,1.
  if (out_desc.m_size == xiiSizeU32(0, 0))
  {
    out_desc.m_size = {1, 1};
    out_desc.layers = 1;
  }
}

vk::Framebuffer xiiResourceCacheVulkan::RequestFrameBuffer(vk::RenderPass renderPass, const xiiGALRenderTargetSetup& renderTargetSetup, xiiSizeU32& out_Size, xiiEnum<xiiGALMSAASampleCount>& out_msaa, xiiUInt32& out_uiLayers)
{
  FramebufferKey key;
  key.m_renderPass        = renderPass;
  key.m_renderTargetSetup = renderTargetSetup;

  if (const FrameBufferCache* pFrameBuffer = s_frameBuffers.GetValue(key))
  {
    out_Size = pFrameBuffer->m_size;
    out_msaa = pFrameBuffer->m_msaa;
    return pFrameBuffer->m_frameBuffer;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating FrameBuffer #{}", s_frameBuffers.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  FramebufferDesc desc;
  GetFrameBufferDesc(renderPass, renderTargetSetup, desc);

  vk::FramebufferCreateInfo framebufferInfo;
  framebufferInfo.renderPass      = desc.renderPass;
  framebufferInfo.attachmentCount = desc.attachments.GetCount();
  framebufferInfo.pAttachments    = desc.attachments.GetData();
  framebufferInfo.width           = desc.m_size.width;
  framebufferInfo.height          = desc.m_size.height;
  framebufferInfo.layers          = desc.layers;

  FrameBufferCache cache;
  cache.m_size   = desc.m_size;
  cache.m_msaa   = desc.m_msaa;
  cache.m_layers = desc.layers;
  VK_LOG_ERROR(s_device.createFramebuffer(&framebufferInfo, nullptr, &cache.m_frameBuffer));

  s_frameBuffers.Insert(key, cache);
  out_Size     = cache.m_size;
  out_msaa     = cache.m_msaa;
  out_uiLayers = cache.m_layers;
  return cache.m_frameBuffer;
}

vk::PipelineLayout xiiResourceCacheVulkan::RequestPipelineLayout(const PipelineLayoutDesc& desc)
{
  if (const vk::PipelineLayout* pPipelineLayout = s_pipelineLayouts.GetValue(desc))
  {
    return *pPipelineLayout;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating Pipeline Layout #{}", s_pipelineLayouts.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts    = &desc.m_layout;

  vk::PipelineLayout layout;
  VK_ASSERT_DEBUG(s_device.createPipelineLayout(&layoutInfo, nullptr, &layout));

  s_pipelineLayouts.Insert(desc, layout);
  return layout;
}

vk::Pipeline xiiResourceCacheVulkan::RequestGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_graphicsPipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating Graphics Pipeline #{}", s_graphicsPipelines.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  vk::PipelineVertexInputStateCreateInfo                                             vertex_input;
  xiiHybridArray<vk::VertexInputBindingDescription, XII_GAL_MAX_VERTEX_BUFFER_COUNT> bindings;
  if (desc.m_pCurrentVertexDecl)
  {
    bindings = desc.m_pCurrentVertexDecl->GetBindings();
    for (xiiUInt32 i = 0; i < bindings.GetCount(); i++)
    {
      bindings[i].stride = desc.m_VertexBufferStrides[bindings[i].binding];
    }
    vertex_input.vertexAttributeDescriptionCount = desc.m_pCurrentVertexDecl->GetAttributes().GetCount();
    vertex_input.pVertexAttributeDescriptions    = desc.m_pCurrentVertexDecl->GetAttributes().GetPtr();
    vertex_input.vertexBindingDescriptionCount   = bindings.GetCount();
    vertex_input.pVertexBindingDescriptions      = bindings.GetData();
  }

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.topology = xiiConversionUtilsVulkan::GetPrimitiveTopology(desc.m_topology);

  // Specify rasterization state.
  const vk::PipelineRasterizationStateCreateInfo* raster = desc.m_pCurrentRasterizerState->GetRasterizerState();

  // Our attachment will write to all color channels, but no blending is enabled.
  vk::PipelineColorBlendStateCreateInfo blend = *desc.m_pCurrentBlendState->GetBlendState();
  blend.attachmentCount                       = desc.m_uiAttachmentCount;

  // We will have one viewport and scissor box.
  vk::PipelineViewportStateCreateInfo viewport;
  viewport.viewportCount = 1;
  viewport.scissorCount  = 1;

  // Disable all depth testing.
  const vk::PipelineDepthStencilStateCreateInfo* depth_stencil = desc.m_pCurrentDepthStencilState->GetDepthStencilState();

  // No multisampling.
  vk::PipelineMultisampleStateCreateInfo multisample;
  multisample.rasterizationSamples = xiiConversionUtilsVulkan::GetSamples(desc.m_msaa);
  if (multisample.rasterizationSamples != vk::SampleCountFlagBits::e1 && desc.m_pCurrentBlendState->GetDescription().m_bAlphaToCoverage)
  {
    multisample.alphaToCoverageEnable = true;
  }

  // Specify that these states will be dynamic, i.e. not part of pipeline state object.
  xiiHybridArray<vk::DynamicState, 2> dynamics;
  dynamics.PushBack(vk::DynamicState::eViewport);
  dynamics.PushBack(vk::DynamicState::eScissor);

  vk::PipelineDynamicStateCreateInfo dynamic;
  dynamic.pDynamicStates    = dynamics.GetData();
  dynamic.dynamicStateCount = dynamics.GetCount();

  // Load our SPIR-V shaders.
  xiiHybridArray<vk::PipelineShaderStageCreateInfo, 6> shader_stages;
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    if (vk::ShaderModule shader = desc.m_pCurrentShader->GetShader((xiiGALShaderStage::Enum)i))
    {
      vk::PipelineShaderStageCreateInfo& stage = shader_stages.ExpandAndGetRef();
      stage.stage                              = xiiConversionUtilsVulkan::GetShaderStage((xiiGALShaderStage::Enum)i);
      stage.module                             = shader;
      stage.pName                              = "main";
    }
  }

  vk::GraphicsPipelineCreateInfo pipe;
  pipe.renderPass          = desc.m_renderPass;
  pipe.layout              = desc.m_layout;
  pipe.stageCount          = shader_stages.GetCount();
  pipe.pStages             = shader_stages.GetData();
  pipe.pVertexInputState   = &vertex_input;
  pipe.pInputAssemblyState = &input_assembly;
  pipe.pRasterizationState = raster;
  pipe.pColorBlendState    = &blend;
  pipe.pMultisampleState   = &multisample;
  pipe.pViewportState      = &viewport;
  pipe.pDepthStencilState  = depth_stencil;
  pipe.pDynamicState       = &dynamic;

  vk::Pipeline      pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createGraphicsPipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_graphicsPipelines.Insert(desc, pipeline);
  {
    s_graphicsPipelineUsedBy[desc.m_pCurrentRasterizerState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentBlendState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentDepthStencilState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentVertexDecl].PushBack(it);
  }

  return pipeline;
}

vk::Pipeline xiiResourceCacheVulkan::RequestComputePipeline(const ComputePipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_computePipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating Compute Pipeline #{}", s_computePipelines.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  vk::ComputePipelineCreateInfo pipe;
  pipe.layout = desc.m_layout;
  {
    vk::ShaderModule shader = desc.m_pCurrentShader->GetShader(xiiGALShaderStage::ComputeShader);
    pipe.stage.stage        = xiiConversionUtilsVulkan::GetShaderStage(xiiGALShaderStage::ComputeShader);
    pipe.stage.module       = shader;
    pipe.stage.pName        = "main";
  }

  vk::Pipeline      pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createComputePipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_computePipelines.Insert(desc, pipeline);
  {
    s_computePipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
  }

  return pipeline;
}

vk::DescriptorSetLayout xiiResourceCacheVulkan::RequestDescriptorSetLayout(const xiiGALShaderVulkan::DescriptorSetLayoutDesc& desc)
{
  if (const vk::DescriptorSetLayout* pLayout = s_descriptorSetLayouts.GetValue(desc))
  {
    return *pLayout;
  }

#ifdef XII_LOG_VULKAN_RESOURCES
  xiiLog::Info("Creating Descriptor Set Layout #{}", s_descriptorSetLayouts.GetCount());
#endif // XII_LOG_VULKAN_RESOURCES

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayout;
  descriptorSetLayout.bindingCount = desc.m_bindings.GetCount();
  descriptorSetLayout.pBindings    = desc.m_bindings.GetData();

  vk::DescriptorSetLayout layout;
  VK_ASSERT_DEBUG(s_device.createDescriptorSetLayout(&descriptorSetLayout, nullptr, &layout));

  s_descriptorSetLayouts.Insert(desc, layout);
  return layout;
}

void xiiResourceCacheVulkan::ResourceDeleted(const xiiRefCounted* pResource)
{
  auto it = s_graphicsPipelineUsedBy.Find(pResource);
  if (it.IsValid())
  {
    const auto& itArray = it.Value();
    for (GraphicsPipelineMap::Iterator it2 : itArray)
    {
      s_pDevice->DeleteLater(it2.Value());

      const GraphicsPipelineDesc&       desc = it2.Key();
      xiiArrayPtr<const xiiRefCounted*> resources((const xiiRefCounted**)&desc.m_pCurrentRasterizerState, 5);
      for (const xiiRefCounted* pResource2 : resources)
      {
        if (pResource2 != pResource)
        {
          s_graphicsPipelineUsedBy[pResource2].RemoveAndSwap(it2);
        }
      }

      s_graphicsPipelines.Remove(it2);
    }

    s_graphicsPipelineUsedBy.Remove(it);
  }
}

void xiiResourceCacheVulkan::ShaderDeleted(const xiiGALShaderVulkan* pShader)
{
  if (pShader->GetDescription().HasByteCodeForStage(xiiGALShaderStage::ComputeShader))
  {
    auto it = s_computePipelineUsedBy.Find(pShader);
    if (it.IsValid())
    {
      const auto& itArray = it.Value();
      for (ComputePipelineMap::Iterator it2 : itArray)
      {
        s_pDevice->DeleteLater(it2.Value());
        s_computePipelines.Remove(it2);
      }

      s_computePipelineUsedBy.Remove(it);
    }
  }
  else
  {
    ResourceDeleted(pShader);
  }
}

xiiUInt32 xiiResourceCacheVulkan::ResourceCacheHash::Hash(const RenderPassDesc& renderingSetup)
{
  xiiHashStreamWriter32 writer;
  for (const auto& attachment : renderingSetup.attachments)
  {
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.format);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.samples);
    writer << xiiConversionUtilsVulkan::GetUnderlyingFlagsValue(attachment.usage);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.initialLayout);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.loadOp);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.storeOp);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.stencilLoadOp);
    writer << xiiConversionUtilsVulkan::GetUnderlyingValue(attachment.stencilStoreOp);
  }
  return writer.GetHashValue();
}


xiiUInt32 xiiResourceCacheVulkan::ResourceCacheHash::Hash(const xiiGALRenderingSetup& renderingSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  const xiiUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }
  writer << renderingSetup.m_ClearColor;
  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_fDepthClear;
  writer << renderingSetup.m_uiStencilClear;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;
  return writer.GetHashValue();
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const RenderPassDesc& a, const RenderPassDesc& b)
{
  const xiiUInt32 uiCount = a.attachments.GetCount();
  if (uiCount != b.attachments.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc& aE = a.attachments[i];
    const AttachmentDesc& bE = b.attachments[i];

    if (aE.format != bE.format || aE.samples != bE.samples || aE.usage != bE.usage || aE.initialLayout != bE.initialLayout || aE.loadOp != bE.loadOp || aE.storeOp != bE.storeOp || aE.stencilLoadOp != bE.stencilLoadOp || aE.stencilStoreOp != bE.stencilStoreOp)
      return false;
  }

  return true;
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b)
{
  return a == b;
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const xiiGALShaderVulkan::DescriptorSetLayoutDesc& a, const xiiGALShaderVulkan::DescriptorSetLayoutDesc& b)
{
  const xiiUInt32 uiCount = a.m_bindings.GetCount();
  if (uiCount != b.m_bindings.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < uiCount; i++)
  {
    const vk::DescriptorSetLayoutBinding& aB = a.m_bindings[i];
    const vk::DescriptorSetLayoutBinding& bB = b.m_bindings[i];
    if (aB.binding != bB.binding || aB.descriptorType != bB.descriptorType || aB.descriptorCount != bB.descriptorCount || aB.stageFlags != bB.stageFlags || aB.pImmutableSamplers != bB.pImmutableSamplers)
      return false;
  }
  return true;
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
#define LESS_CHECK(member)  \
  if (a.member != b.member) \
    return a.member < b.member;

  LESS_CHECK(m_renderPass);
  LESS_CHECK(m_layout);
  LESS_CHECK(m_topology);
  LESS_CHECK(m_msaa);
  LESS_CHECK(m_uiAttachmentCount);
  LESS_CHECK(m_pCurrentRasterizerState);
  LESS_CHECK(m_pCurrentBlendState);
  LESS_CHECK(m_pCurrentDepthStencilState);
  LESS_CHECK(m_pCurrentShader);
  LESS_CHECK(m_pCurrentVertexDecl);

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (a.m_VertexBufferStrides[i] != b.m_VertexBufferStrides[i])
      return a.m_VertexBufferStrides[i] < b.m_VertexBufferStrides[i];
  }
  return false;

#undef LESS_CHECK
}

xiiUInt32 xiiResourceCacheVulkan::ResourceCacheHash::Hash(const FramebufferKey& key)
{
  xiiHashStreamWriter32 writer;
  writer << key.m_renderPass;
  writer << key.m_renderTargetSetup.GetDepthStencilTarget();
  const xiiUInt8 uiCount = key.m_renderTargetSetup.GetRenderTargetCount();
  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << key.m_renderTargetSetup.GetRenderTarget(i);
  }
  return writer.GetHashValue();
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const FramebufferKey& a, const FramebufferKey& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_renderTargetSetup == b.m_renderTargetSetup;
}

xiiUInt32 xiiResourceCacheVulkan::ResourceCacheHash::Hash(const PipelineLayoutDesc& desc)
{
  xiiHashStreamWriter32 writer;
  writer << desc.m_layout;
  return writer.GetHashValue();
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b)
{
  return a.m_layout == b.m_layout;
}

xiiUInt32 xiiResourceCacheVulkan::ResourceCacheHash::Hash(const GraphicsPipelineDesc& desc)
{
  xiiHashStreamWriter32 writer;
  writer << desc.m_renderPass;
  writer << desc.m_layout;
  writer << desc.m_topology;
  writer << desc.m_msaa;
  writer << desc.m_uiAttachmentCount;
  writer << desc.m_pCurrentRasterizerState;
  writer << desc.m_pCurrentBlendState;
  writer << desc.m_pCurrentDepthStencilState;
  writer << desc.m_pCurrentShader;
  writer << desc.m_pCurrentVertexDecl;
  writer.WriteArray(desc.m_VertexBufferStrides).IgnoreResult();
  return writer.GetHashValue();
}

bool ArraysEqual(const xiiUInt32 (&a)[XII_GAL_MAX_VERTEX_BUFFER_COUNT], const xiiUInt32 (&b)[XII_GAL_MAX_VERTEX_BUFFER_COUNT])
{
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_layout == b.m_layout && a.m_topology == b.m_topology && a.m_msaa == b.m_msaa && a.m_uiAttachmentCount == b.m_uiAttachmentCount && a.m_pCurrentRasterizerState == b.m_pCurrentRasterizerState && a.m_pCurrentBlendState == b.m_pCurrentBlendState && a.m_pCurrentDepthStencilState == b.m_pCurrentDepthStencilState && a.m_pCurrentShader == b.m_pCurrentShader && a.m_pCurrentVertexDecl == b.m_pCurrentVertexDecl && ArraysEqual(a.m_VertexBufferStrides, b.m_VertexBufferStrides);
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  if (a.m_layout != b.m_layout)
    return a.m_layout < b.m_layout;
  return a.m_pCurrentShader < b.m_pCurrentShader;
}

bool xiiResourceCacheVulkan::ResourceCacheHash::Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  return a.m_layout == b.m_layout && a.m_pCurrentShader == b.m_pCurrentShader;
}
