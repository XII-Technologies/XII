/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusterListClearPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
using namespace xiiLightClusterPassUtils;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusterListClearPass, 1, xiiRTTIDefaultAllocator<xiiClusterListClearPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layout", m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid", m_Grid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices", m_Indices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Overflow", m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusterListClearPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* layout = Layout(context, m_Layout);
  if (!layout)
    return context.ReportFailure("A current-setup immutable cluster layout is required.");
  auto snapshot = layout->m_Constants;

  xiiGALDispatchComputeDescription dispatch;
  if (Dispatch(snapshot, snapshot.Work.y, dispatch).Failed())
    return context.ReportFailure("Cluster operation exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/ClusterListClear.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Constants, m_Grid, m_Indices, m_Overflow;
    xiiLightClusterConstants                 m_Snapshot;
    xiiGALDispatchComputeDescription         m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot; data.m_Dispatch=dispatch; data.m_Pipeline=m_Pipeline;
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Cluster Operation Constants",sizeof(snapshot));

    data.m_Grid=builder.WriteBuffer("Cluster Light Grid",Description(sizeof(xiiVec2U32),snapshot.ClusterCounts.w),xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Indices=builder.WriteBuffer("Cluster Light Indices",Description(sizeof(xiiUInt32),snapshot.Work.y),xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Overflow=builder.WriteBuffer("Cluster List Overflow",Description(sizeof(xiiUInt32),1),xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    Bind(execute,data.m_Constants,data.m_Snapshot,data.m_Pipeline.Borrow());
    auto& commands=execute.GetCommandList();

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
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusterListClearPass);
