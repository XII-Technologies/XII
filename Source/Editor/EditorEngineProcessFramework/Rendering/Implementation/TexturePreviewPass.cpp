/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Editor/TexturePreviewConstants.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTexturePreviewPass, 1, xiiRTTIDefaultAllocator<xiiTexturePreviewPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Cube", m_bCube),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  struct PreviewData
  {
    xiiRenderGraphTextureHandle m_Color, m_Depth, m_Source;
    xiiRenderGraphBufferHandle  m_Vertices, m_Indices, m_Constants;
    xiiTexturePreviewConstants  m_Settings;
    xiiSharedPtr<xiiGALSampler> m_pSampler;
    xiiUInt32                   m_uiIndexCount = 0;
    xiiEnum<xiiGALValueType>    m_IndexType;
    bool                        m_bConstantsReady = false;
  };

  template <typename T>
  xiiResult Parameter(const xiiVariantDictionary& parameters, xiiStringView name, T& out_value)
  {
    const auto it = parameters.Find(name);
    if (it.IsValid())
    {
      xiiResult result = XII_FAILURE;
      out_value        = it.Value().ConvertTo<T>(&result);
      return result;
    }
    return XII_SUCCESS;
  }
} // namespace

xiiRenderPipelineResourceHandle xiiTexturePreviewPass::GetPipeline(bool bCube)
{
  const xiiStringView id       = bCube ? "{ XII Editor Cube Texture Preview }" : "{ XII Editor Texture Preview }";
  auto                existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(id);
  if (existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor descriptor;
  xiiVariantDictionary                properties;
  properties.Insert("Cube", bCube);
  descriptor.AddPass("Texture inspection", "xiiTexturePreviewPass", properties);
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(id, std::move(descriptor));
}

void xiiTexturePreviewPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  out_pins.PushBack({"Color", xiiRenderPipelinePin::Texture, false, false});
}

