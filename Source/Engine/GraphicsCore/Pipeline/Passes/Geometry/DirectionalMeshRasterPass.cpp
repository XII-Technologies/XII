/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DirectionalMeshRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <Shaders/Pipeline/Passes/Geometry/DirectionalMeshConstants.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalMeshRasterPass,1,xiiRTTIDefaultAllocator<xiiDirectionalMeshRasterPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances",m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SkinningMatrices",m_Skinning)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata",m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata",m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets",m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount",m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommands",m_Arguments)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommandCount",m_ArgumentCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SurfaceMaterials",m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RasterConstants",m_RasterConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VirtualShadowConstants",m_ShadowConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VirtualPageTable",m_PageTable)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ShadowMap",m_ShadowMap)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VirtualShadowAtlas",m_ShadowAtlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ColorTarget",m_ColorTarget)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DepthTarget",m_DepthTarget)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Color",m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDirectionalMeshRasterPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Directional mesh rasterization requires a device.");
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (device->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled || device->GetFeatures().m_BindlessResources != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Directional mesh rasterization requires mesh shaders and bindless resources.");
  const auto scene           = context.GetInputBuffer(m_Scene);
  const auto skinning        = context.GetInputBuffer(m_Skinning);
  const auto geometry        = context.GetInputBuffer(m_Geometry);
  const auto meshlets        = context.GetInputBuffer(m_Meshlets);
  const auto visible         = context.GetInputBuffer(m_Visible);
  const auto count           = context.GetInputBuffer(m_Count);
  const auto arguments       = context.GetInputBuffer(m_Arguments);
  const auto argumentcount   = context.GetInputBuffer(m_ArgumentCount);
  const auto materials       = context.GetInputBuffer(m_Materials);
  const auto rasterconstants = context.GetInputBuffer(m_RasterConstants);
  const auto constants       = context.GetInputBuffer(m_Constants);
  const auto shadowconstants = context.GetInputBuffer(m_ShadowConstants);
  const auto pagetable       = context.GetInputBuffer(m_PageTable);
  const auto valid           = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* desc = context.m_Graph.GetBufferDescription(handle);
    return desc && desc->m_Mode == xiiGALBufferMode::Structured && desc->m_uiElementByteStride == stride && desc->m_uiSize >= stride && desc->m_uiSize % stride == 0 && desc->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  const auto uniform = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 size) {
    const auto* desc = context.m_Graph.GetBufferDescription(handle);
    return desc && desc->m_uiSize == size && desc->m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer);
  };
  const auto* draws = context.m_Graph.GetBufferDescription(arguments);
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(skinning, sizeof(xiiMat4)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(meshlets, sizeof(xiiMeshlet)) ||
      !valid(visible, sizeof(xiiVec2U32)) || !valid(count, sizeof(xiiUInt32)) || !valid(materials, sizeof(xiiGpuSurfaceMaterial)) || !valid(pagetable, sizeof(xiiGpuVirtualShadowPage)) ||
      !valid(argumentcount, sizeof(xiiUInt32)) || !draws || draws->m_uiSize != sizeof(xiiVec3U32) || !draws->m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments) ||
      !uniform(rasterconstants, sizeof(xiiGpuSceneGBufferConstants)) || !uniform(constants, sizeof(xiiDirectionalMeshConstants)) || !uniform(shadowconstants, sizeof(xiiVirtualShadowSamplingConstants)))
    return context.ReportFailure("Directional mesh rasterization requires canonical structured streams, explicit uniforms and one mesh indirect command.");
  const auto  color = context.GetInputTexture(m_ColorTarget), depth = context.GetInputTexture(m_DepthTarget), shadow = context.GetInputTexture(m_ShadowMap), atlas = context.GetInputTexture(m_ShadowAtlas);
  const auto* colorDesc = context.m_Graph.GetTextureDescription(color);
  const auto* depthDesc = context.m_Graph.GetTextureDescription(depth);
  const auto  image     = [&](xiiRenderGraphTextureHandle handle, xiiGALResourceFormat::Enum format) {
    const auto* desc = context.m_Graph.GetTextureDescription(handle);
    return desc && desc->m_Type == xiiGALResourceDimension::Texture2D && desc->m_uiArraySizeOrDepth == 1 && desc->m_uiSampleCount == 1 && desc->m_Size.HasNonZeroArea() && (format == xiiGALResourceFormat::Unknown || desc->m_Format == format);
  };
  if (!image(color, xiiGALResourceFormat::Unknown) || !image(depth, xiiGALResourceFormat::D32Float) || !image(shadow, xiiGALResourceFormat::D32Float) || !image(atlas, xiiGALResourceFormat::D32Float) || colorDesc->m_Size != depthDesc->m_Size ||
      (colorDesc->m_Format != xiiGALResourceFormat::RGBA8UNormalized && colorDesc->m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB && colorDesc->m_Format != xiiGALResourceFormat::RGBA16Float && colorDesc->m_Format != xiiGALResourceFormat::RGBA32Float) ||
      !colorDesc->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || !depthDesc->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
    return context.ReportFailure("Directional mesh rasterization requires matching color/D32 targets and supplied single-sample shadow images.");
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Geometry/DirectionalMesh.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Directional mesh shader is unavailable.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(m_Sampler));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle                     m_Scene, m_Skinning, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Arguments, m_ArgumentCount, m_Materials, m_Constants, m_ShadowConstants, m_PageTable, m_RasterConstants;
    xiiRenderGraphTextureHandle                    m_Color, m_Depth, m_Shadow, m_Atlas;
    xiiGALGraphicsPipelineStateCreationDescription m_Pipeline;
    xiiSharedPtr<xiiGALSampler>                    m_Sampler;
    xiiSizeU32                                     m_Size;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Scene=builder.ReadBuffer(scene,xiiGALResourceStateFlags::ShaderResource);
    data.m_Skinning=builder.ReadBuffer(skinning,xiiGALResourceStateFlags::ShaderResource);
    data.m_Geometry=builder.ReadBuffer(geometry,xiiGALResourceStateFlags::ShaderResource);
    data.m_Meshlets=builder.ReadBuffer(meshlets,xiiGALResourceStateFlags::ShaderResource);
    data.m_Visible=builder.ReadBuffer(visible,xiiGALResourceStateFlags::ShaderResource);
    data.m_Count=builder.ReadBuffer(count,xiiGALResourceStateFlags::ShaderResource);
    data.m_Arguments=builder.ReadBuffer(arguments,xiiGALResourceStateFlags::IndirectArgument);
    data.m_ArgumentCount=builder.ReadBuffer(argumentcount,xiiGALResourceStateFlags::IndirectArgument);
    data.m_Materials=builder.ReadBuffer(materials,xiiGALResourceStateFlags::ShaderResource);
    data.m_RasterConstants=builder.ReadBuffer(rasterconstants,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants=builder.ReadBuffer(constants,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_ShadowConstants=builder.ReadBuffer(shadowconstants,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_PageTable=builder.ReadBuffer(pagetable,xiiGALResourceStateFlags::ShaderResource);
    data.m_Color=builder.ReadWriteTexture(color,xiiGALResourceStateFlags::RenderTarget);
    data.m_Depth=builder.ReadWriteTexture(depth,xiiGALResourceStateFlags::DepthWrite);
    data.m_Shadow=builder.ReadTexture(shadow,xiiGALResourceStateFlags::ShaderResource); data.m_Atlas=builder.ReadTexture(atlas,xiiGALResourceStateFlags::ShaderResource);
    data.m_Size=colorDesc->m_Size; data.m_Sampler=m_Sampler;
    auto& pipeline=data.m_Pipeline;
    pipeline.m_PipelineType=xiiGALPipelineType::Mesh;
    pipeline.m_pPipelineResourceSignature=shader->GetPipelineResourceSignature(); pipeline.m_pMeshShader=shader->GetGALShader(xiiGALShaderType::Mesh); pipeline.m_pPixelShader=shader->GetGALShader(xiiGALShaderType::Pixel);
    pipeline.m_GraphicsPipeline.m_pBlendState=shader->GetBlendState(); pipeline.m_GraphicsPipeline.m_pRasterizerState=shader->GetRasterizerState(); pipeline.m_GraphicsPipeline.m_pDepthStencilState=shader->GetDepthStencilState();
    pipeline.m_GraphicsPipeline.m_PrimitiveTopology=xiiGALPrimitiveTopology::TriangleList;
    builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto pipeline=data.m_Pipeline; pipeline.m_GraphicsPipeline.m_pRenderPass=execute.GetRenderPass(); pipeline.m_GraphicsPipeline.m_uiSubpassIndex=static_cast<xiiUInt8>(execute.GetSubpassIndex());
    const auto state=xiiGALPipelineCache::GetPipeline(pipeline);
    if (!state) { xiiLog::Error("Could not create directional mesh raster state."); return; }
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(state.Borrow());
    commands.SetViewport({0,0,float(data.m_Size.width),float(data.m_Size.height),0,1}); commands.SetScissorRect({0,0,data.m_Size.width,data.m_Size.height});
    commands.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants",execute.GetBuffer(data.m_RasterConstants),xiiGALShaderType::Mesh | xiiGALShaderType::Pixel);
    commands.ResolveAndSetConstantBuffer("xiiDirectionalMeshConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Pixel);
    commands.ResolveAndSetConstantBuffer("xiiVirtualShadowSamplingConstants",execute.GetBuffer(data.m_ShadowConstants),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances",execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices",execute.GetBuffer(data.m_Skinning)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_Geometry",execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_Meshlets",execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets",execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount",execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Mesh);
    commands.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials",execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_VirtualShadowPageTable",execute.GetBuffer(data.m_PageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("g_ShadowMap",execute.GetTexture(data.m_Shadow)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceTextureView("g_VirtualShadowAtlas",execute.GetTexture(data.m_Atlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetSampler("PointClampSampler",data.m_Sampler.Borrow(),xiiGALShaderType::Pixel);
    xiiGALBindlessResourceTable::BindBufferSRVs(commands,"g_Buffers",xiiGALShaderType::Mesh);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
    commands.DrawMeshIndirect({execute.GetBuffer(data.m_Arguments),1,0,xiiGALStateTransitionMode::None,execute.GetBuffer(data.m_ArgumentCount)}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Color, pass.first->m_Color));
  return context.SetOutput(m_Depth, pass.first->m_Depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_DirectionalMeshRasterPass);
