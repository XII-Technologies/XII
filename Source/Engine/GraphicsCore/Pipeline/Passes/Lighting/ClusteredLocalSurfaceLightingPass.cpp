/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/SurfaceLightingPassUtils.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredLocalSurfaceLightingPass,1,xiiRTTIDefaultAllocator<xiiClusteredLocalSurfaceLightingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lighting",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Layout",m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights",m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Counts",m_Counts)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates",m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IESProfiles",m_Profiles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid",m_Grid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices",m_Indices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Overflow",m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scale",m_fScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusteredLocalSurfaceLightingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScale) && m_fScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiClusteredLocalSurfaceLightingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiSurfaceLightingPassUtils::Inputs surface{context.GetInputTexture(m_Depth), context.GetInputTexture(m_Normal), context.GetInputTexture(m_Albedo), context.GetInputTexture(m_Material), context.GetInputTexture(m_Emissive), context.GetInputBuffer(m_Camera), {}};
  XII_SUCCEED_OR_RETURN(xiiSurfaceLightingPassUtils::Validate(context, surface));
  const auto  frame  = context.GetInputData(m_Layout);
  const auto* layout = frame ? xiiDynamicCast<const xiiLightClusterFrameData*>(frame.Borrow()) : nullptr;
  if (!layout || layout->m_Constants.Viewport.x != surface.m_Size.width || layout->m_Constants.Viewport.y != surface.m_Size.height)
    return context.ReportFailure("Local lighting requires a current cluster layout matching the surface extent.");
  const auto  lights = context.GetInputBuffer(m_Lights), counts = context.GetInputBuffer(m_Counts), candidates = context.GetInputBuffer(m_Candidates), profiles = context.GetInputBuffer(m_Profiles), grid = context.GetInputBuffer(m_Grid), indices = context.GetInputBuffer(m_Indices), overflow = context.GetInputBuffer(m_Overflow);
  const auto* records   = context.m_Graph.GetBufferDescription(lights);
  const auto* selection = context.m_Graph.GetBufferDescription(candidates);
  const auto* ies       = context.m_Graph.GetBufferDescription(profiles);
  using xiiLightClusterPassUtils::Buffer;
  if (!records || !records->m_uiSize || records->m_uiSize % sizeof(xiiGpuLightData) || records->m_uiSize / sizeof(xiiGpuLightData) > 1048576 || !Buffer(context, lights, sizeof(xiiGpuLightData), records->m_uiSize / sizeof(xiiGpuLightData), xiiGALBindFlags::ShaderResource) ||
      !selection || !selection->m_uiSize || selection->m_uiSize % sizeof(xiiUInt32) || selection->m_uiSize / sizeof(xiiUInt32) > records->m_uiSize / sizeof(xiiGpuLightData) || !Buffer(context, candidates, sizeof(xiiUInt32), selection->m_uiSize / sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource) ||
      !ies || !ies->m_uiSize || ies->m_uiSize % sizeof(float) || (ies->m_uiSize != sizeof(float) && ies->m_uiSize % (2048 * sizeof(float))) || !Buffer(context, profiles, sizeof(float), ies->m_uiSize / sizeof(float), xiiGALBindFlags::ShaderResource) ||
      !Buffer(context, counts, sizeof(xiiVec4U32), 1, xiiGALBindFlags::ShaderResource) || !Buffer(context, grid, sizeof(xiiVec2U32), layout->m_Constants.ClusterCounts.w, xiiGALBindFlags::ShaderResource) || !Buffer(context, indices, sizeof(xiiUInt32), layout->m_Constants.Work.y, xiiGALBindFlags::ShaderResource) || !Buffer(context, overflow, sizeof(xiiUInt32), 1, xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Local lighting requires exact canonical light, candidate, IES and cluster-list layouts.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLighting.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSurfaceLightingPassUtils::Inputs      m_Surface;
    xiiRenderGraphTextureHandle              m_Output;
    xiiRenderGraphBufferHandle               m_Constants, m_Layout, m_Lights, m_Counts, m_Candidates, m_Profiles, m_Grid, m_Indices, m_Overflow;
    xiiSurfaceLightingConstants              m_Snapshot;
    xiiLightClusterConstants                 m_ClusterSnapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Surface=xiiSurfaceLightingPassUtils::Read(surface,builder); data.m_Pipeline=m_Pipeline; data.m_Output=xiiSurfaceLightingPassUtils::Output(builder,surface.m_Size);
    data.m_Snapshot.SurfaceExtent=xiiVec4U32(surface.m_Size.width,surface.m_Size.height,0,0); data.m_Snapshot.SurfaceEvaluation=xiiVec4(m_fScale,0,0,0);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Surface Evaluation",sizeof(xiiSurfaceLightingConstants));
    data.m_ClusterSnapshot=layout->m_Constants; data.m_Layout=xiiRenderPassUtils::Constants(builder,"Cluster Layout",sizeof(xiiLightClusterConstants));
    data.m_Lights=builder.ReadBuffer(lights,xiiGALResourceStateFlags::ShaderResource);
    data.m_Counts=builder.ReadBuffer(counts,xiiGALResourceStateFlags::ShaderResource);
    data.m_Candidates=builder.ReadBuffer(candidates,xiiGALResourceStateFlags::ShaderResource);
    data.m_Profiles=builder.ReadBuffer(profiles,xiiGALResourceStateFlags::ShaderResource);
    data.m_Grid=builder.ReadBuffer(grid,xiiGALResourceStateFlags::ShaderResource);
    data.m_Indices=builder.ReadBuffer(indices,xiiGALResourceStateFlags::ShaderResource);
    data.m_Overflow=builder.ReadBuffer(overflow,xiiGALResourceStateFlags::ShaderResource); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow()); xiiSurfaceLightingPassUtils::Bind(data.m_Surface,execute); xiiSurfaceLightingPassUtils::BindConstants(execute,data.m_Constants,data.m_Snapshot);
    xiiLightClusterPassUtils::Bind(execute,data.m_Layout,data.m_ClusterSnapshot,data.m_Pipeline.Borrow());
    commands.ResolveAndSetShaderResourceBufferView("g_Lights",execute.GetBuffer(data.m_Lights)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Counts",execute.GetBuffer(data.m_Counts)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Candidates",execute.GetBuffer(data.m_Candidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_IESProfiles",execute.GetBuffer(data.m_Profiles)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_LightGrid",execute.GetBuffer(data.m_Grid)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_LightIndices",execute.GetBuffer(data.m_Indices)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Overflow",execute.GetBuffer(data.m_Overflow)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Lighting",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute({(data.m_Surface.m_Size.width+7U)/8U,(data.m_Surface.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusteredLocalSurfaceLightingPass);
