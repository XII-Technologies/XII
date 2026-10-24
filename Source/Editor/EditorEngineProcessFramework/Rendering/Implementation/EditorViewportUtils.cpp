/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/Rendering/Implementation/EditorViewportUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>

xiiResult xiiEditorViewportUtils::PixelRatio(const xiiView& view, float& ratio)
{
  ratio = 1;
  if (const auto* value = view.GetPipelineParameters().GetValue("Editor.DevicePixelRatio"))
  {
    if (!value->IsA<float>())
      return XII_FAILURE;
    ratio = value->Get<float>();
  }
  return xiiMath::IsFinite(ratio) && ratio > 0 && ratio <= 8 ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiEditorViewportUtils::LoadPipeline(xiiStringView shader, xiiGALGraphicsPipelineStateCreationDescription& pipeline)
{
  auto permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>(shader), {}, false);
  if (!permutation.IsValid())
    return XII_FAILURE;
  xiiResourceLock<xiiShaderPermutationResource> program(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!program || !program->IsShaderValid() || !program->GetGALShader(xiiGALShaderType::Vertex) || !program->GetGALShader(xiiGALShaderType::Pixel))
    return XII_FAILURE;
  pipeline.m_pPipelineResourceSignature            = program->GetPipelineResourceSignature();
  pipeline.m_pVertexShader                         = program->GetGALShader(xiiGALShaderType::Vertex);
  pipeline.m_pPixelShader                          = program->GetGALShader(xiiGALShaderType::Pixel);
  pipeline.m_GraphicsPipeline.m_pBlendState        = program->GetBlendState();
  pipeline.m_GraphicsPipeline.m_pDepthStencilState = program->GetDepthStencilState();
  pipeline.m_GraphicsPipeline.m_pRasterizerState   = program->GetRasterizerState();
  pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
  pipeline.m_GraphicsPipeline.m_uiViewportCount    = 1;
  return XII_SUCCESS;
}

xiiResult xiiEditorViewportUtils::ValidateTargets(const xiiRenderGraph& graph, xiiRenderGraphTextureHandle color, xiiRenderGraphTextureHandle depth)
{
  const auto* output     = graph.GetTextureDescription(color);
  const auto* sceneDepth = graph.GetTextureDescription(depth);
  return output && sceneDepth && output->m_Type == xiiGALResourceDimension::Texture2D && sceneDepth->m_Type == xiiGALResourceDimension::Texture2D && output->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) && sceneDepth->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) && xiiGALResourceFormat::IsDepthFormat(sceneDepth->m_Format) && output->m_uiSampleCount == 1 && sceneDepth->m_uiSampleCount == 1 && output->m_Size.HasNonZeroArea() ? XII_SUCCESS : XII_FAILURE;
}