xiiResult xiiTexturePreviewPass::EnsureGeometry()
{
  if (m_hGeometry.IsValid())
    return XII_SUCCESS;
  const xiiStringView id = m_bCube ? "Editor cube texture inspection sphere" : "Editor texture inspection quad";
  m_hGeometry            = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(id);
  if (m_hGeometry.IsValid())
    return XII_SUCCESS;
  xiiGeometry geometry;
  if (m_bCube)
    geometry.AddStackedSphere(0.5f, 32, 32);
  else
  {
    xiiUInt32 indices[4];
    indices[0] = geometry.AddVertex(xiiVec3(-0.5f, 0, -0.5f), -xiiVec3::MakeAxisY(), xiiVec2(-1, 2));
    indices[1] = geometry.AddVertex(xiiVec3(0.5f, 0, -0.5f), -xiiVec3::MakeAxisY(), xiiVec2(2, 2));
    indices[2] = geometry.AddVertex(xiiVec3(0.5f, 0, 0.5f), -xiiVec3::MakeAxisY(), xiiVec2(2, -1));
    indices[3] = geometry.AddVertex(xiiVec3(-0.5f, 0, 0.5f), -xiiVec3::MakeAxisY(), xiiVec2(-1, -1));
    geometry.AddPolygon(indices, false);
  }
  geometry.ComputeFaceNormals();
  geometry.Transform(xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90)), true);
  geometry.ComputeTangents();
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  m_hGeometry = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(id, std::move(descriptor));
  return m_hGeometry.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiTexturePreviewPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* view = context.m_pView;
  if (!view || !view->IsValid() || view->GetCamera()->GetCameraMode() == xiiCameraMode::None)
    return XII_FAILURE;
  xiiString textureID;
  xiiInt32  channel = 0, mip = -1;
  float     threshold = 0.5f;
  XII_SUCCEED_OR_RETURN(Parameter(context.m_Parameters, "Texture", textureID));
  XII_SUCCEED_OR_RETURN(Parameter(context.m_Parameters, "Channel", channel));
  XII_SUCCEED_OR_RETURN(Parameter(context.m_Parameters, "Mip", mip));
  XII_SUCCEED_OR_RETURN(Parameter(context.m_Parameters, "AlphaThreshold", threshold));
  if (textureID.IsEmpty() || channel < 0 || channel > 7 || mip < -1 || mip > 31 || !xiiMath::IsFinite(threshold) || threshold < 0 || threshold > 1)
    return XII_FAILURE;
  xiiSharedPtr<xiiGALTexture> source;
  xiiSharedPtr<xiiGALSampler> sampler;
  if (m_bCube)
  {
    xiiResourceLock<xiiTextureCubeResource> resource(xiiResourceManager::LoadResource<xiiTextureCubeResource>(textureID), xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!resource || resource->GetLoadingState() != xiiResourceState::Loaded)
      return XII_FAILURE;
    source  = resource->GetGALTexture();
    sampler = resource->GetGALSampler();
  }
  else
  {
    xiiResourceLock<xiiTexture2DResource> resource(xiiResourceManager::LoadResource<xiiTexture2DResource>(textureID), xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!resource || resource->GetLoadingState() != xiiResourceState::Loaded)
      return XII_FAILURE;
    source  = resource->GetGALTexture();
    sampler = resource->GetGALSampler();
  }
  xiiSharedPtr<xiiGALTexture> output;
  if (view->GetSwapChain())
    output = view->GetSwapChain()->GetBackBufferTexture();
  else if (view->GetRenderTargetView())
    output = view->GetRenderTargetView()->GetTexture();
  if (!source || !sampler || !output || source == output || source->GetDescription().m_Type != (m_bCube ? xiiGALResourceDimension::TextureCube : xiiGALResourceDimension::Texture2D))
    return XII_FAILURE;
  if (output->GetDescription().m_uiSampleCount != 1 || source->GetDescription().m_uiSampleCount != 1)
    return XII_FAILURE;
  const auto& viewport = view->GetViewport();
  if (viewport.width <= 0 || viewport.height <= 0 || !xiiMath::IsFinite(viewport.x) || !xiiMath::IsFinite(viewport.y) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height))
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(EnsureGeometry());
  xiiResourceLock<xiiMeshBufferResource> geometry(m_hGeometry, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!geometry || !geometry->GetVertexBuffer() || !geometry->GetIndexBuffer())
    return XII_FAILURE;
  auto                                          shader      = xiiResourceManager::LoadResource<xiiShaderResource>(m_bCube ? "Shaders/Editor/TextureCubePreview.xiiShader" : "Shaders/Editor/TexturePreview.xiiShader");
  auto                                          permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, true);
  xiiResourceLock<xiiShaderPermutationResource> program(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!program || !program->IsShaderValid() || !program->GetGALShader(xiiGALShaderType::Vertex) || !program->GetGALShader(xiiGALShaderType::Pixel))
    return XII_FAILURE;
  if (!m_pInputLayout)
  {
    xiiGALInputLayoutCreationDescription layout;
    layout.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::Position, 0, xiiGALResourceFormat::RGB32Float, offsetof(xiiMeshPackedVertex, m_vPosition), sizeof(xiiMeshPackedVertex), xiiGALInputElementFrequency::PerVertex, 1));
    layout.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::Normal, 0, xiiGALResourceFormat::RGB32Float, offsetof(xiiMeshPackedVertex, m_vNormal), sizeof(xiiMeshPackedVertex), xiiGALInputElementFrequency::PerVertex, 1));
    layout.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::TexCoord0, 0, xiiGALResourceFormat::RG32Float, offsetof(xiiMeshPackedVertex, m_vTexCoord0), sizeof(xiiMeshPackedVertex), xiiGALInputElementFrequency::PerVertex, 1));
    m_pInputLayout = program->GetGALShader(xiiGALShaderType::Vertex)->CreateInputLayout(layout);
    if (!m_pInputLayout)
      return XII_FAILURE;
  }
  xiiGALGraphicsPipelineStateCreationDescription pipeline;
  pipeline.m_pPipelineResourceSignature            = program->GetPipelineResourceSignature();
  pipeline.m_pVertexShader                         = program->GetGALShader(xiiGALShaderType::Vertex);
  pipeline.m_pPixelShader                          = program->GetGALShader(xiiGALShaderType::Pixel);
  pipeline.m_GraphicsPipeline.m_pBlendState        = program->GetBlendState();
  pipeline.m_GraphicsPipeline.m_pDepthStencilState = program->GetDepthStencilState();
  pipeline.m_GraphicsPipeline.m_pRasterizerState   = program->GetRasterizerState();
  pipeline.m_GraphicsPipeline.m_pInputLayout       = m_pInputLayout;
  pipeline.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
  pipeline.m_GraphicsPipeline.m_uiViewportCount    = 1;

  auto&                       graph = context.m_Graph;
  xiiRenderGraphTextureHandle color, depth;
  const auto                  setupClear = [&](PreviewData& data, xiiRenderGraphBuilder& builder) {
    color = data.m_Color = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, output, output->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = output->GetDescription().m_Size;
    description.m_Format    = xiiGALResourceFormat::D32Float;
    description.m_BindFlags = xiiGALBindFlags::DepthStencil;
    depth = data.m_Depth = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, description, xiiGALResourceStateFlags::DepthWrite);
  };
  const auto executeClear = [](const PreviewData& data, xiiRenderGraphPassContext& execution) {
    auto& commands = execution.GetCommandList();
    commands.ClearRenderTargetView(execution.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
    commands.ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0);
  };
  graph.AddPass<PreviewData>("Clear texture preview", xiiGALCommandQueueFlags::Graphics, setupClear, executeClear);
  const auto setupInspection = [&](PreviewData& data, xiiRenderGraphBuilder& builder) {
    data.m_Color    = builder.ReadWriteTexture(color, xiiGALResourceStateFlags::RenderTarget);
    data.m_Depth    = builder.ReadWriteTexture(depth, xiiGALResourceStateFlags::DepthWrite);
    data.m_Source   = builder.ReadTexture(builder.ImportTexture("Inspected texture", source, source->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    data.m_Vertices = builder.ReadBuffer(builder.ImportBuffer("Preview vertices", geometry->GetVertexBuffer(), geometry->GetVertexBuffer()->GetResourceState()), xiiGALResourceStateFlags::VertexBuffer);
    data.m_Indices  = builder.ReadBuffer(builder.ImportBuffer("Preview indices", geometry->GetIndexBuffer(), geometry->GetIndexBuffer()->GetResourceState()), xiiGALResourceStateFlags::IndexBuffer);
    xiiGALBufferCreationDescription description;
    description.m_uiSize           = sizeof(xiiTexturePreviewConstants);
    description.m_BindFlags        = xiiGALBindFlags::UniformBuffer;
    description.m_Usage            = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags   = xiiGALCPUAccessFlag::Write;
    data.m_Constants               = builder.WriteBuffer("Texture inspection settings", description, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Settings.ViewProjection = view->GetViewProjectionMatrix(xiiCameraEye::Left);
    data.m_Settings.Channel        = channel;
    data.m_Settings.Mip            = mip < 0 ? -1.0f : static_cast<float>(xiiMath::Min(static_cast<xiiUInt32>(mip), source->GetDescription().m_uiMipLevels - 1));
    data.m_Settings.IsLinear       = !xiiGALResourceFormat::IsSrgb(source->GetDescription().m_Format);
    data.m_Settings.AlphaThreshold = threshold;
    data.m_Settings.OutputIsSrgb   = xiiGALResourceFormat::IsSrgb(output->GetDescription().m_Format);
    data.m_Settings.Padding        = xiiVec3::MakeZero();
    data.m_pSampler                = sampler;
    data.m_uiIndexCount            = geometry->GetIndexCount();
    data.m_IndexType               = geometry->GetIndexType();
    builder.SetPassAllowMerge(false);
    builder.SetPassRenderPassManaged();
    builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execution) {
      auto& commands = execution.GetCommandList();
      {
        xiiGALMapHelper<xiiTexturePreviewConstants> mapped(commands, execution.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        if (mapped.GetMappedData())
        {
          *mapped.GetMappedData() = data.m_Settings;
          data.m_bConstantsReady  = true;
        }
        else
        {
          xiiLog::Error("Failed to upload texture preview settings.");
        }
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource       = execution.GetBuffer(data.m_Constants);
      transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    });
  };
  const auto executeInspection = [pipeline](const PreviewData& data, xiiRenderGraphPassContext& execution) mutable {
    if (!data.m_bConstantsReady)
      return;
    pipeline.m_GraphicsPipeline.m_pRenderPass    = execution.GetRenderPass();
    pipeline.m_GraphicsPipeline.m_uiSubpassIndex = execution.GetSubpassIndex();
    auto state                                   = xiiGALPipelineCache::GetPipeline(pipeline);
    if (!state)
    {
      xiiLog::Error("Failed to create the texture preview graphics pipeline.");
      return;
    }
    auto&       commands = execution.GetCommandList();
    const auto& viewport = execution.GetView()->GetViewport();
    commands.SetViewport({viewport.x, viewport.y, viewport.width, viewport.height, 0, 1});
    commands.SetPipelineState(state.Borrow());
    xiiGALBuffer* vertices[] = {execution.GetBuffer(data.m_Vertices)};
    commands.SetVertexBuffers(0, xiiMakeArrayPtr(vertices), {}, xiiGALSetVertexBufferFlags::Reset, xiiGALStateTransitionMode::None);
    commands.SetIndexBuffer(execution.GetBuffer(data.m_Indices), 0, xiiGALStateTransitionMode::None);
    commands.ResolveAndSetConstantBuffer("xiiTexturePreviewConstants", execution.GetBuffer(data.m_Constants), xiiGALShaderType::Vertex | xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("PreviewTexture", execution.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    commands.ResolveAndSetSampler("PreviewSampler", data.m_pSampler.Borrow(), xiiGALShaderType::Pixel);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Succeeded())
      commands.DrawIndexed({data.m_uiIndexCount, data.m_IndexType});
  };
  graph.AddPass<PreviewData>("Inspect texture", xiiGALCommandQueueFlags::Graphics, setupInspection, executeInspection);
  xiiDebugRenderer::AddRenderGraphPasses(graph);
  xiiRenderGraphTextureHandle result;
  const bool                  swapChain    = view->GetSwapChain() != nullptr;
  const auto                  setupPublish = [&](PreviewData& data, xiiRenderGraphBuilder& builder) {
    result = data.m_Color = builder.ReadTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, xiiGALResourceStateFlags::RenderTarget);
    builder.ExportTexture(result, swapChain ? xiiGALResourceStateFlags::Present : xiiGALResourceStateFlags::ShaderResource);
  };
  // Export-only root: the graph performs the final transition after all drawing and overlays.
  graph.AddPass<PreviewData>("Publish texture preview", xiiGALCommandQueueFlags::Graphics, setupPublish, [](const PreviewData&, xiiRenderGraphPassContext&) {}, true);
  return context.SetOutput("Color", result);
}
