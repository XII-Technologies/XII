/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthRasterPass, 3, xiiRTTIDefaultAllocator<xiiDepthRasterPass>)
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
    XII_MEMBER_PROPERTY("DepthTarget", m_InputDepth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneDepth", m_OutputDepth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("CascadeSnapshot", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SpotShadows", m_Spots)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ShadowCaster", m_bShadowCaster)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("CascadeIndex", m_uiCascadeIndex)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 3U)),
    XII_MEMBER_PROPERTY("Viewport", m_Viewport)->AddAttributes(new xiiDefaultValueAttribute(xiiVec4U32::MakeZero())),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("FillMode", xiiGALFillMode, m_FillMode)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFillMode::Solid)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDepthRasterPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return (m_uiCascadeIndex < 4 && (m_FillMode == xiiGALFillMode::Solid || m_FillMode == xiiGALFillMode::Wireframe) &&
          ((m_Viewport.z && m_Viewport.w) || (m_Viewport == xiiVec4U32::MakeZero()))) ?
    XII_SUCCESS :
    XII_FAILURE;
}

void xiiDepthRasterPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  xiiRenderPipelinePass::GetPins(out_pins);
  // Shadow occurrences may forward an absent atlas. Ordinary depth retains its
  // required public contract; only optionality changes, never the reflected ports.
  if (!m_bShadowCaster)
    for (auto& pin : out_pins)
      if (pin.m_sName == "DepthTarget" || pin.m_sName == "SceneDepth" || pin.m_sName == "RasterConstants" || pin.m_sName == "VisibleMeshlets" || pin.m_sName == "VisibleMeshletCount" || pin.m_sName == "DrawCommands" || pin.m_sName == "DrawCommandCount")
        pin.m_bOptional = false;
}

