/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Editor/SceneShapeIconData.h>
#include <SharedPluginScene/Scene/Implementation/SceneShapeIconPicking.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneShapeIconPass, 1, xiiRTTIDefaultAllocator<xiiSceneShapeIconPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ColorInput", m_InputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ColorOutput", m_OutputColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color")),
    XII_MEMBER_PROPERTY("Picking", m_OutputPicking)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(64.0f), new xiiSuffixAttribute(" px")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSceneShapeIconPass::xiiSceneShapeIconPass()  = default;
xiiSceneShapeIconPass::~xiiSceneShapeIconPass() = default;

xiiResult xiiSceneShapeIconPass::PickIcon(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const
{
  out_hit = {};
  return m_pPicking ? m_pPicking->Pick(view, x, y, out_hit) : XII_FAILURE;
}

xiiResult xiiSceneShapeIconPass::PickIconsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<PickingResult>& out_hits) const
{
  out_hits.Clear();
  return m_pPicking ? m_pPicking->Marquee(view, rectangle, out_hits) : XII_FAILURE;
}

namespace
{
  struct Batch
  {
    xiiRenderGraphTextureHandle m_Texture;
    xiiSharedPtr<xiiGALSampler> m_pSampler;
    xiiUInt32                   m_uiFirst = 0, m_uiCount = 0;
  };
  struct DrawData
  {
    xiiRenderGraphTextureHandle            m_Color, m_Depth, m_Picking;
    xiiRenderGraphBufferHandle             m_Instances;
    xiiDynamicArray<xiiSceneShapeIconData> m_Data;
    xiiDynamicArray<Batch>                 m_Batches;
    bool                                   m_bUploaded = false;
  };
} // namespace



