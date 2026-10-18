/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGBufferRasterPass, 1, xiiRTTIDefaultAllocator<xiiGBufferRasterPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_InputSceneInstances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SkinningMatrices", m_InputSkinningMatrices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_InputVisibleMeshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_InputVisibleMeshletCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_InputGeometryMetadata)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_InputMeshletMetadata)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SurfaceMaterials", m_InputSurfaceMaterials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RasterConstants", m_InputRasterConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommands", m_InputDrawCommands)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommandCount", m_InputDrawCommandCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneDepth", m_InputDepth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo", m_OutputAlbedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal", m_OutputNormal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material", m_OutputMaterial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive", m_OutputEmissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("NormalRoughness", m_OutputNormalRoughness)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Velocity", m_OutputVelocity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ObjectId", m_OutputObjectId)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("FillMode", xiiGALFillMode, m_FillMode)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFillMode::Solid)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiGBufferRasterPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_FillMode == xiiGALFillMode::Solid || m_FillMode == xiiGALFillMode::Wireframe ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGBufferRasterPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (m_FillMode == xiiGALFillMode::Wireframe && device->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("G-buffer wireframe rasterization is unsupported by this device.");
  if (m_bUseMeshShaders && device->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("The authored G-buffer mesh-shader path is unsupported by this device.");
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
    return context.ReportFailure("G-buffer rasterization requires canonical data, raster constants and explicit indirect commands.");
  const auto  capacity = context.m_Graph.GetBufferDescription(arguments)->m_uiSize / (m_bUseMeshShaders ? sizeof(xiiVec3U32) : sizeof(xiiVec4U32));
  const auto& draw     = device->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties;
  if (capacity > draw.m_uiMaxDrawIndirectCount || !draw.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer) || (m_bUseMeshShaders && capacity != 1))
    return context.ReportFailure("G-buffer indirect command capacity or counter capability is unsupported.");
  const auto width = context.m_pView->GetRenderResolutionWidth(), height = context.m_pView->GetRenderResolutionHeight();
  if (!width || !height || width > 65536 || height > 65536)
    return context.ReportFailure("Invalid G-buffer raster resolution.");
  const auto  depth            = context.GetInputTexture(m_InputDepth);
  const auto* depthDescription = context.m_Graph.GetTextureDescription(depth);
  if (!depthDescription || depthDescription->m_Format != xiiGALResourceFormat::D32Float || depthDescription->m_Type != xiiGALResourceDimension::Texture2D || depthDescription->m_uiArraySizeOrDepth != 1 || depthDescription->m_uiSampleCount != 1 || depthDescription->m_Size.width != width || depthDescription->m_Size.height != height || !depthDescription->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
    return context.ReportFailure("G-buffer rasterization requires a matching single-sample D32 depth attachment.");
  if (!m_hShader.IsValid() || m_bCachedMeshShaders != m_bUseMeshShaders || m_CachedFill != m_FillMode)
  {
    const auto shader    = xiiResourceManager::LoadResource<xiiShaderResource>(m_bUseMeshShaders ? "Shaders/Geometry/GBufferMesh.xiiShader" : "Shaders/Geometry/GBufferVertexPull.xiiShader");
    m_hShader            = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
    m_bCachedMeshShaders = m_bUseMeshShaders;
    m_CachedFill         = m_FillMode;
  }
  xiiResourceLock<xiiShaderPermutationResource> shader(m_hShader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("G-buffer raster shader is unavailable.");
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
  if (!m_Rasterizer || m_Rasterizer->GetDescription().m_FillMode != m_FillMode)
  {
    auto description       = shader->GetRasterizerState()->GetDescription();
    description.m_FillMode = m_FillMode;
    m_Rasterizer           = device->CreateRasterizerState(description);
  }
  if (!m_Rasterizer)
    return context.ReportFailure("Could not create the G-buffer rasterizer state.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle          m_Scene, m_Skinning, m_Visible, m_Count, m_Geometry, m_Meshlets, m_Materials, m_Constants, m_Arguments, m_ArgumentCount;
    xiiRenderGraphTextureHandle         m_Depth, m_Albedo, m_Normal, m_Material, m_Emissive, m_NormalRoughness, m_Velocity, m_ObjectId;
    xiiShaderPermutationResourceHandle  m_Shader;
    xiiSharedPtr<xiiGALSampler>         m_Sampler;
    xiiUInt32                           m_uiWidth, m_uiHeight, m_uiCapacity;
    xiiSharedPtr<xiiGALRasterizerState> m_Rasterizer;
    bool                                m_bMesh;
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
      data.m_Arguments = builder.ReadBuffer(arguments, xiiGALResourceStateFlags::IndirectArgument);
      data.m_ArgumentCount = builder.ReadBuffer(argumentCount, xiiGALResourceStateFlags::IndirectArgument);
      data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::DepthRead);
      xiiGALTextureCreationDescription target;
      target.m_Type = xiiGALResourceDimension::Texture2D;
      target.m_Size = {width, height};
      target.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      target.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_Albedo = builder.WriteTexture("Albedo", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::RG16SNormalized;
      data.m_Normal = builder.WriteTexture("Normal", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_Material = builder.WriteTexture("Material", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::RGBA16Float;
      data.m_Emissive = builder.WriteTexture("Emissive", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      data.m_NormalRoughness = builder.WriteTexture("Normal Roughness", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::RG16Float;
      data.m_Velocity = builder.WriteTexture("Velocity", target, xiiGALResourceStateFlags::RenderTarget);
      target.m_Format = xiiGALResourceFormat::R32UInt;
      data.m_ObjectId = builder.WriteTexture("Object Identity", target, xiiGALResourceStateFlags::RenderTarget);
      data.m_Shader = m_hShader; data.m_Sampler = m_Sampler;
      data.m_uiWidth = width; data.m_uiHeight = height; data.m_uiCapacity = static_cast<xiiUInt32>(capacity); data.m_bMesh = m_bUseMeshShaders; data.m_Rasterizer = m_Rasterizer;
      builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Albedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0, 0, 0, 1));
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.5f, 0, 0));
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Material)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0, 1, 0));
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Emissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_NormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.5f, 0.5f, 1));
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_Velocity)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands, execute.GetTexture(data.m_ObjectId)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
      xiiResourceLock<xiiShaderPermutationResource> shader(data.m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
      if (!shader.IsValid() || !shader->IsShaderValid()) { xiiLog::Error("G-buffer raster shader became unavailable."); return; }
      xiiGALGraphicsPipelineStateCreationDescription pipeline;
      pipeline.m_PipelineType = data.m_bMesh ? xiiGALPipelineType::Mesh : xiiGALPipelineType::Graphics;
      pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
      if (data.m_bMesh) pipeline.m_pMeshShader = shader->GetGALShader(xiiGALShaderType::Mesh);
      else pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex);
      pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
      pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState();
      pipeline.m_GraphicsPipeline.m_pRasterizerState = data.m_Rasterizer;
      pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
      pipeline.m_GraphicsPipeline.m_pRenderPass = execute.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(execute.GetSubpassIndex());
      pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state) { xiiLog::Error("Could not create the explicit G-buffer raster pipeline."); return; }
      commands.SetPipelineState(state);
      commands.SetViewport({0, 0, float(data.m_uiWidth), float(data.m_uiHeight), 0, 1});
      const auto stage = data.m_bMesh ? xiiGALShaderType::Mesh : xiiGALShaderType::Vertex;
      commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
      commands.ResolveAndSetSampler("g_sLinearWrap", data.m_Sampler.Borrow());
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
      commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
      if (data.m_bMesh) commands.DrawMeshIndirect({execute.GetBuffer(data.m_Arguments), 1, 0, xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)});
      else commands.DrawIndirect({execute.GetBuffer(data.m_Arguments), data.m_uiCapacity, 0, sizeof(xiiVec4U32), xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputAlbedo, emitted.first->m_Albedo));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputNormal, emitted.first->m_Normal));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputMaterial, emitted.first->m_Material));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputEmissive, emitted.first->m_Emissive));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputNormalRoughness, emitted.first->m_NormalRoughness));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputVelocity, emitted.first->m_Velocity));
  return context.SetOutput(m_OutputObjectId, emitted.first->m_ObjectId);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_GBufferRasterPass);
