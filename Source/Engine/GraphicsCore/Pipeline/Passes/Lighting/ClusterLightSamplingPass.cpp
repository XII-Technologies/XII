/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusterLightSamplingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
using namespace xiiLightClusterPassUtils;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusterLightSamplingPass,1,xiiRTTIDefaultAllocator<xiiClusterLightSamplingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layout",m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights",m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid",m_Grid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices",m_Indices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PackedIndices",m_Packed)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SamplingTotals",m_Totals)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusterLightSamplingPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* layout = Layout(context, m_Layout);
  if (!layout)
    return context.ReportFailure("Sampling requires a current-setup immutable cluster layout.");
  auto        snapshot = layout->m_Constants;
  const auto  lights = context.GetInputBuffer(m_Lights), grid = context.GetInputBuffer(m_Grid), indices = context.GetInputBuffer(m_Indices);
  const auto* description = context.m_Graph.GetBufferDescription(lights);
  if (!description || !description->m_uiSize || description->m_uiSize % sizeof(xiiGpuLightData) || description->m_uiSize / sizeof(xiiGpuLightData) > 1048576)
    return context.ReportFailure("Sampling requires canonical light records within the packed-index limit.");
  snapshot.Work.x = xiiUInt32(description->m_uiSize / sizeof(xiiGpuLightData));
  if (!Buffer(context, lights, sizeof(xiiGpuLightData), snapshot.Work.x, xiiGALBindFlags::ShaderResource) || !Buffer(context, grid, sizeof(xiiVec2U32), snapshot.ClusterCounts.w, xiiGALBindFlags::ShaderResource) || !Buffer(context, indices, sizeof(xiiUInt32), snapshot.Work.y, xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Sampling inputs must match the canonical light and cluster list layouts.");
  xiiGALDispatchComputeDescription dispatch;
  if (Dispatch(snapshot, snapshot.ClusterCounts.w, dispatch).Failed())
    return context.ReportFailure("Cluster sampling exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/ClusterLightSampling.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Constants, m_Lights, m_Grid, m_Indices, m_Packed, m_Totals;
    xiiLightClusterConstants                 m_Snapshot;
    xiiGALDispatchComputeDescription         m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot; data.m_Dispatch=dispatch; data.m_Pipeline=m_Pipeline;
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Cluster Sampling Constants",sizeof(snapshot));
    data.m_Lights=builder.ReadBuffer(lights,xiiGALResourceStateFlags::ShaderResource); data.m_Grid=builder.ReadBuffer(grid,xiiGALResourceStateFlags::ShaderResource); data.m_Indices=builder.ReadBuffer(indices,xiiGALResourceStateFlags::ShaderResource);
    data.m_Packed=builder.WriteBuffer("Packed Cluster CDF",Description(sizeof(xiiUInt32),snapshot.Work.y),xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Totals=builder.WriteBuffer("Cluster Sampling Power",Description(sizeof(float),snapshot.ClusterCounts.w),xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    Bind(execute,data.m_Constants,data.m_Snapshot,data.m_Pipeline.Borrow());
    auto& commands=execute.GetCommandList();
    commands.ResolveAndSetShaderResourceBufferView("g_Lights",execute.GetBuffer(data.m_Lights)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_LightGrid",execute.GetBuffer(data.m_Grid)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_LightIndex",execute.GetBuffer(data.m_Indices)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_PackedIndices",execute.GetBuffer(data.m_Packed)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_LightSamplingTotals",execute.GetBuffer(data.m_Totals)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute(data.m_Dispatch); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Packed, pass.first->m_Packed));
  return context.SetOutput(m_Totals, pass.first->m_Totals);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusterLightSamplingPass);
