/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/DepthRegionClearPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <Shaders/Utility/DepthRegionClearConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthRegionClearPass, 1, xiiRTTIDefaultAllocator<xiiDepthRegionClearPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DepthTarget", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Depth", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Viewport", m_Viewport)->AddAttributes(new xiiDefaultValueAttribute(xiiVec4U32::MakeZero())),
    XII_MEMBER_PROPERTY("ClearDepth", m_fDepth)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDepthRegionClearPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return ((m_Viewport.z && m_Viewport.w) || m_Viewport == xiiVec4U32::MakeZero()) && xiiMath::IsFinite(m_fDepth) && m_fDepth >= 0 && m_fDepth <= 1 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiDepthRegionClearPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input = context.GetInputTexture(m_Input);
  const auto* image = context.m_Graph.GetTextureDescription(input);
  if (!image || image->m_Format != xiiGALResourceFormat::D32Float || image->m_Type != xiiGALResourceDimension::Texture2D || image->m_uiSampleCount != 1 || image->m_uiArraySizeOrDepth != 1)
    return context.ReportFailure("Depth region clear requires a single-sample D32 image.");
  const xiiVec4U32 viewport = m_Viewport.z ? m_Viewport : xiiVec4U32(0, 0, image->m_Size.width, image->m_Size.height);
  if (!viewport.z || !viewport.w || viewport.x > image->m_Size.width || viewport.y > image->m_Size.height || viewport.z > image->m_Size.width - viewport.x || viewport.w > image->m_Size.height - viewport.y)
    return context.ReportFailure("Depth clear viewport is outside its supplied image.");
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Utility/DepthRegionClear.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Depth clear shader is unavailable.");
  if (!m_Constants || m_fCachedDepth != m_fDepth)
  {
    xiiDepthRegionClearConstants value{};
    value.DepthValue = m_fDepth;
    xiiGALBufferCreationDescription uniform;
    uniform.m_uiSize    = sizeof(value);
    uniform.m_BindFlags = xiiGALBindFlags::UniformBuffer;
    uniform.m_Usage     = xiiGALResourceUsage::Immutable;
    xiiGALBufferData initial(&value, sizeof(value));
    m_Constants = xiiGALDevice::GetDefaultDevice()->CreateBuffer(uniform, &initial);
    if (!m_Constants)
      return context.ReportFailure("Could not allocate immutable depth-clear parameters.");
    m_fCachedDepth = m_fDepth;
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle                    m_Depth;
    xiiRenderGraphBufferHandle                     m_Constants;
    xiiVec4U32                                     m_Viewport;
    xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth = builder.ReadWriteTexture(input, xiiGALResourceStateFlags::DepthWrite); data.m_Viewport = viewport;
    data.m_Constants = builder.ReadBuffer(builder.ImportBuffer("Depth Clear Value", m_Constants, m_Constants->GetResourceState()), xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature(); data.m_Pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex); data.m_Pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
    data.m_Pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState(); data.m_Pipeline.m_GraphicsPipeline.m_pRasterizerState = shader->GetRasterizerState(); data.m_Pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
    data.m_Pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList; builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    auto description = data.m_Pipeline; description.m_GraphicsPipeline.m_pRenderPass = execute.GetRenderPass(); description.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(execute.GetSubpassIndex());
    auto pipeline = xiiGALPipelineCache::GetPipeline(description); if (!pipeline) { xiiLog::Error("Could not create depth-region clear pipeline."); return; }
    commands.SetPipelineState(pipeline); commands.SetViewport({float(data.m_Viewport.x), float(data.m_Viewport.y), float(data.m_Viewport.z), float(data.m_Viewport.w), 0, 1}); commands.SetScissorRect({data.m_Viewport.x, data.m_Viewport.y, data.m_Viewport.z, data.m_Viewport.w});
    commands.ResolveAndSetConstantBuffer("xiiDepthRegionClearConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Pixel); commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess(); commands.Draw({3}); });
  return context.SetOutput(m_Output, emitted.first->m_Depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_DepthRegionClearPass);
