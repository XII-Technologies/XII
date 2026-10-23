/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusterBoundsPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
using namespace xiiLightClusterPassUtils;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusterBoundsPass, 1, xiiRTTIDefaultAllocator<xiiClusterBoundsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layout", m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Clusters", m_Clusters)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusterBoundsPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* layout = Layout(context, m_Layout);
  if (!layout)
    return context.ReportFailure("A current-setup immutable cluster layout is required.");
  auto snapshot = layout->m_Constants;

  xiiGALDispatchComputeDescription dispatch;
  if (Dispatch(snapshot, snapshot.ClusterCounts.w, dispatch).Failed())
    return context.ReportFailure("Cluster operation exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/ClusterBounds.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Constants, m_Clusters;
    xiiLightClusterConstants                 m_Snapshot;
    xiiGALDispatchComputeDescription         m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot; data.m_Dispatch=dispatch; data.m_Pipeline=m_Pipeline;
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Cluster Operation Constants",sizeof(snapshot));

    data.m_Clusters=builder.WriteBuffer("Cluster Bounds",Description(sizeof(xiiGpuClusterBounds),snapshot.ClusterCounts.w),xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    Bind(execute,data.m_Constants,data.m_Snapshot,data.m_Pipeline.Borrow());
    auto& commands=execute.GetCommandList();

    commands.ResolveAndSetUnorderedAccessBufferView("g_Clusters",execute.GetBuffer(data.m_Clusters)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    commands.DispatchCompute(data.m_Dispatch); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Clusters, pass.first->m_Clusters));
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusterBoundsPass);
