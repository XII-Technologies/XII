/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Pipeline/Passes/Output/ViewRenderModeConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceQueryPass, 2, xiiRTTIDefaultAllocator<xiiSurfaceQueryPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_InputSceneInstances)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("SkinningMatrices", m_InputSkinningMatrices)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_InputVisibleMeshlets)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_InputVisibleMeshletCount)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_InputGeometryMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_InputMeshletMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("SurfaceMaterials", m_InputSurfaceMaterials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("RasterConstants", m_InputRasterConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("DrawCommands", m_InputDrawCommands)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("DrawCommandCount", m_InputDrawCommandCount)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("SceneDepth", m_OutputDepth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Surface", m_OutputSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("OnlyViewWireframe")),
    XII_MEMBER_PROPERTY("Overdraw", m_OutputOverdraw)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OnlyViewWireframe", m_bOnlyViewWireframe)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("VisualizeView", m_bVisualizeView)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("FillMode", xiiGALFillMode, m_FillMode)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFillMode::Solid)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSurfaceQueryPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_FillMode == xiiGALFillMode::Solid || m_FillMode == xiiGALFillMode::Wireframe ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiSurfaceQueryPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto mode        = static_cast<xiiViewRenderMode::Enum>(context.m_pView->GetViewRenderMode().GetValue());
  const bool diagnostics = m_bVisualizeView && xiiViewRenderMode::IsSurfaceVisualizationMode(mode);
  const bool overdraw    = m_bVisualizeView && mode == xiiViewRenderMode::Overdraw;
  if (m_bOnlyViewWireframe && mode != xiiViewRenderMode::Wireframe && !diagnostics && !overdraw)
    return XII_SUCCESS;
  if ((diagnostics || overdraw) && m_bUseMeshShaders)
    return context.ReportFailure("Geometry diagnostics require the authored indexed surface-query path.");
  const auto fill   = diagnostics || overdraw ? xiiGALFillMode::Solid : static_cast<xiiGALFillMode::Enum>(m_FillMode.GetValue());
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (fill == xiiGALFillMode::Wireframe && device->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Surface wireframe rasterization is unsupported by this device.");
  if (m_bUseMeshShaders && device->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("The authored surface mesh-shader path is unsupported by this device.");
  const auto scene = context.GetInputBuffer(m_InputSceneInstances), skinning = context.GetInputBuffer(m_InputSkinningMatrices);
  const auto visible = context.GetInputBuffer(m_InputVisibleMeshlets), count = context.GetInputBuffer(m_InputVisibleMeshletCount);
  const auto geometry = context.GetInputBuffer(m_InputGeometryMetadata), meshlets = context.GetInputBuffer(m_InputMeshletMetadata), materials = context.GetInputBuffer(m_InputSurfaceMaterials);
  const auto constants = context.GetInputBuffer(m_InputRasterConstants), arguments = context.GetInputBuffer(m_InputDrawCommands), argumentCount = context.GetInputBuffer(m_InputDrawCommandCount);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride &&
      description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  const auto* uniform = context.m_Graph.GetBufferDescription(constants);
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(skinning, sizeof(xiiMat4)) || !valid(visible, sizeof(xiiVec2U32)) || !valid(count, sizeof(xiiUInt32)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(meshlets, sizeof(xiiMeshlet)) || !valid(materials, sizeof(xiiGpuSurfaceMaterial)) || !valid(arguments, m_bUseMeshShaders ? sizeof(xiiVec3U32) : sizeof(xiiVec4U32)) || !valid(argumentCount, sizeof(xiiUInt32)) || !uniform || uniform->m_uiSize != sizeof(xiiGpuSceneGBufferConstants) || !uniform->m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
    return context.ReportFailure("Surface rasterization requires canonical data, raster constants and explicit indirect commands.");
  const auto  capacity = context.m_Graph.GetBufferDescription(arguments)->m_uiSize / (m_bUseMeshShaders ? sizeof(xiiVec3U32) : sizeof(xiiVec4U32));
  const auto& draw     = device->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties;
  if (capacity > draw.m_uiMaxDrawIndirectCount || !draw.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer) || (m_bUseMeshShaders && capacity != 1))
    return context.ReportFailure("Surface indirect command capacity or counter capability is unsupported.");
  const auto width = context.m_pView->GetRenderResolutionWidth(), height = context.m_pView->GetRenderResolutionHeight();
  if (!width || !height || width > 65536 || height > 65536)
    return context.ReportFailure("Invalid surface raster resolution.");
  if (!m_hShader.IsValid() || m_bCachedMeshShaders != m_bUseMeshShaders || m_CachedFill != fill || m_bCachedDiagnostics != diagnostics || m_bCachedOverdraw != overdraw)
  {
    const auto                                     shader = xiiResourceManager::LoadResource<xiiShaderResource>(overdraw ? "Shaders/Pipeline/GpuSceneOverdraw.xiiShader" : diagnostics ? "Shaders/Pipeline/GpuSceneDiagnostics.xiiShader" :
                                                                                                                  m_bUseMeshShaders                                                    ? "Shaders/Pipeline/GpuScenePicking.xiiShader" :
                                                                                                                                                                                         "Shaders/Pipeline/GpuScenePickingIndexed.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> variables;
    xiiHashedString                                name, value;
    name.Assign("RENDER_PASS");
    value.Assign(fill == xiiGALFillMode::Wireframe ? "RENDER_PASS_PICKING_WIREFRAME" : "RENDER_PASS_PICKING");
    variables.Insert(name, value);
    m_hShader            = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, variables, false);
    m_bCachedMeshShaders = m_bUseMeshShaders;
    m_CachedFill         = fill;
    m_bCachedDiagnostics = diagnostics;
    m_bCachedOverdraw    = overdraw;
  }
  xiiResourceLock<xiiShaderPermutationResource> shader(m_hShader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Surface raster shader is unavailable.");
  if (!m_Sampler)
  {
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter       = xiiGALFilterType::Anisotropic;
    description.m_MagFilter       = xiiGALFilterType::Anisotropic;
    description.m_MipFilter       = xiiGALFilterType::Anisotropic;
    description.m_AddressU        = xiiGALTextureAddressMode::Wrap;
    description.m_AddressV        = xiiGALTextureAddressMode::Wrap;
    description.m_AddressW        = xiiGALTextureAddressMode::Wrap;
    description.m_uiMaxAnisotropy = 8;
    m_Sampler                     = device->CreateSampler(description);
  }
  if (!m_Sampler)
    return XII_FAILURE;
  xiiRenderPassResourceScope scope(context);
  xiiRenderGraphBufferHandle diagnosticConstants;
  if (diagnostics)
  {
    struct Constants
    {
      xiiRenderGraphBufferHandle m_hBuffer;
      xiiViewRenderModeConstants m_Snapshot;
    };
    diagnosticConstants = context.m_Graph.AddPass<Constants>("Surface diagnostic constants", xiiGALCommandQueueFlags::Transfer, [&](Constants& data, xiiRenderGraphBuilder& builder) {
        data.m_hBuffer = xiiRenderPassUtils::Constants(builder, "Surface diagnostics", sizeof(xiiViewRenderModeConstants));
        data.m_Snapshot = {};
        data.m_Snapshot.RenderMode = mode;
        data.m_Snapshot.TexCoord1Offset = offsetof(xiiMeshPackedVertex, m_vTexCoord1);
        data.m_Snapshot.ColorOffset = offsetof(xiiMeshPackedVertex, m_Color0); }, [](const Constants& data, xiiRenderGraphPassContext& execute) {
        auto& commands = execute.GetCommandList();
        {
          xiiGALMapHelper<xiiViewRenderModeConstants> mapped(commands, execute.GetBuffer(data.m_hBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
          *mapped = data.m_Snapshot;
        }
        xiiGALStateTransitionDescription transition;
        transition.m_pResource = execute.GetBuffer(data.m_hBuffer);
        transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
        transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); }).first->m_hBuffer;
  }
  struct Data
  {
    xiiRenderGraphBufferHandle         m_Scene, m_Skinning, m_Visible, m_Count, m_Geometry, m_Meshlets, m_Materials, m_Constants, m_Arguments, m_ArgumentCount;
    xiiRenderGraphTextureHandle        m_Surface, m_Depth;
    xiiShaderPermutationResourceHandle m_Shader;
    xiiSharedPtr<xiiGALSampler>        m_Sampler;
    xiiUInt32                          m_uiWidth, m_uiHeight, m_uiCapacity;
    bool                               m_bMesh;
    xiiRenderGraphBufferHandle         m_Diagnostics;
    bool                               m_bDiagnostics;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource);
      data.m_Skinning = builder.ReadBuffer(skinning, xiiGALResourceStateFlags::ShaderResource);
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      if (m_bUseMeshShaders) data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_Materials = builder.ReadBuffer(materials, xiiGALResourceStateFlags::ShaderResource);
      data.m_Constants = builder.ReadBuffer(constants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_bDiagnostics = diagnostics;
      if (diagnostics)
        data.m_Diagnostics = builder.ReadBuffer(diagnosticConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_Arguments = builder.ReadBuffer(arguments, xiiGALResourceStateFlags::IndirectArgument);
      data.m_ArgumentCount = builder.ReadBuffer(argumentCount, xiiGALResourceStateFlags::IndirectArgument);
      xiiGALTextureCreationDescription target;
      target.m_Type = xiiGALResourceDimension::Texture2D;
      target.m_Size = {width, height};
      target.m_Format    = overdraw ? xiiGALResourceFormat::RGBA32Float : xiiGALResourceFormat::RGBA32UInt;
      target.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      data.m_Surface = builder.WriteTexture("Surface Query", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::D32Float;
      target.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
      if (!overdraw)
        data.m_Depth = builder.WriteTexture("Surface Query Depth", target, xiiGALResourceStateFlags::DepthWrite);
      data.m_Shader = m_hShader; data.m_Sampler = m_Sampler;
      data.m_uiWidth = width; data.m_uiHeight = height; data.m_uiCapacity = static_cast<xiiUInt32>(capacity); data.m_bMesh = m_bUseMeshShaders;
      builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Surface)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      if (data.m_Depth.IsValid())
        xiiRenderPassUtils::ClearDepthPreservingState(commands, execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0);
      xiiResourceLock<xiiShaderPermutationResource> shader(data.m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
      if (!shader.IsValid() || !shader->IsShaderValid())
      {
        execute.ReportFailure("Surface query shader became unavailable.");
        return;
      }
      xiiGALGraphicsPipelineStateCreationDescription pipeline;
      pipeline.m_PipelineType = data.m_bMesh ? xiiGALPipelineType::Mesh : xiiGALPipelineType::Graphics;
      pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
      if (data.m_bMesh) pipeline.m_pMeshShader = shader->GetGALShader(xiiGALShaderType::Mesh);
      else pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex);
      pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
      pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState();
      pipeline.m_GraphicsPipeline.m_pRasterizerState = shader->GetRasterizerState();
      pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
      pipeline.m_GraphicsPipeline.m_pRenderPass = execute.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(execute.GetSubpassIndex());
      pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state)
      {
        execute.ReportFailure("Could not create the explicit surface raster pipeline.");
        return;
      }
      commands.SetPipelineState(state);
      commands.SetViewport({0, 0, float(data.m_uiWidth), float(data.m_uiHeight), 0, 1});
      const auto stage = data.m_bMesh ? xiiGALShaderType::Mesh : xiiGALShaderType::Vertex;
      if (data.m_bDiagnostics)
      {
        commands.ResolveAndSetConstantBuffer("xiiViewRenderModeConstants", execute.GetBuffer(data.m_Diagnostics), stage | xiiGALShaderType::Pixel);
      }
      commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
      commands.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants", execute.GetBuffer(data.m_Constants), stage | xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      commands.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices", execute.GetBuffer(data.m_Skinning)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      if (data.m_bMesh) commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stage);
      commands.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
      xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_Buffers", stage);
      xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_Textures", xiiGALShaderType::Pixel);
      xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", xiiGALShaderType::Pixel);
      if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed())
      {
        execute.ReportFailure("Could not bind surface query resources.");
        return;
      }
      if (data.m_bMesh) commands.DrawMeshIndirect({execute.GetBuffer(data.m_Arguments), 1, 0, xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)});
      else commands.DrawIndirect({execute.GetBuffer(data.m_Arguments), data.m_uiCapacity, 0, sizeof(xiiVec4U32), xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)}); });
  if (overdraw)
    return context.SetOutput(m_OutputOverdraw, emitted.first->m_Surface);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSurface, emitted.first->m_Surface));
  return context.SetOutput(m_OutputDepth, emitted.first->m_Depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visualization_SurfaceQueryPass);
