/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusterLightAssignmentPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
using namespace xiiLightClusterPassUtils;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusterLightAssignmentPass, 1, xiiRTTIDefaultAllocator<xiiClusterLightAssignmentPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layout", m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights", m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Clusters", m_Clusters)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedGrid", m_InputGrid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedIndices", m_InputIndices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedOverflow", m_InputOverflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid", m_Grid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices", m_Indices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Overflow", m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusterLightAssignmentPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* layout = Layout(context, m_Layout);
  if (!layout)
    return context.ReportFailure("A current-setup immutable cluster layout is required.");
  auto snapshot = layout->m_Constants;

  const auto  lights = context.GetInputBuffer(m_Lights), clusters = context.GetInputBuffer(m_Clusters), grid = context.GetInputBuffer(m_InputGrid), indices = context.GetInputBuffer(m_InputIndices), overflow = context.GetInputBuffer(m_InputOverflow);
  const auto* lightDescription = context.m_Graph.GetBufferDescription(lights);
  if (!lightDescription || lightDescription->m_uiSize % sizeof(xiiGpuLightData) || lightDescription->m_uiSize / sizeof(xiiGpuLightData) > 1048576 || !lightDescription->m_uiSize)
    return context.ReportFailure("Cluster assignment requires one to one million canonical light records; use a directional sentinel for an empty stream.");
  snapshot.Work.x       = xiiUInt32(lightDescription->m_uiSize / sizeof(xiiGpuLightData));
  const auto candidates = context.GetInputBuffer(m_Candidates);
  snapshot.Work.w       = candidates.IsValid() ? 1U : 0U;
  if (candidates.IsValid())
  {
    const auto* description = context.m_Graph.GetBufferDescription(candidates);
    if (!description || !description->m_uiSize || description->m_uiSize % sizeof(xiiUInt32) || description->m_uiSize / sizeof(xiiUInt32) > snapshot.Work.x || !Buffer(context, candidates, sizeof(xiiUInt32), description->m_uiSize / sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Cluster candidates require an exact structured uint selection within the canonical light capacity; use an invalid-index sentinel for an empty selection.");
    snapshot.Work.x = xiiUInt32(description->m_uiSize / sizeof(xiiUInt32));
  }
  const auto lightCount = xiiUInt32(lightDescription->m_uiSize / sizeof(xiiGpuLightData));
  const auto flags      = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  if (!Buffer(context, lights, sizeof(xiiGpuLightData), lightCount, xiiGALBindFlags::ShaderResource) || !Buffer(context, clusters, sizeof(xiiGpuClusterBounds), snapshot.ClusterCounts.w, xiiGALBindFlags::ShaderResource) || !Buffer(context, grid, sizeof(xiiVec2U32), snapshot.ClusterCounts.w, flags) || !Buffer(context, indices, sizeof(xiiUInt32), snapshot.Work.y, flags) || !Buffer(context, overflow, sizeof(xiiUInt32), 1, flags) || grid.m_Id == indices.m_Id || grid.m_Id == overflow.m_Id || indices.m_Id == overflow.m_Id)
    return context.ReportFailure("Cluster list resources must match the layout and have distinct writable allocations.");
  xiiGALDispatchComputeDescription dispatch;
  if (Dispatch(snapshot, snapshot.Work.x, dispatch).Failed())
    return context.ReportFailure("Cluster operation exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/ClusterLightAssignment.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  auto                       candidateBuffer = candidates;
  if (!candidateBuffer.IsValid())
  {
    struct EmptySelection
    {
      xiiRenderGraphBufferHandle m_Buffer;
    };
    const auto empty = context.m_Graph.AddPass<EmptySelection>("Unfiltered Cluster Candidate Binding", xiiGALCommandQueueFlags::Transfer, [](EmptySelection& data, xiiRenderGraphBuilder& builder) {
        auto description = Description(sizeof(xiiUInt32), 1);
        description.m_Usage = xiiGALResourceUsage::Mutable;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource;
        data.m_Buffer = builder.WriteBuffer("Unused Typed Candidate", description, xiiGALResourceStateFlags::CopyDestination); }, [](const EmptySelection& data, xiiRenderGraphPassContext& execute) {
        const xiiUInt32 sentinel = xiiInvalidIndex;
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&sentinel), sizeof(sentinel))); });
    candidateBuffer  = empty.first->m_Buffer;
  }
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Constants, m_Lights, m_Candidates, m_Clusters, m_Grid, m_Indices, m_Overflow;
    xiiLightClusterConstants                 m_Snapshot;
    xiiGALDispatchComputeDescription         m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot; data.m_Dispatch=dispatch; data.m_Pipeline=m_Pipeline;
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Cluster Operation Constants",sizeof(snapshot));

    data.m_Lights=builder.ReadBuffer(lights,xiiGALResourceStateFlags::ShaderResource); data.m_Clusters=builder.ReadBuffer(clusters,xiiGALResourceStateFlags::ShaderResource);
    data.m_Candidates=builder.ReadBuffer(candidateBuffer,xiiGALResourceStateFlags::ShaderResource);
    data.m_Grid=builder.ReadWriteBuffer(grid,xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Indices=builder.ReadWriteBuffer(indices,xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Overflow=builder.ReadWriteBuffer(overflow,xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    Bind(execute,data.m_Constants,data.m_Snapshot,data.m_Pipeline.Borrow());
    auto& commands=execute.GetCommandList();

    commands.ResolveAndSetShaderResourceBufferView("g_Lights",execute.GetBuffer(data.m_Lights)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Candidates",execute.GetBuffer(data.m_Candidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Clusters",execute.GetBuffer(data.m_Clusters)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_LightGrid",execute.GetBuffer(data.m_Grid)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_LightIndices",execute.GetBuffer(data.m_Indices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Overflow",execute.GetBuffer(data.m_Overflow)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    commands.DispatchCompute(data.m_Dispatch); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Grid, pass.first->m_Grid));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Indices, pass.first->m_Indices));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Overflow, pass.first->m_Overflow));
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusterLightAssignmentPass);
