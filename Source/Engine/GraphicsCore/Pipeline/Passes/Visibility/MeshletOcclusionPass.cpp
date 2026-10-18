/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletOcclusionPass.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Visibility/DepthOcclusionConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletOcclusionPass, 1, xiiRTTIDefaultAllocator<xiiMeshletOcclusionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Instances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("OcclusionView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("CandidateCount", m_CandidateCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialCount", m_InitialCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DepthPyramid", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DepthBias", m_fDepthBias)->AddAttributes(new xiiDefaultValueAttribute(0.0005f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiMeshletOcclusionPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fDepthBias) && m_fDepthBias >= 0 && m_fDepthBias <= 1 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiMeshletOcclusionPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Instances), view = context.GetInputBuffer(m_View);
  const auto candidates = context.GetInputBuffer(m_Candidates), count = context.GetInputBuffer(m_CandidateCount), initial = context.GetInputBuffer(m_InitialCount);
  const auto depth    = context.GetInputTexture(m_Depth);
  const auto meshlets = context.GetInputBuffer(m_Meshlets);
  const auto valid    = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  const auto* image = context.m_Graph.GetTextureDescription(depth);
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(candidates, sizeof(xiiVec2U32)) || !valid(meshlets, sizeof(xiiMeshlet)) || !valid(count, sizeof(xiiUInt32)) || !valid(initial, sizeof(xiiUInt32)) || initial.m_Id == count.m_Id || initial.m_Id == candidates.m_Id || !image || image->m_Format != xiiGALResourceFormat::R32Float || image->m_Type != xiiGALResourceDimension::Texture2D || image->m_uiSampleCount != 1 || image->m_uiArraySizeOrDepth != 1 || !image->m_Size.width || !image->m_Size.height)
    return context.ReportFailure("Meshlet occlusion requires canonical scene/view data, bounded meshlet pairs, an independent zero counter and an R32 depth pyramid.");
  const auto capacity = context.m_Graph.GetBufferDescription(candidates)->m_uiSize / sizeof(xiiVec2U32);
  const auto groups   = (capacity + 63ULL) / 64;
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (groups > limits.m_uiMaxThreadGroupCountX || limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64)
    return context.ReportFailure("Meshlet occlusion exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/MeshletOcclusion.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Scene, m_View, m_Meshlets, m_Candidates, m_CandidateCount, m_Visible, m_Count, m_Constants;
    xiiRenderGraphTextureHandle              m_Depth;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiUInt32                                m_uiGroups;
    float                                    m_fBias;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
    data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource); data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
    data.m_Candidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource); data.m_CandidateCount = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource); data.m_Count = builder.ReadWriteBuffer(initial, xiiGALResourceStateFlags::UnorderedAccess);
    auto output = *context.m_Graph.GetBufferDescription(candidates); output.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    output.m_Usage = xiiGALResourceUsage::Default; output.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
    data.m_Visible = builder.WriteBuffer("Occlusion Visible Meshlets", output, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Occlusion Parameters", sizeof(xiiDepthOcclusionConstants));
    data.m_Pipeline = m_Pipeline; data.m_uiGroups = static_cast<xiiUInt32>(groups); data.m_fBias = m_fDepthBias; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiDepthOcclusionConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); mapped->DepthBias = data.m_fBias; }
    xiiGALStateTransitionDescription transition; transition.m_pResource = execute.GetBuffer(data.m_Constants); transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiDepthOcclusionConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_OcclusionView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Candidates", execute.GetBuffer(data.m_Candidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_CandidateCount", execute.GetBuffer(data.m_CandidateCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_DepthPyramid", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Visible", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Count", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess(); commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Visible, emitted.first->m_Visible));
  return context.SetOutput(m_Count, emitted.first->m_Count);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_MeshletOcclusionPass);
