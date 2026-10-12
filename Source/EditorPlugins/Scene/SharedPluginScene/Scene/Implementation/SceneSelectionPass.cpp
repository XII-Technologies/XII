/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Editor/SceneSelectionConstants.h>
#include <SharedPluginScene/Scene/SceneSelectionPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneSelectionPass, 1, xiiRTTIDefaultAllocator<xiiSceneSelectionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VisibleColor", m_VisibleColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor(1.0f, 0.5f, 0.0f, 1.0f))),
    XII_MEMBER_PROPERTY("OccludedColor", m_OccludedColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor(0.2f, 0.4f, 1.0f, 0.5f))),
    XII_MEMBER_PROPERTY("FillOpacity", m_fFillOpacity)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.1f)),
    XII_MEMBER_PROPERTY("OutlineWidth", m_uiOutlineWidth)->AddAttributes(new xiiClampValueAttribute(1, 4), new xiiDefaultValueAttribute(1)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneSelectionPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"Color", xiiRenderPipelinePin::Texture, true, false});
  pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, false});
  pins.PushBack({"SelectionSurface", xiiRenderPipelinePin::Texture, true, true, xiiGALResourceFormat::RGBA32UInt});
  pins.PushBack({"Color", xiiRenderPipelinePin::Texture, false, false});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
}

xiiResult xiiSceneSelectionPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !m_VisibleColor.IsValid() || !m_OccludedColor.IsValid() || !xiiMath::IsFinite(m_fFillOpacity) ||
      m_fFillOpacity < 0 || m_fFillOpacity > 1 || m_uiOutlineWidth < 1 || m_uiOutlineWidth > 4)
    return XII_FAILURE;
  bool enabled = true;
  if (const auto* setting = context.m_Parameters.GetValue("Editor.SelectionOverlay"))
  {
    xiiResult result = XII_FAILURE;
    enabled          = setting->ConvertTo<bool>(&result);
    XII_SUCCEED_OR_RETURN(result);
  }
  auto       color     = context.GetInputTexture("Color");
  const auto selection = context.GetInputTexture("SelectionSurface");
  if (!enabled || context.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView || !selection.IsValid())
    return context.SetOutput("Color", color);
  const auto  depth      = context.GetInputTexture("SceneDepth");
  const auto* output     = context.m_Graph.GetTextureDescription(color);
  const auto* sceneDepth = context.m_Graph.GetTextureDescription(depth);
  const auto* surface    = context.m_Graph.GetTextureDescription(selection);
  const auto& viewport   = context.m_pView->GetViewport();
  if (!output || !sceneDepth || !surface || output->m_Type != xiiGALResourceDimension::Texture2D ||
      sceneDepth->m_Type != xiiGALResourceDimension::Texture2D || surface->m_Type != xiiGALResourceDimension::Texture2D ||
      output->m_uiSampleCount != 1 || sceneDepth->m_uiSampleCount != 1 || surface->m_uiSampleCount != 1 ||
      !output->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || !sceneDepth->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ||
      !surface->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !xiiGALResourceFormat::IsDepthFormat(sceneDepth->m_Format) ||
      viewport.x < 0 || viewport.y < 0 || viewport.x + viewport.width > output->m_Size.width || viewport.y + viewport.height > output->m_Size.height)
    return XII_FAILURE;
  auto                                          shader      = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Editor/SceneSelection.xiiShader");
  auto                                          permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
  if (!permutation.IsValid())
    return XII_FAILURE;
  xiiResourceLock<xiiShaderPermutationResource> program(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!program || !program->IsShaderValid() || !program->GetGALShader(xiiGALShaderType::Vertex) || !program->GetGALShader(xiiGALShaderType::Pixel))
    return XII_FAILURE;
  xiiGALGraphicsPipelineStateCreationDescription pipeline;
  pipeline.m_pPipelineResourceSignature            = program->GetPipelineResourceSignature();
  pipeline.m_pVertexShader                         = program->GetGALShader(xiiGALShaderType::Vertex);
  pipeline.m_pPixelShader                          = program->GetGALShader(xiiGALShaderType::Pixel);
  pipeline.m_GraphicsPipeline.m_pBlendState        = program->GetBlendState();
  pipeline.m_GraphicsPipeline.m_pDepthStencilState = program->GetDepthStencilState();
  pipeline.m_GraphicsPipeline.m_pRasterizerState   = program->GetRasterizerState();
  pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
  pipeline.m_GraphicsPipeline.m_uiViewportCount    = 1;
  struct Data
  {
    xiiRenderGraphTextureHandle m_Color, m_Depth, m_Surface;
    xiiRenderGraphBufferHandle  m_Constants;
    xiiSceneSelectionConstants  m_Settings;
    bool                        m_bUploaded = false;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    color = data.m_Color = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Surface = builder.ReadTexture(selection, xiiGALResourceStateFlags::ShaderResource);
    data.m_Settings.VisibleColor = m_VisibleColor.GetAsVec4();
    data.m_Settings.OccludedColor = m_OccludedColor.GetAsVec4();
    data.m_Settings.FillOpacity = m_fFillOpacity;
    data.m_Settings.OutlineWidth = m_uiOutlineWidth;
    data.m_Settings.Padding = xiiVec2::MakeZero();
    xiiGALBufferCreationDescription buffer;
    buffer.m_uiSize = sizeof(xiiSceneSelectionConstants);
    buffer.m_BindFlags = xiiGALBindFlags::UniformBuffer;
    buffer.m_Usage = xiiGALResourceUsage::Dynamic;
    buffer.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    xiiStringBuilder name(context.m_sPassName, ": Constants");
    data.m_Constants = builder.WriteBuffer(name, buffer, xiiGALResourceStateFlags::ConstantBuffer);
    builder.SetPassAllowMerge(false);
    builder.SetPassRenderPassManaged();
    builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
      {
        xiiGALMapHelper<xiiSceneSelectionConstants> mapped(execution.GetCommandList(), execution.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        if (mapped.GetMappedData())
        {
          *mapped.GetMappedData() = data.m_Settings;
          data.m_bUploaded = true;
        }
        else
          xiiLog::Error("Failed to upload scene selection constants.");
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execution.GetBuffer(data.m_Constants);
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      execution.GetCommandList().TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    }); }, [pipeline](const Data& data, xiiRenderGraphPassContext& execution) mutable {
    if (!data.m_bUploaded)
      return;
    pipeline.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
    pipeline.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
    auto state = xiiGALPipelineCache::GetPipeline(pipeline);
    if (!state)
    {
      xiiLog::Error("Failed to create the scene selection pipeline.");
      return;
    }
    auto& commands = execution.GetCommandList();
    const auto& viewport = execution.GetView()->GetViewport();
    commands.SetViewport({viewport.x, viewport.y, viewport.width, viewport.height, 0, 1});
    commands.SetPipelineState(state.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSceneSelectionConstants", execution.GetBuffer(data.m_Constants), xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("SceneDepth", execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("SelectionSurface", execution.GetTexture(data.m_Surface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded())
      commands.Draw({3}); });
  return context.SetOutput("Color", color);
}