xiiResult xiiSceneShapeIconPass::Build(xiiRenderPipelinePassContext& context)
{
  bool captureValid = false;
  XII_SCOPE_EXIT(if (!captureValid && m_pPicking) m_pPicking->Invalidate());
  m_Icons.Clear();
  if (!context.m_pView)
    return XII_FAILURE;
  bool enabled = true;
  bool capture = true;
  if (const auto* parameter = context.m_Parameters.GetValue("Editor.Picking"))
  {
    xiiResult result = XII_FAILURE;
    capture          = parameter->ConvertTo<bool>(&result);
    XII_SUCCEED_OR_RETURN(result);
  }
  if (const auto* parameter = context.m_Parameters.GetValue("Editor.ShapeIcons"))
  {
    xiiResult result = XII_FAILURE;
    enabled          = parameter->ConvertTo<bool>(&result);
    XII_SUCCEED_OR_RETURN(result);
  }
  auto color = context.GetInputTexture(m_InputColor);
  if (!enabled || context.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return context.SetOutput(m_OutputColor, color);
  XII_SUCCEED_OR_RETURN(Collect(*context.m_pView));
  auto        depth      = context.GetInputTexture(m_InputSceneDepth);
  auto&       graph      = context.m_Graph;
  const auto* output     = graph.GetTextureDescription(color);
  const auto* sceneDepth = graph.GetTextureDescription(depth);
  if (!output || !sceneDepth || output->m_Type != xiiGALResourceDimension::Texture2D || sceneDepth->m_Type != xiiGALResourceDimension::Texture2D || !output->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || !sceneDepth->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !xiiGALResourceFormat::IsDepthFormat(sceneDepth->m_Format) || output->m_uiSampleCount != 1 || sceneDepth->m_uiSampleCount != 1 || !output->m_Size.HasNonZeroArea())
  {
    m_Icons.Clear();
    return XII_FAILURE;
  }

  xiiGALGraphicsPipelineStateCreationDescription pipeline;
  if (!m_Icons.IsEmpty())
  {
    auto shader      = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Editor/SceneShapeIcons.xiiShader");
    auto permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
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
  }

  // Clear even when no icons remain, so a downstream picking consumer never sees last frame's IDs.
  auto pickingDescription          = *output;
  pickingDescription.m_Format      = xiiGALResourceFormat::R32UInt;
  pickingDescription.m_uiMipLevels = 1;
  pickingDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  xiiRenderGraphTextureHandle picking;
  xiiStringBuilder            pickingName(context.m_sPassName, ": Picking");
  xiiStringBuilder            clearName(context.m_sPassName, ": Clear picking");
  graph.AddPass<DrawData>(clearName, xiiGALCommandQueueFlags::Graphics, [&](DrawData& data, xiiRenderGraphBuilder& builder) {
    picking = data.m_Picking = builder.WriteTexture(pickingName, pickingDescription, xiiGALResourceStateFlags::RenderTarget);
    builder.SetPassRenderPassManaged(); }, [](const DrawData& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Picking)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); });
  if (!m_Icons.IsEmpty())
  {
    graph.AddPass<DrawData>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](DrawData& data, xiiRenderGraphBuilder& builder) {
      color = data.m_Color = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
      picking = data.m_Picking = builder.ReadWriteTexture(picking, xiiGALResourceStateFlags::RenderTarget);
      data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
      data.m_Data.Reserve(m_Icons.GetCount());
      xiiTexture2DResourceHandle previous;
      for (xiiUInt32 index = 0; index < m_Icons.GetCount(); ++index)
      {
        const auto& icon = m_Icons[index];
        auto& instance = data.m_Data.ExpandAndGetRef();
        instance.ClipRect = xiiVec4(icon.m_vScreenPosition.x * 2 - 1 - icon.m_vScreenSize.x, 1 - icon.m_vScreenPosition.y * 2 + icon.m_vScreenSize.y, icon.m_vScreenSize.x * 2, -icon.m_vScreenSize.y * 2);
        instance.Color = xiiVec4(icon.m_Color.r, icon.m_Color.g, icon.m_Color.b, 1);
        instance.Depth = icon.m_vScreenPosition.z;
        instance.PickingID = index + 1;
        instance.Padding = xiiVec2::MakeZero();
        if (previous == icon.m_hTexture && !data.m_Batches.IsEmpty())
        {
          ++data.m_Batches.PeekBack().m_uiCount;
          continue;
        }
        previous.Invalidate();
        xiiResourceLock<xiiTexture2DResource> texture(icon.m_hTexture, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (!texture || !texture->GetGALTexture() || !texture->GetGALSampler())
          continue;
        auto source = texture->GetGALTexture();
        if (source->GetDescription().m_Type != xiiGALResourceDimension::Texture2D || source->GetDescription().m_uiSampleCount != 1)
          continue;
        auto& batch = data.m_Batches.ExpandAndGetRef();
        xiiStringBuilder textureName("Shape icon texture: ", icon.m_hTexture.GetResourceID());
        batch.m_Texture = builder.ReadTexture(builder.ImportTexture(textureName, source, source->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
        batch.m_pSampler = texture->GetGALSampler();
        batch.m_uiFirst = index;
        batch.m_uiCount = 1;
        previous = icon.m_hTexture;
      }
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = data.m_Data.GetCount() * sizeof(xiiSceneShapeIconData);
      buffer.m_uiElementByteStride = sizeof(xiiSceneShapeIconData);
      buffer.m_Mode = xiiGALBufferMode::Structured;
      buffer.m_Usage = xiiGALResourceUsage::Dynamic;
      buffer.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
      xiiStringBuilder instanceName(context.m_sPassName, ": Instances");
      data.m_Instances = builder.WriteBuffer(instanceName, buffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassAllowMerge(false);
      builder.SetPassRenderPassManaged();
      builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
        {
        xiiGALMapHelper<xiiSceneShapeIconData> mapped(execution.GetCommandList(), execution.GetBuffer(data.m_Instances), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        if (mapped.GetMappedData())
        {
          xiiMemoryUtils::Copy(mapped.GetMappedData(), data.m_Data.GetData(), data.m_Data.GetCount());
          data.m_bUploaded = true;
        }
        else
          xiiLog::Error("Failed to upload scene shape icons.");
        }
        xiiGALStateTransitionDescription transition;
        transition.m_pResource = execution.GetBuffer(data.m_Instances);
        transition.m_NewState = xiiGALResourceStateFlags::ShaderResource;
        transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        execution.GetCommandList().TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      }); }, [pipeline](const DrawData& data, xiiRenderGraphPassContext& execution) mutable {
      if (!data.m_bUploaded)
        return;
      pipeline.m_GraphicsPipeline.m_pRenderPass = execution.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state)
      {
        xiiLog::Error("Failed to create the scene shape icon graphics pipeline.");
        return;
      }
      auto& commands = execution.GetCommandList();
      const auto size     = execution.GetTexture(data.m_Color)->GetDescription().m_Size;
      commands.SetViewport({0, 0, float(size.width), float(size.height), 0, 1});
      commands.SetPipelineState(state.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("IconData", execution.GetBuffer(data.m_Instances)->GetDefaultView(xiiGALBufferViewType::ShaderResource).Borrow(), xiiGALShaderType::Vertex);
      commands.ResolveAndSetShaderResourceTextureView("SceneDepth", execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      for (const auto& batch : data.m_Batches)
      {
        commands.ResolveAndSetShaderResourceTextureView("IconTexture", execution.GetTexture(batch.m_Texture)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
        commands.ResolveAndSetSampler("IconSampler", batch.m_pSampler.Borrow(), xiiGALShaderType::Pixel);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded())
          commands.Draw({6, batch.m_uiCount, 0, batch.m_uiFirst});
      } });
  }
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputColor, color));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputPicking, picking));
  if (capture)
  {
    if (!m_pPicking)
      m_pPicking = XII_DEFAULT_NEW(xiiSceneShapeIconPicking);
    XII_SUCCEED_OR_RETURN(m_pPicking->AddCapture(context, picking, m_Icons));
    captureValid = true;
  }
  return XII_SUCCESS;
}