xiiResult xiiDepthRasterPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.GetInputBuffer(m_InputRasterConstants).IsValid())
  {
    if (!m_bShadowCaster)
      return context.ReportFailure("Ordinary depth rasterization requires raster constants.");
    const auto depth = context.GetInputTexture(m_InputDepth);
    return depth.IsValid() ? context.SetOutput(m_OutputDepth, depth) : XII_SUCCESS;
  }
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (m_FillMode == xiiGALFillMode::Wireframe && device->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Depth wireframe rasterization is unsupported by this device.");
  if (m_bUseMeshShaders && device->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("The authored depth mesh-shader path is unsupported by this device.");
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
    return context.ReportFailure("Depth rasterization requires canonical data, raster constants and explicit indirect commands.");
  const auto  capacity = context.m_Graph.GetBufferDescription(arguments)->m_uiSize / (m_bUseMeshShaders ? sizeof(xiiVec3U32) : sizeof(xiiVec4U32));
  const auto& draw     = device->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties;
  if (capacity > draw.m_uiMaxDrawIndirectCount || !draw.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer) || (m_bUseMeshShaders && capacity != 1))
    return context.ReportFailure("Depth indirect command capacity or counter capability is unsupported.");
  const auto  target = context.GetInputTexture(m_InputDepth);
  const auto* image  = context.m_Graph.GetTextureDescription(target);
  if (!image || image->m_Format != xiiGALResourceFormat::D32Float || image->m_Type != xiiGALResourceDimension::Texture2D || image->m_uiSampleCount != 1 || image->m_uiArraySizeOrDepth != 1)
    return context.ReportFailure("Depth rasterization requires an explicit single-sample D32 target.");
  auto viewport = m_Viewport;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterViewport", viewport));
  if (m_bShadowCaster)
  {
    const auto  input    = context.GetInputData(m_Cascades);
    const auto* cascades = xiiDynamicCast<const xiiShadowCascadeSnapshot*>(input.Borrow());
    const auto  spots    = context.GetInputData(m_Spots).Downcast<const xiiSpotShadowSnapshot>();
    if ((cascades && spots) || (!cascades && !spots) || m_uiCascadeIndex >= (spots ? spots->m_Views.GetCount() : cascades->m_Cascades.GetCount()))
      return context.ReportFailure("Active shadow rasterization requires exactly one matching directional or spotlight snapshot.");
    const auto resolution = spots ? spots->m_uiResolution : cascades->m_uiResolution;
    viewport              = xiiVec4U32((m_uiCascadeIndex % 2) * resolution, (m_uiCascadeIndex / 2) * resolution, resolution, resolution);
  }
  if (!(viewport.z && viewport.w) && viewport != xiiVec4U32::MakeZero())
    return context.ReportFailure("RasterViewport must be an explicit nonempty rectangle or all zero for the full image.");
  if (viewport == xiiVec4U32::MakeZero())
    viewport = xiiVec4U32(0, 0, image->m_Size.width, image->m_Size.height);
  if (!viewport.z || !viewport.w || viewport.x > image->m_Size.width || viewport.y > image->m_Size.height || viewport.z > image->m_Size.width - viewport.x || viewport.w > image->m_Size.height - viewport.y)
    return context.ReportFailure("Depth raster viewport is outside its supplied image.");
  if (!m_hShader.IsValid() || m_bCachedMeshShaders != m_bUseMeshShaders || m_CachedFill != m_FillMode || m_bCachedShadowCaster != m_bShadowCaster)
  {
    const auto                                     shader = xiiResourceManager::LoadResource<xiiShaderResource>(m_bUseMeshShaders ? "Shaders/Geometry/DepthMesh.xiiShader" : "Shaders/Geometry/DepthVertexPull.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> variables;
    xiiHashedString                                name, value;
    name.Assign("RENDER_PASS");
    value.Assign(m_bShadowCaster ? "RENDER_PASS_DEPTH_ONLY" : m_FillMode == xiiGALFillMode::Wireframe ? "RENDER_PASS_PICKING_WIREFRAME" :
                                                                                                        "RENDER_PASS_PICKING");
    variables.Insert(name, value);
    m_hShader            = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, variables, false);
    m_bCachedMeshShaders = m_bUseMeshShaders;
    m_bCachedShadowCaster = m_bShadowCaster;
    m_CachedFill         = m_FillMode;
  }
  xiiResourceLock<xiiShaderPermutationResource> shader(m_hShader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Depth raster shader is unavailable.");
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
  struct Data
  {
    xiiRenderGraphBufferHandle                     m_Scene, m_Skinning, m_Visible, m_Count, m_Geometry, m_Meshlets, m_Materials, m_Constants, m_Arguments, m_ArgumentCount;
    xiiRenderGraphTextureHandle                    m_Depth;
    xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
    xiiSharedPtr<xiiGALSampler>                    m_Sampler;
    xiiUInt32                                      m_uiCapacity;
    xiiVec4U32                                     m_Viewport;
    bool                                           m_bMesh;
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
      data.m_Depth = builder.ReadWriteTexture(target, xiiGALResourceStateFlags::DepthWrite);
      data.m_Sampler = m_Sampler; data.m_Viewport = viewport;
      data.m_uiCapacity = static_cast<xiiUInt32>(capacity); data.m_bMesh = m_bUseMeshShaders;
      data.m_Pipeline.m_PipelineType = m_bUseMeshShaders ? xiiGALPipelineType::Mesh : xiiGALPipelineType::Graphics;
      data.m_Pipeline.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
      if (m_bUseMeshShaders) data.m_Pipeline.m_pMeshShader = shader->GetGALShader(xiiGALShaderType::Mesh);
      else data.m_Pipeline.m_pVertexShader = shader->GetGALShader(xiiGALShaderType::Vertex);
      data.m_Pipeline.m_pPixelShader = shader->GetGALShader(xiiGALShaderType::Pixel);
      data.m_Pipeline.m_GraphicsPipeline.m_pBlendState = shader->GetBlendState();
      data.m_Pipeline.m_GraphicsPipeline.m_pRasterizerState = shader->GetRasterizerState();
      data.m_Pipeline.m_GraphicsPipeline.m_pDepthStencilState = shader->GetDepthStencilState();
      data.m_Pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
      builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      auto pipeline = data.m_Pipeline;
      pipeline.m_GraphicsPipeline.m_pRenderPass = execute.GetRenderPass();
      pipeline.m_GraphicsPipeline.m_uiSubpassIndex = static_cast<xiiUInt8>(execute.GetSubpassIndex());
      pipeline.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
      auto state = xiiGALPipelineCache::GetPipeline(pipeline);
      if (!state)
      {
        execute.ReportFailure("Could not create the explicit depth raster pipeline.");
        return;
      }
      commands.SetPipelineState(state);
      commands.SetViewport({float(data.m_Viewport.x), float(data.m_Viewport.y), float(data.m_Viewport.z), float(data.m_Viewport.w), 0, 1});
      commands.SetScissorRect({data.m_Viewport.x, data.m_Viewport.y, data.m_Viewport.z, data.m_Viewport.w});
      const auto stage = data.m_bMesh ? xiiGALShaderType::Mesh : xiiGALShaderType::Vertex;
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
      commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
      if (data.m_bMesh) commands.DrawMeshIndirect({execute.GetBuffer(data.m_Arguments), 1, 0, xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)});
      else commands.DrawIndirect({execute.GetBuffer(data.m_Arguments), data.m_uiCapacity, 0, sizeof(xiiVec4U32), xiiGALStateTransitionMode::None, execute.GetBuffer(data.m_ArgumentCount)}); });
  return context.SetOutput(m_OutputDepth, emitted.first->m_Depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_DepthRasterPass);
