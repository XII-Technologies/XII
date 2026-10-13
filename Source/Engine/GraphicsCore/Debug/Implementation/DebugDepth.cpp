/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Debug/Implementation/DebugDepth.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

namespace xiiDebugRendererInternal
{
  xiiRenderGraphTextureHandle PrepareDepth(xiiRenderGraph& graph)
  {
    const auto  depth  = graph.FindTexture(xiiRGBlackboardKeys::k_SceneDepthTexture);
    const auto  color  = graph.FindTexture(xiiRGBlackboardKeys::k_DisplayLinearColor);
    const auto* source = graph.GetTextureDescription(depth);
    const auto* target = graph.GetTextureDescription(color);
    if (!source || !target)
      return {};
    if (source->m_Size == target->m_Size)
      return source->m_uiSampleCount == target->m_uiSampleCount ? depth : xiiRenderGraphTextureHandle{};
    if (source->m_Type != xiiGALResourceDimension::Texture2D || target->m_Type != xiiGALResourceDimension::Texture2D ||
        !source->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || source->m_uiSampleCount != 1 || target->m_uiSampleCount != 1)
      return {};

    const auto shader      = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugDepth.xiiShader");
    const auto permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
    if (!permutation.IsValid())
      return {};
    xiiResourceLock<xiiShaderPermutationResource> program(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!program.IsValid() || !program->IsShaderValid())
      return {};

    xiiGALGraphicsPipelineStateCreationDescription pipeline;
    pipeline.m_pPipelineResourceSignature            = program->GetPipelineResourceSignature();
    pipeline.m_pVertexShader                         = program->GetGALShader(xiiGALShaderType::Vertex);
    pipeline.m_pPixelShader                          = program->GetGALShader(xiiGALShaderType::Pixel);
    pipeline.m_GraphicsPipeline.m_pBlendState        = program->GetBlendState();
    pipeline.m_GraphicsPipeline.m_pRasterizerState   = program->GetRasterizerState();
    pipeline.m_GraphicsPipeline.m_pDepthStencilState = program->GetDepthStencilState();
    pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
    pipeline.m_GraphicsPipeline.m_uiViewportCount    = 1;
    if (!pipeline.m_pVertexShader || !pipeline.m_pPixelShader)
      return {};

    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = target->m_Size;
    description.m_Format    = xiiGALResourceFormat::D32Float;
    description.m_BindFlags = xiiGALBindFlags::DepthStencil;
    struct Data
    {
      xiiRenderGraphTextureHandle m_Source, m_Target;
    };
    auto pass = graph.AddPass<Data>("Debug display depth", xiiGALCommandQueueFlags::Graphics, [depth, description](Data& data, xiiRenderGraphBuilder& builder) {
        data.m_Source = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
        data.m_Target = builder.WriteTexture("Debug display depth", description, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged(); }, [pipeline](const Data& data, xiiRenderGraphPassContext& context) mutable {
        pipeline.m_GraphicsPipeline.m_pRenderPass = context.GetRenderPass();
        pipeline.m_GraphicsPipeline.m_uiSubpassIndex = context.GetSubpassIndex();
        auto state = xiiGALPipelineCache::GetPipeline(pipeline);
        if (!state)
        {
          xiiLog::Error("Failed to create the debug depth resampling pipeline.");
          return;
        }
        auto& commands = context.GetCommandList();
        // Pipeline intermediates use local coordinates; the view origin belongs to final presentation.
        const auto& size = context.GetTexture(data.m_Target)->GetDescription().m_Size;
        commands.SetViewport({0, 0, static_cast<float>(size.width), static_cast<float>(size.height), 0, 1});
        commands.SetPipelineState(state.Borrow());
        commands.ResolveAndSetShaderResourceTextureView("SourceDepth", context.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded())
          commands.Draw({3, 1}); });
    return pass.first->m_Target;
  }
} // namespace xiiDebugRendererInternal
